/*
 * uartrxdmamain.c
 *
 * A testing "main" for exploring how to do DMA-based
 * UART receiving of data.
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
#include "main.h"
#include "usartdma.h"

#ifdef USE_HAL_USART2
#else
#define CONSOLE_UART       USART2 // Low level USART - HAL would be huart2
#define CONSOLE_DMA_RX        DMA1
#define CONSOLE_DMA_RX_STREAM LL_DMA_STREAM_5
#define CONSOLE_DMA_TX        DMA1
#define CONSOLE_DMA_TX_STREAM LL_DMA_STREAM_6
#endif // USE_HAL_USART2
// This is connected to DMA 1 Stream 5


#define BUF_SZ_USART_RX ((size_t)32)
uint8_t console_rx_buf[BUF_SZ_USART_RX];
#define BUF_SZ_USART_TX ((size_t)200)
uint8_t console_tx_buf1[BUF_SZ_USART_RX];
uint8_t console_tx_buf2[BUF_SZ_USART_RX];

void serial_transmit2(char *buf, uint16_t len) {
  for (size_t i = 0; i < len; i++) {
    while (!LL_USART_IsActiveFlag_TXE(CONSOLE_UART));
    LL_USART_TransmitData8(CONSOLE_UART, buf[i]);
  }
}

void uartrxdmamain() {

  usart_dma_config_t console;
  bool x = false;
  uint16_t rxc_or_not; // received character or not
  char rxc;

  console.usartx = CONSOLE_UART;
  console.dma_rx = CONSOLE_DMA_RX;
  console.dma_rx_stream = CONSOLE_DMA_RX_STREAM;
  console.dma_tx = CONSOLE_DMA_TX;
  console.dma_tx_stream = CONSOLE_DMA_TX_STREAM;

  console.rx_buf = console_rx_buf;
  console.rx_buf_sz = sizeof(console_rx_buf);
  console.tx_buf1 = console_tx_buf1;
  console.tx_buf2 = console_tx_buf2;
  console.tx_buf_sz = sizeof(console_tx_buf1);

  udcr_init(&console);

  while (1) {
    x = !x;

    // Every second, print the DMA buffer
    serial_transmit2("\r\n", 2);
    serial_transmit2(x ? "-" : "=", 1);

    while ((rxc_or_not = udcr_read_byte(&console)) < 0x100) {
      rxc = rxc_or_not & 0xFF;
      serial_transmit2(&rxc, 1);
    }
    HAL_Delay(2000);
  }
}


void uartrxdmamain2() {

  bool x = 0;
  uint32_t datalength;
  char buf[64];

  LL_DMA_ConfigAddresses(CONSOLE_DMA_RX, CONSOLE_DMA_RX_STREAM,
      (uint32_t)LL_USART_DMA_GetRegAddr(CONSOLE_UART, LL_USART_DMA_REG_DATA_RECEIVE),
      (uint32_t)console_rx_buf,
      LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  /*
  LL_DMA_SetMemoryAddress(CONSOLE_DMA_RX, CONSOLE_DMA_RX_STREAM, (uint32_t)console_rx_buf);
  LL_DMA_SetMemorySize(CONSOLE_DMA_RX, CONSOLE_DMA_RX_STREAM, BUF_SZ_USART_RX);
  */
  LL_DMA_SetDataLength(CONSOLE_DMA_RX, CONSOLE_DMA_RX_STREAM, sizeof(console_rx_buf));
  LL_USART_EnableDMAReq_RX(CONSOLE_UART);
  // LL_USART_EnableDirectionRx(CONSOLE_UART);
  LL_DMA_EnableStream(CONSOLE_DMA_RX, CONSOLE_DMA_RX_STREAM);

  /*
  LL_DMA_GetCurrentTargetMem(DMAx, Stream);
  LL_DMA_GetMemoryAddress(DMAx, Stream);
  */

  while (1) {
    x = !x;

    datalength = LL_DMA_GetDataLength(CONSOLE_DMA_RX, CONSOLE_DMA_RX_STREAM);

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
