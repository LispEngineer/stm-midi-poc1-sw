/*
 *  Created on: 2024-11-14
 *  Updated on: 2024-11-16
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 *
 * Standalone test spimain() for an ILI9341 display connected
 * to an STM32 board.
 *
 * Configurable things:
 * 1. CONSOLE_UART: The UART to send data to for debugging purposes
 * 2. DISPLAY_SPI: spidma_config_t
 * 3. ILI9341_SPI_PORT: the SPI Handle for SPI display
 * 4. DISPLAY_DMA: The DMA handle for the SPI transmit
 *
 * Conceived from https://github.com/afiskon/stm32-ili9341
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "main.h"
#ifdef INCLUDE_TOUCH
#include "ili9341_touch.h"
#endif // INCLUDE_TOUCH
#include "fonts.h"
#include "testimg.h"
#include "spidma.h"
#include "spidma_ili9341.h"


// From main
#define CONSOLE_UART huart2  // EVT#1 Console Port
extern UART_HandleTypeDef CONSOLE_UART;

#define DISPLAY_SPI spi2tx
#define DISPLAY_SPIP (&(DISPLAY_SPI))
extern spidma_config_t DISPLAY_SPI;

#define ILI9341_SPI_PORT hspi2
extern SPI_HandleTypeDef ILI9341_SPI_PORT;

#define DISPLAY_DMA hdma_spi2_tx
extern DMA_HandleTypeDef DISPLAY_DMA;


/*
 * Transmits synchronously to the UART.
 * Credit: https://github.com/afiskon/stm32-ili9341
 */
static void console_printf(const char* fmt, ...) {
  char buff[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buff, sizeof(buff), fmt, args);
  HAL_UART_Transmit(&CONSOLE_UART, (uint8_t*) buff, strlen(buff), HAL_MAX_DELAY);
  va_end(args);
}

static void print_queue_info(spidma_config_t *spi, int id) {
  console_printf("%d - "
              "ql: %u, qr: %u, sdqf: %u, iliqf: %u, ilics: %u; "
              "fqf: %u, bfqf: %u; "
              "maf: %u, sz: %u; ili_allocs: %u, sd_frees: %u\r\n",

              id,

              spidma_queue_length(spi), spidma_queue_remaining(spi),
              DISPLAY_SPI.entry_queue_failures,
              ili_queue_failures, ili_characters_skipped,

              DISPLAY_SPI.free_queue_failures, DISPLAY_SPI.backup_free_queue_failures,

              ili_mem_alloc_failures, ili_last_alloc_failure_size,
              ili_mem_allocs, DISPLAY_SPI.mem_frees);
}


static void loop(spidma_config_t *spi) {
  spidma_queue(spi, SPIDMA_SELECT, 0, NULL, 1000000);
  spidma_empty_queue(spi);

  // Draw black screen with red border
  spidma_ili9341_fill_screen(spi, ILI9341_BLACK);
  spidma_empty_queue(spi);
  for(int x = 0; x < ILI9341_WIDTH; x++) {
    if (spidma_queue_remaining(spi) < 12) {
      spidma_empty_queue(spi);
    }
    spidma_ili9341_draw_pixel(spi, x, 0, ILI9341_RED);
    spidma_ili9341_draw_pixel(spi, x, ILI9341_HEIGHT-1, ILI9341_RED);
  }

  for(int y = 0; y < ILI9341_HEIGHT; y++) {
    if (spidma_queue_remaining(spi) < 12) {
      spidma_empty_queue(spi);
    }
    spidma_ili9341_draw_pixel(spi, 0, y, ILI9341_RED);
    spidma_ili9341_draw_pixel(spi, ILI9341_WIDTH-1, y, ILI9341_RED);
  }
  spidma_empty_queue(spi);
  HAL_Delay(1000);

  // Check fonts
  uint32_t end_loc;
  uint16_t end_x;
  uint16_t end_y;
  spidma_ili9341_fill_screen(spi, ILI9341_BLACK);
  print_queue_info(spi, 1000);
  spidma_empty_queue(spi);
  print_queue_info(spi, 1001); // Should be 0

  end_loc = spidma_ili9341_write_string(spi, 0, 0, "Font_7x10, red/black, abcdefgABCDEFG", Font_7x10, ILI9341_RED, ILI9341_BLACK);
  print_queue_info(spi, 1002);
  spidma_empty_queue(spi);
  end_x = end_loc & 0xFFFF; // Lower word
  end_y = end_loc >> 16; // Higher word

  print_queue_info(spi, 1003); // Should be 0
  end_loc = spidma_ili9341_write_string(spi, end_x, end_y, "Font_11x18, green/blue, ghijklmGHIJKLM", Font_11x18, ILI9341_GREEN, ILI9341_BLUE);
  print_queue_info(spi, 1004);
  spidma_empty_queue(spi);
  end_x = end_loc & 0xFFFF; // Lower word
  end_y = end_loc >> 16; // Higher word

  print_queue_info(spi, 1005); // Should be 0
  spidma_ili9341_write_string(spi, end_x, end_y, "Font_16x26, black/yellow, mnopqrsMNOPQRS", Font_16x26, ILI9341_BLACK, ILI9341_YELLOW);
  print_queue_info(spi, 1006);
  spidma_empty_queue(spi);
  print_queue_info(spi, 1007); // Should be 0
  HAL_Delay(2000);

  spidma_ili9341_invert(spi, true);
  spidma_empty_queue(spi);
  HAL_Delay(500);
  spidma_ili9341_invert(spi, false);
  spidma_empty_queue(spi);
  HAL_Delay(1000);

  // Check colors
  spidma_ili9341_fill_screen(spi, ILI9341_WHITE);
  spidma_ili9341_write_string(spi, 0, 0, "WHITE", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
  spidma_empty_queue(spi);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_BLUE);
  spidma_ili9341_write_string(spi, 0, 0, "BLUE", Font_11x18, ILI9341_BLACK, ILI9341_BLUE);
  spidma_empty_queue(spi);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_RED);
  spidma_ili9341_write_string(spi, 0, 0, "RED", Font_11x18, ILI9341_BLACK, ILI9341_RED);
  spidma_empty_queue(spi);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_GREEN);
  spidma_ili9341_write_string(spi, 0, 0, "GREEN", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);
  spidma_empty_queue(spi);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_CYAN);
  spidma_ili9341_write_string(spi, 0, 0, "CYAN", Font_11x18, ILI9341_BLACK, ILI9341_CYAN);
  spidma_empty_queue(spi);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_MAGENTA);
  spidma_ili9341_write_string(spi, 0, 0, "MAGENTA", Font_11x18, ILI9341_BLACK, ILI9341_MAGENTA);
  spidma_empty_queue(spi);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_YELLOW);
  spidma_ili9341_write_string(spi, 0, 0, "YELLOW", Font_11x18, ILI9341_BLACK, ILI9341_YELLOW);
  spidma_empty_queue(spi);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_BLACK);
  spidma_ili9341_write_string(spi, 0, 0, "BLACK", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
  spidma_empty_queue(spi);
  HAL_Delay(1000);

  console_printf("Drawing image...\r\n");
  // Last argument is to copy the data before sending it.
  // If the data is coming from flash, we should do that.
  spidma_ili9341_draw_image(spi, (ILI9341_WIDTH - 240) / 2, (ILI9341_HEIGHT - 240) / 2, 240, 240, (const uint16_t*)test_img_240x240, 1);
  spidma_empty_queue(spi);
  console_printf("...done.\r\n");
  HAL_Delay(1500);

#ifdef INCLUDE_TOUCH
  ILI9341_FillScreen(ILI9341_BLACK);
  ILI9341_WriteString(0, 0, "Touchpad test.  Draw something!", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
  HAL_Delay(1000);
  ILI9341_FillScreen(ILI9341_BLACK);

  int npoints = 0;
  while(npoints < 10000) {
      uint16_t x, y;

      if(ILI9341_TouchGetCoordinates(&x, &y)) {
          ILI9341_DrawPixel(x, 320-y, ILI9341_WHITE);
          npoints++;
      }
  }
#endif

  spidma_queue(spi, SPIDMA_DESELECT, 0, NULL, 1000100);
  spidma_empty_queue(spi);
  console_printf("loop() done\r\n");
}

// Main program for an SPI demo
void spimain(void) {
  // Configure new DMA driver
  DISPLAY_SPI.bank_cs = GPIO_PA15_SPI2_CS_GPIO_Port;
  DISPLAY_SPI.pin_cs = GPIO_PA15_SPI2_CS_Pin;
  DISPLAY_SPI.bank_dc = GPIO_PB8_SPI2_DC_GPIO_Port;
  DISPLAY_SPI.pin_dc = GPIO_PB8_SPI2_DC_Pin;
  DISPLAY_SPI.bank_reset = GPIO_PB5_SPI2_RESET_GPIO_Port;
  DISPLAY_SPI.pin_reset = GPIO_PB5_SPI2_RESET_Pin;
  DISPLAY_SPI.use_cs = 1;
  DISPLAY_SPI.use_reset = 1;
  DISPLAY_SPI.spi = &ILI9341_SPI_PORT;
  DISPLAY_SPI.dma_tx = &DISPLAY_DMA;

  if (spidma_init(DISPLAY_SPIP) != SDRV_OK) {
    console_printf("spidma_init failed\r\n");
    while (1) {}
  }

  console_printf("Starting ILI9341 init...\r\n");
  spidma_ili9341_init(DISPLAY_SPIP);
  spidma_empty_queue(DISPLAY_SPIP);

  console_printf("Starting loop...\r\n");
  while (1) {
    loop(DISPLAY_SPIP);
  }
}
