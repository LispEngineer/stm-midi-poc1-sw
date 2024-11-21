/*
 * usartdma.h
 *
 *  Created on: 2024-11-17
 *  Updated on: 2024-11-21
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#ifndef INC_USARTDMA_H_
#define INC_USARTDMA_H_

#include "stm32f7xx_ll_usart.h"
#include "stm32f7xx_ll_dma.h"


typedef enum udcr_return_value {
  UDCR_OK,
  UDCR_IN_USE, // This USART channel is already in use (init error)
  UDCR_IGNORED, // Bad input values; the call is being ignroed (init error)
  UDCR_FULL // No more interrupt entries (init error)
} udcr_return_value_t;


typedef struct usart_dma_config {
  // Creator set fields

  // The USART we're using
  USART_TypeDef *usartx;  // e.g., USART2

  // Receiving DMA channel (set for circular receive)
  DMA_TypeDef *dma_rx;  // e.g., DMA1
  uint32_t dma_rx_stream;  // e.g., LL_DMA_STREAM_5

  // Sending DMA channel (set for one-off send)
  DMA_TypeDef *dma_tx;  // e.g., DMA1
  uint32_t dma_tx_stream;  // e.g., LL_DMA_STREAM_6

  // Receive circular buffer
  uint8_t *rx_buf;
  size_t   rx_buf_sz;

  // Transmit buffers 1 and 2 (same size)
  uint8_t *tx_buf1;
  uint8_t *tx_buf2;
  size_t   tx_buf_sz;

  // Library managed fields ////////////////////////////

  // What byte we should read next from circular rx_buf
  // (this is in units of LL_DMA_GetDataLength)
  uint32_t next_read_pos;

  // Currently queueing buffer
  uint8_t *tx_q_buf;
  uint8_t *tx_q_next;
  size_t   tx_q_sz_remain;

  // Currently sending buffer
  uint8_t *tx_send_buf;
  size_t   tx_send_sz;

  // Are we currently sending something via DMA?
  // This gets reset via interrupt
  volatile uint32_t is_sending;
} usart_dma_config_t;


udcr_return_value_t udcr_init(usart_dma_config_t *udcr);
uint16_t udcr_read_byte(usart_dma_config_t *udcr);

// DMA TX interrupt callback function
void usart_dma_transfer_complete(USART_TypeDef *usartx);

#endif /* INC_USARTDMA_H_ */
