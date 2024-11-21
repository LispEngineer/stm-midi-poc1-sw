/*
 * usartdma.c
 *
 * USART Receiving: Implements a DMA-based circular buffer receiver
 * with the ability to pull characters when available via a function.
 *
 * USART needs to be configured as follows:
 * 1. LL API
 * 2. DMA RX
 * 3. Circular receive
 * 4. No FIFO
 *
 * Since we're using the LL drivers, we have to manually set the
 * IRQ Handler up - see for example DMA1_Stream6_IRQHandler() in
 * stm32f7xx_it.c. Each one used has to be set up appropriately
 * first, unfortunately.
 *
 *  Created on: 2024-11-17
 *  Updated on: 2024-11-21
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "usartdma.h"


typedef struct udcr_callback_map_entry {
  USART_TypeDef *usartx;
  usart_dma_config_t *udcr;
} udcr_callback_map_entry_t;

#define NUM_USART_CHANNELS 6
/* Our callbacks only have n USARTx in the LL implementation,
 * so we have to map that to our usart_dma_config_t somehow.
 */
// TODO: Make this live in fast data RAM
static udcr_callback_map_entry_t callback_map[NUM_USART_CHANNELS] = { 0 };

/*
 * Callback function when an DMA transfer has completed,
 * mediated by the STM32 LL. This is an interrupt handler.
 *
 * We gratituously blink an LED for now just to show it working.
 * FIXME: Remove LED blinking.
 */
// TODO: Make this live in fast code RAM
void usart_dma_transfer_complete(USART_TypeDef *usartx) {
  if (usartx == NULL) {
    return;
  }

  usart_dma_config_t *udcr = NULL;

  // Find our usart_dma_config_t from usartx
  // (hopefully the optimizer will unroll this loop)
  for (int i = 0; i < NUM_USART_CHANNELS; i++) {
    if (callback_map[i].usartx == usartx) {
      udcr = callback_map[i].udcr;
      break;
    }
  }

  // If we cannot identify the sending interrupt, do nothing
  if (udcr == NULL) {
    return;
  }

  // Flash or transmit stuff for gratuitous purposes
#ifdef TOGGLE_LEDS
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14); // Green LED
  // HAL_UART_Transmit(&huart2, (uint8_t *)"+", 1, HAL_MAX_DELAY);
#endif

  udcr->is_sending = 0;
}



/** Using LL, configures a U(S)ART and DMA for (circular) buffer
 * receive, and starts that receiving.
 *
 * Also configures USART and DMA for one-off circular buffer
 * sending, and initializes the internal queue for sending
 * stuff via USART, but sending will happen later. An interrupt
 * will happen when sending is complete.
 *
 * The caller should set the specified fields before calling
 * this.
 *
 * The LL Interrupt handlers like DMA1_Channel6_IRQHandler
 * have to be set up manually, unfortunately - for now.
 */
udcr_return_value_t udcr_init(usart_dma_config_t *udcr) {
  if (NULL == udcr) {
    return UDCR_IGNORED;
  }

  // Update our callback map so we can handle this new spidma_config_t
  int i;
  for (i = 0; i < NUM_USART_CHANNELS; i++) {
    if (callback_map[i].usartx == udcr->usartx) {
      return UDCR_IN_USE;
    }
  }
  for (i = 0; i < NUM_USART_CHANNELS; i++) {
    if (callback_map[i].usartx == NULL) {
      // Allocate this one
      callback_map[i].usartx = udcr->usartx;
      callback_map[i].udcr = udcr;
      break;
    }
  }
  if (i >= NUM_USART_CHANNELS) {
    // No available entry found
    return UDCR_FULL;
  }

  // And register the callback
  // XXX: START CODING HERE
  // TODO: Check return value

  udcr->is_sending = 0;


  // Set our last rx read position, which is always the buffer size
  udcr->next_read_pos = udcr->rx_buf_sz;

  // Set up our tx buffers
  // We will queue initially into the first buffer
  udcr->tx_q_buf = udcr->tx_buf1;
  udcr->tx_q_next = udcr->tx_q_buf;
  udcr->tx_q_sz_remain = udcr->tx_buf_sz;

  // And we'll send from the other buffer
  udcr->tx_send_buf = udcr->tx_buf2;
  udcr->tx_send_sz = 0;
  udcr->is_sending = 0;

  // Initialze and start the DMA circular buffer receiving
  LL_DMA_ConfigAddresses(udcr->dma_rx, udcr->dma_rx_stream,
      (uint32_t)LL_USART_DMA_GetRegAddr(udcr->usartx, LL_USART_DMA_REG_DATA_RECEIVE),
      (uint32_t)udcr->rx_buf,
      LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetDataLength(udcr->dma_rx, udcr->dma_rx_stream, udcr->rx_buf_sz);
  LL_USART_EnableDMAReq_RX(udcr->usartx);
  LL_DMA_EnableStream(udcr->dma_rx, udcr->dma_rx_stream);

  // Initialize DMA receiving
  return UDCR_OK;
}

/** Returns a byte from the DMA RX circular buffer, if any.
 * Returns > 0xFF if there is no data.
 */
// TODO: Make this vastly more efficient
uint16_t udcr_read_byte(usart_dma_config_t *udcr) {
  uint32_t datalength = LL_DMA_GetDataLength(udcr->dma_rx, udcr->dma_rx_stream);

  if (datalength == udcr->next_read_pos) {
    // DMA has not advanced, so nothing to read
    return 0x100;
  }

  /* datalength - upon initialization, this starts at 32 (rx_buf_sz).
   * As each letter comes in, it goes down. It never reaches 0, but after
   * 1 it goes back to 32.
   *
   * However, it does receive into the buffer in the expected order, that is,
   * the first letter received is at console_rx_buf[0].
   */
  size_t most_recent = udcr->rx_buf_sz - udcr->next_read_pos;

  // datalength decrements each byte received by DMA
  // and wraps at 0 to the starting length,
  // so do that too.
  udcr->next_read_pos--;
  if (udcr->next_read_pos == 0) {
    udcr->next_read_pos = udcr->rx_buf_sz;
  }

  return udcr->rx_buf[most_recent];
}

