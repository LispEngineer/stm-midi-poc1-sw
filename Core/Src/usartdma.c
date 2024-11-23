/*
 * usartdma.c
 *
 * USART Receiving: Implements a DMA-based circular buffer receiver
 * with the ability to pull characters when available via a function.
 *
 * USART Transmitting: Implements a dual-buffer DMA send.
 * One buffer is actually sending, and the other buffer is
 * queuing things for send. Periodically check if the DMA is
 * not sending, and if so, swap buffers and initiate a send.
 *
 * USART needs to be configured as follows:
 * 1. LL API
 * 2. DMA RX & TX
 * 3. Circular receive, Normal Transmit
 * 4. No FIFO
 *
 * Since we're using the LL drivers, we have to manually set the
 * IRQ Handler up - see for example DMA1_Stream6_IRQHandler() in
 * stm32f7xx_it.c. Each one used has to be set up appropriately
 * first, unfortunately.
 *
 *  Created on: 2024-11-17
 *  Updated on: 2024-11-23
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
 * We gratuitously blink an LED for now just to show it working.
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

  // Register the send complete interrupt callback next.
  // The LL system does not have a mechanism for this.
  // You have to hard-code the callback.
  // See: DMA1_Stream6_IRQHandler() in stm32f7xx_it.c
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

  // Initialize and start the DMA circular buffer receiving
  LL_DMA_ConfigAddresses(udcr->dma_rx, udcr->dma_rx_stream,
      (uint32_t)LL_USART_DMA_GetRegAddr(udcr->usartx, LL_USART_DMA_REG_DATA_RECEIVE),
      (uint32_t)udcr->rx_buf,
      LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetDataLength(udcr->dma_rx, udcr->dma_rx_stream, udcr->rx_buf_sz);
  LL_DMA_EnableStream(udcr->dma_rx, udcr->dma_rx_stream);
  LL_USART_EnableDMAReq_RX(udcr->usartx);

  // Initialize DMA receiving
  // If we enable TC interrupts, we need to clear the flag upon an interrupt.
  /*
  LL_DMA_EnableIT_TC(udcr->dma_rx, udcr->dma_rx_stream);
  */
  // This would need to be added in DMA1_Stream5_IRQHandler(void) for USART2_RX
  // We seem to be unable to receive more than a full buffer worth,
  // even with circular buffers turned on,
  // if TC interrupts are enabled, unless we clear these interrupts.
  /*
  if (LL_DMA_IsActiveFlag_TC5(DMA1)) {
    LL_DMA_ClearFlag_TC5(DMA1);
  }
  */
  LL_DMA_DisableStream(udcr->dma_tx, udcr->dma_tx_stream);
  LL_DMA_EnableIT_TC(udcr->dma_tx, udcr->dma_tx_stream); // Transmit complete interrupt
  // LL_DMA_EnableIT_TE(udcr->dma_tx, udcr->dma_tx_stream); // Transmit error interrupt
  LL_DMA_ConfigAddresses(udcr->dma_tx, udcr->dma_tx_stream,
      (uint32_t)udcr->tx_buf1,
      (uint32_t)LL_USART_DMA_GetRegAddr(udcr->usartx, LL_USART_DMA_REG_DATA_TRANSMIT),
      LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  return UDCR_OK;
}

/** Call this regularly to send the queued bytes.
 * Return values:
 * UDCR_OK - send started
 * UDCR_IGNORED - nothing to send
 * UDCR_IN_USE - sending already going on
 */
udcr_return_value_t udcr_send_from_queue(usart_dma_config_t *udcr) {
	if (udcr->is_sending) {
		return UDCR_IN_USE;
	}
	if (udcr->tx_q_sz_remain == udcr->tx_buf_sz) {
		// We have nothing to send
		return UDCR_IGNORED;
	}

	udcr->is_sending = 1;

	// Start sending our buffers
	size_t send_sz = udcr->tx_buf_sz - udcr->tx_q_sz_remain;
	uint8_t *temp;

	// Swap our sending and queueing buffers
	temp = udcr->tx_send_buf;
	udcr->tx_send_buf = udcr->tx_q_buf;
	udcr->tx_q_buf = temp;
	udcr->tx_q_next = temp;
	udcr->tx_q_sz_remain = udcr->tx_buf_sz;

	// Begin sending our current sending buffer
	LL_DMA_DisableStream(udcr->dma_tx, udcr->dma_tx_stream);
	// Crazy that the memory address type is a uint32_t than a pointer
	LL_DMA_SetMemoryAddress(udcr->dma_tx, udcr->dma_tx_stream, (uint32_t)udcr->tx_send_buf);
	LL_DMA_SetDataLength(udcr->dma_tx, udcr->dma_tx_stream, send_sz);
	LL_DMA_EnableStream(udcr->dma_tx, udcr->dma_tx_stream);
  LL_USART_EnableDMAReq_TX(udcr->usartx);
  LL_USART_EnableDirectionTx(udcr->usartx);

	return UDCR_OK;
}

/*
 * Returns the # of bytes actually queued.
 */
size_t udcr_queue_bytes(usart_dma_config_t *udcr, const uint8_t *buf, size_t buf_sz) {
	size_t to_enqueue = buf_sz < udcr->tx_q_sz_remain ? buf_sz : udcr->tx_q_sz_remain;

	if (to_enqueue == 0) {
		return 0;
	}

	// Copy the data to send
	// destination, source, size
	memcpy(udcr->tx_q_next, buf, to_enqueue);

	// Increment our next send pointer & reduce our send size remaining
	udcr->tx_q_next += to_enqueue;
	udcr->tx_q_sz_remain -= to_enqueue;

	return to_enqueue;
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

