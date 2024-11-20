/*
 * usartdma.c
 *
 * USART Receiving: Implements a DMA-based circular buffer receiver
 * with the ability to pull characters when available via a function.
 *
 *  Created on: 2024-11-17
 *  Updated on: 2024-11-20
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

/** Using LL, configures a U(S)ART and DMA for (circular) buffer
 * receive, and starts that receiving.
 *
 * The caller should set the specified fields before calling
 * this.
 */
void udcr_init(usart_dma_circular_receive_t *udcr) {
  // Set our last read position, which is always the buffer size
  udcr->next_read_pos = udcr->rx_buf_sz;

  // Initialze and start the DMA circular buffer receiving
  LL_DMA_ConfigAddresses(udcr->dmax, udcr->dma_stream,
      (uint32_t)LL_USART_DMA_GetRegAddr(udcr->usartx, LL_USART_DMA_REG_DATA_RECEIVE),
      (uint32_t)udcr->rx_buf,
      LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetDataLength(udcr->dmax, udcr->dma_stream, udcr->rx_buf_sz);
  LL_USART_EnableDMAReq_RX(udcr->usartx);
  LL_DMA_EnableStream(udcr->dmax, udcr->dma_stream);
}

/** Returns a byte from the DMA RX circular buffer, if any.
 * Returns > 0xFF if there is no data.
 */
// TODO: Make this vastly more efficient
uint16_t udcr_read_byte(usart_dma_circular_receive_t *udcr) {
  uint32_t datalength = LL_DMA_GetDataLength(udcr->dmax, udcr->dma_stream);

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

