/*
 * uartrxdmamain.c
 *
 * A testing "main" for exploring how to do DMA-based
 * UART receiving of data.
 *
 *  Created on: 2024-11-17
 *  Updated on: 2024-11-17
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#include <stdint.h>
#include <stddef.h>
#include "main.h"

#ifdef USE_HAL_USART2
#else
#define CONSOLE_UART       USART2 // Low level USART - HAL would be huart2
#define CONSOLE_DMA        DMA1
#define CONSOLE_DMA_STREAM LL_DMA_STREAM_5
#endif // USE_HAL_USART2
// This is connected to DMA 1 Stream 5


#define BUF_SZ_USART_RX ((size_t)32)
uint8_t console_rx_buf[BUF_SZ_USART_RX];

void uartrxdmamain() {

  LL_DMA_SetMemoryAddress(CONSOLE_DMA, CONSOLE_DMA_STREAM, (uint32_t)console_rx_buf);
  LL_DMA_SetMemorySize(CONSOLE_DMA, CONSOLE_DMA_STREAM, BUF_SZ_USART_RX);
  LL_USART_EnableDMAReq_RX(CONSOLE_UART);

  while (1) {

    // Every second, print the DMA buffer
    LL_USART_TransmitData8(CONSOLE_UART, '\r');
    LL_USART_TransmitData8(CONSOLE_UART, '\n');
    LL_USART_TransmitData8(CONSOLE_UART, '\r');
    LL_USART_TransmitData8(CONSOLE_UART, '\n');
    for (size_t i = 0; i < BUF_SZ_USART_RX; i++) {
      LL_USART_TransmitData8(CONSOLE_UART, console_rx_buf[i]);
    }
    HAL_Delay(1000);

  }
}
