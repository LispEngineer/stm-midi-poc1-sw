/*
 * usartdma.h
 *
 *  Created on: 2024-11-17
 *  Updated on: 2024-11-20
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#ifndef INC_USARTDMA_H_
#define INC_USARTDMA_H_

#include "stm32f7xx_ll_usart.h"
#include "stm32f7xx_ll_dma.h"

typedef struct usart_dma_circular_receive {
  // Creator set fields
  USART_TypeDef *usartx;  // e.g., USART2
  DMA_TypeDef *dmax;  // e.g., DMA1
  uint32_t dma_stream;  // e.g., LL_DMA_STREAM_5
  uint8_t *rx_buf;
  size_t   rx_buf_sz;

  // Library managed fields
  uint32_t next_read_pos;
} usart_dma_circular_receive_t;


void udcr_init(usart_dma_circular_receive_t *udcr);
uint16_t udcr_read_byte(usart_dma_circular_receive_t *udcr);



#endif /* INC_USARTDMA_H_ */
