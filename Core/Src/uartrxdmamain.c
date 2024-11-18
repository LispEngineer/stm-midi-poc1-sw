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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

void serial_transmit2(char *buf, uint16_t len) {
  for (size_t i = 0; i < len; i++) {
    while (!LL_USART_IsActiveFlag_TXE(CONSOLE_UART));
    LL_USART_TransmitData8(CONSOLE_UART, buf[i]);
  }
}

void uartrxdmamain() {

  bool x = 0;
  uint32_t datalength;
  char buf[64];

  LL_DMA_ConfigAddresses(CONSOLE_DMA, CONSOLE_DMA_STREAM,
      (uint32_t)LL_USART_DMA_GetRegAddr(CONSOLE_UART, LL_USART_DMA_REG_DATA_RECEIVE),
      (uint32_t)console_rx_buf,
      LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  /*
  LL_DMA_SetMemoryAddress(CONSOLE_DMA, CONSOLE_DMA_STREAM, (uint32_t)console_rx_buf);
  LL_DMA_SetMemorySize(CONSOLE_DMA, CONSOLE_DMA_STREAM, BUF_SZ_USART_RX);
  */
  LL_DMA_SetDataLength(CONSOLE_DMA, CONSOLE_DMA_STREAM, sizeof(console_rx_buf));
  LL_USART_EnableDMAReq_RX(CONSOLE_UART);
  // LL_USART_EnableDirectionRx(CONSOLE_UART);
  LL_DMA_EnableStream(CONSOLE_DMA, CONSOLE_DMA_STREAM);

  /*
  LL_DMA_GetCurrentTargetMem(DMAx, Stream);
  LL_DMA_GetMemoryAddress(DMAx, Stream);
  */

  while (1) {
    x = !x;

    datalength = LL_DMA_GetDataLength(CONSOLE_DMA, CONSOLE_DMA_STREAM);

    /*
     * datalength - upon initialization, this starts at 32 (BUF_SZ_UART_RX)
     * As each letter comes in, it goes down. It never reaches 0, but after
     * 1 it goes back to 32.
     *
     * However, it does receive into the buffer in the expected order, that is,
     * the first letter received is at console_rx_buf[0].
     *
     * So, the most recent letter received is:
     */
    size_t most_recent = BUF_SZ_USART_RX - datalength;
    if (most_recent == 0) {
      most_recent = BUF_SZ_USART_RX - 1;
    } else {
      most_recent--;
    }

    // Every second, print the DMA buffer
    serial_transmit2("\r\n", 2);
    serial_transmit2(x ? "-" : "=", 1);
    snprintf(buf, sizeof(buf), "\r\ndl: %lu\r\n", datalength);
    serial_transmit2(buf, strlen(buf));
    snprintf(buf, sizeof(buf), "mr: %u - %c\r\n", most_recent, console_rx_buf[most_recent]);
    serial_transmit2(buf, strlen(buf));
    serial_transmit2((char *)console_rx_buf, BUF_SZ_USART_RX);
    HAL_Delay(1000);

  }
}
