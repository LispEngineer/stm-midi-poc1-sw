/*
 * Adapted from https://github.com/afiskon/stm32-ili9341
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


static void UART_Printf(const char* fmt, ...) {
    char buff[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, sizeof(buff), fmt, args);
    HAL_UART_Transmit(&CONSOLE_UART, (uint8_t*)buff, strlen(buff),
                      HAL_MAX_DELAY);
    va_end(args);
}


static void loop(spidma_config_t *spi) {
  // Draw black screen with red border
  spidma_ili9341_fill_screen(spi, ILI9341_BLACK);

  for(int x = 0; x < ILI9341_WIDTH; x++) {
    spidma_ili9341_draw_pixel(spi, x, 0, ILI9341_RED);
    spidma_ili9341_draw_pixel(spi, x, ILI9341_HEIGHT-1, ILI9341_RED);
  }

  for(int y = 0; y < ILI9341_HEIGHT; y++) {
    spidma_ili9341_draw_pixel(spi, 0, y, ILI9341_RED);
    spidma_ili9341_draw_pixel(spi, ILI9341_WIDTH-1, y, ILI9341_RED);
  }
  HAL_Delay(1000);

  // Check fonts
  spidma_ili9341_fill_screen(spi, ILI9341_BLACK);
  spidma_ili9341_write_string(spi, 0, 0, "Font_7x10, red on black, lorem ipsum dolor sit amet", Font_7x10, ILI9341_RED, ILI9341_BLACK);
  spidma_ili9341_write_string(spi, 0, 3*10, "Font_11x18, green, lorem ipsum dolor sit amet", Font_11x18, ILI9341_GREEN, ILI9341_BLACK);
  spidma_ili9341_write_string(spi, 0, 3*10+3*18, "Font_16x26, blue, lorem ipsum dolor sit amet", Font_16x26, ILI9341_BLUE, ILI9341_BLACK);
  HAL_Delay(500);

  spidma_ili9341_invert(spi, true);
  HAL_Delay(500);
  spidma_ili9341_invert(spi, false);
  HAL_Delay(1000);

  // Check colors
  spidma_ili9341_fill_screen(spi, ILI9341_WHITE);
  spidma_ili9341_write_string(spi, 0, 0, "WHITE", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_BLUE);
  spidma_ili9341_write_string(spi, 0, 0, "BLUE", Font_11x18, ILI9341_BLACK, ILI9341_BLUE);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_RED);
  spidma_ili9341_write_string(spi, 0, 0, "RED", Font_11x18, ILI9341_BLACK, ILI9341_RED);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_GREEN);
  spidma_ili9341_write_string(spi, 0, 0, "GREEN", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_CYAN);
  spidma_ili9341_write_string(spi, 0, 0, "CYAN", Font_11x18, ILI9341_BLACK, ILI9341_CYAN);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_MAGENTA);
  spidma_ili9341_write_string(spi, 0, 0, "MAGENTA", Font_11x18, ILI9341_BLACK, ILI9341_MAGENTA);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_YELLOW);
  spidma_ili9341_write_string(spi, 0, 0, "YELLOW", Font_11x18, ILI9341_BLACK, ILI9341_YELLOW);
  HAL_Delay(500);

  spidma_ili9341_fill_screen(spi, ILI9341_BLACK);
  spidma_ili9341_write_string(spi, 0, 0, "BLACK", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
  HAL_Delay(1000);

  UART_Printf("Drawing image...\r\n");
  // Last argument is to copy the data before sending it.
  // If the data is coming from flash, we should do that.
  spidma_ili9341_draw_image(spi, (ILI9341_WIDTH - 240) / 2, (ILI9341_HEIGHT - 240) / 2, 240, 240, (const uint16_t*)test_img_240x240, 1);
  UART_Printf("...done.\r\n");
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

  UART_Printf("loop() done\r\n");
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
  DISPLAY_SPI.synchronous = 1; // Fake synchronous DMA
  DISPLAY_SPI.dma_tx = &DISPLAY_DMA;
  spidma_init(DISPLAY_SPIP);

  UART_Printf("Starting init...\r\n");
  spidma_ili9341_init(DISPLAY_SPIP);

  UART_Printf("Starting loop...\r\n");
  while (1) {
    loop(DISPLAY_SPIP);
  }
}
