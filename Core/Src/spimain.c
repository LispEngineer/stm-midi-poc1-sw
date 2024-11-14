/*
 * Adapted from https://github.com/afiskon/stm32-ili9341
 */


/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "main.h"
#include "ili9341.h"
#ifdef INCLUDE_TOUCH
#include "ili9341_touch.h"
#endif // INCLUDE_TOUCH
#include "fonts.h"
#include "testimg.h"
#include "spidma.h"


// From main
#define CONSOLE_UART huart2  // EVT#1 Console Port
extern UART_HandleTypeDef CONSOLE_UART;

#define DISPLAY_SPI spi2tx
extern spidma_config_t DISPLAY_SPI;

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

static void init() {
    ILI9341_Unselect();
#ifdef INCLUDE_TOUCH
    ILI9341_TouchUnselect();
#endif
    ILI9341_Init();
}

uint8_t init_0[]   = { 0x01 }; // Command SOFTWARE RESET
uint8_t init_1[]   = { 0xCB }; // Command POWER CONTROL A
uint8_t init_1d[]  = { 0x39, 0x2C, 0x00, 0x34, 0x02 }; // Data for above
uint8_t init_2[]   = { 0xCF }; // POWER CONTROL B
uint8_t init_2d[]  = { 0x00, 0xC1, 0x30 };
uint8_t init_3[]   = { 0xE8 }; // DRIVER TIMING CONTROL A
uint8_t init_3d[]  = { 0x85, 0x00, 0x78 };
uint8_t init_4[]   = { 0xEA }; // DRIVER TIMING CONTROL B
uint8_t init_4d[]  = { 0x00, 0x00 };
uint8_t init_5[]   = { 0xED }; // POWER ON SEQUENCE CONTROL
uint8_t init_5d[]  = { 0x64, 0x03, 0x12, 0x81 };
uint8_t init_6[]   = { 0xF7 }; // PUMP RATIO CONTROL
uint8_t init_6d[]  = { 0x20 };
uint8_t init_7[]   = { 0xC0 }; // POWER CONTROL,VRH[5:0]
uint8_t init_7d[]  = { 0x23 };
uint8_t init_8[]   = { 0xC1 }; // POWER CONTROL,SAP[2:0];BT[3:0
uint8_t init_8d[]  = { 0x10 };
uint8_t init_9[]   = { 0xC5 }; // VCM CONTROL
uint8_t init_9d[]  = { 0x3E, 0x28 };
uint8_t init_[]   = {  };
uint8_t init_d[]  = {  };
uint8_t init_[]   = {  };
uint8_t init_d[]  = {  };
uint8_t init_[]   = {  };
uint8_t init_d[]  = {  };
uint8_t init_[]   = {  };
uint8_t init_d[]  = {  };


static void spidma_ili9341_init() {
  // Queue all the commands to do a full reset
  spidma_queue(DISPLAY_SPI, SPIDMA_DESELECT, 0, 0, 100);
  spidma_queue(DISPLAY_SPI, SPIDMA_SELECT, 0, 0, 200);

  spidma_queue(DISPLAY_SPI, SPIDMA_RESET, 0, 0, 300);
  spidma_queue(DISPLAY_SPI, SPIDMA_DELAY, 5 + 1, 0, 400); // Wait 5 milliseconds
  spidma_queue(DISPLAY_SPI, SPIDMA_UNRESET, 0, 0, 500);

  // Display initialization based upon https://github.com/afiskon/stm32-ili9341

  // SOFTWARE RESET
  spidma_queue(DISPLAY_SPI, SPIDMA_COMMAND, sizeof(init_0), init_0, 600);
  spidma_queue(DISPLAY_SPI, SPIDMA_DELAY, 1000 + 1, 0, 700); // Wait 1000 milliseconds

  // POWER CONTROL A
  spidma_queue(DISPLAY_SPI, SPIDMA_COMMAND, sizeof(init_1), init_1, 800);
  spidma_queue(DISPLAY_SPI, SPIDMA_DATA, sizeof(init_1d), init_1d, 900);

  // POWER CONTROL B
  spidma_queue(DISPLAY_SPI, SPIDMA_COMMAND, sizeof(init_2), init_2, 1000);
  spidma_queue(DISPLAY_SPI, SPIDMA_DATA, sizeof(init_2d), init_2d, 1100);

  // DRIVER TIMING CONTROL A
  spidma_queue(DISPLAY_SPI, SPIDMA_COMMAND, sizeof(init_3), init_3, 1200);
  spidma_queue(DISPLAY_SPI, SPIDMA_DATA, sizeof(init_3d), init_3d, 1300);

  // DRIVER TIMING CONTROL B
  spidma_queue(DISPLAY_SPI, SPIDMA_COMMAND, sizeof(init_4), init_4, 1400);
  spidma_queue(DISPLAY_SPI, SPIDMA_DATA, sizeof(init_4d), init_4d, 1500);

  // POWER ON SEQUENCE CONTROL
  spidma_queue(DISPLAY_SPI, SPIDMA_COMMAND, sizeof(init_5), init_5, 1600);
  spidma_queue(DISPLAY_SPI, SPIDMA_DATA, sizeof(init_5d), init_5d, 1700);

  // PUMP RATIO CONTROL
  ILI9341_WriteCommand(0xF7);
  {
      uint8_t data[] = { 0x20 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // POWER CONTROL,VRH[5:0]
  ILI9341_WriteCommand(0xC0);
  {
      uint8_t data[] = { 0x23 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // POWER CONTROL,SAP[2:0];BT[3:0]
  ILI9341_WriteCommand(0xC1);
  {
      uint8_t data[] = { 0x10 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // VCM CONTROL
  ILI9341_WriteCommand(0xC5);
  {
      uint8_t data[] = { 0x3E, 0x28 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // VCM CONTROL 2
  ILI9341_WriteCommand(0xC7);
  {
      uint8_t data[] = { 0x86 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // MEMORY ACCESS CONTROL
  ILI9341_WriteCommand(0x36);
  {
      uint8_t data[] = { 0x48 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // PIXEL FORMAT
  ILI9341_WriteCommand(0x3A);
  {
      uint8_t data[] = { 0x55 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // FRAME RATIO CONTROL, STANDARD RGB COLOR
  ILI9341_WriteCommand(0xB1);
  {
      uint8_t data[] = { 0x00, 0x18 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // DISPLAY FUNCTION CONTROL
  ILI9341_WriteCommand(0xB6);
  {
      uint8_t data[] = { 0x08, 0x82, 0x27 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // 3GAMMA FUNCTION DISABLE
  ILI9341_WriteCommand(0xF2);
  {
      uint8_t data[] = { 0x00 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // GAMMA CURVE SELECTED
  ILI9341_WriteCommand(0x26);
  {
      uint8_t data[] = { 0x01 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // POSITIVE GAMMA CORRECTION
  ILI9341_WriteCommand(0xE0);
  {
      uint8_t data[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                         0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
      ILI9341_WriteData(data, sizeof(data));
  }

  // NEGATIVE GAMMA CORRECTION
  ILI9341_WriteCommand(0xE1);
  {
      uint8_t data[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                         0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
      ILI9341_WriteData(data, sizeof(data));
  }

  // EXIT SLEEP
  ILI9341_WriteCommand(0x11);
  HAL_Delay(120);

  // TURN ON DISPLAY
  ILI9341_WriteCommand(0x29);

  // MADCTL
  ILI9341_WriteCommand(0x36);
  {
      uint8_t data[] = { ILI9341_ROTATION };
      ILI9341_WriteData(data, sizeof(data));
  }

  ILI9341_Unselect();

}

static void loop() {
  // Not sure why the original demo code kept DeInit/Init'ing the SPI
  /*
    if(HAL_SPI_DeInit(&ILI9341_SPI_PORT) != HAL_OK) {
        UART_Printf("HAL_SPI_DeInit failed 1!\r\n");
        return;
    }

    ILI9341_SPI_PORT.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;

    if(HAL_SPI_Init(&ILI9341_SPI_PORT) != HAL_OK) {
        UART_Printf("HAL_SPI_Init failed 1!\r\n");
        return;
    }
    */

    // Check border
    ILI9341_FillScreen(ILI9341_BLACK);

    for(int x = 0; x < ILI9341_WIDTH; x++) {
        ILI9341_DrawPixel(x, 0, ILI9341_RED);
        ILI9341_DrawPixel(x, ILI9341_HEIGHT-1, ILI9341_RED);
    }

    for(int y = 0; y < ILI9341_HEIGHT; y++) {
        ILI9341_DrawPixel(0, y, ILI9341_RED);
        ILI9341_DrawPixel(ILI9341_WIDTH-1, y, ILI9341_RED);
    }

    HAL_Delay(3000);

    // Check fonts
    ILI9341_FillScreen(ILI9341_BLACK);
    ILI9341_WriteString(0, 0, "Font_7x10, red on black, lorem ipsum dolor sit amet", Font_7x10, ILI9341_RED, ILI9341_BLACK);
    ILI9341_WriteString(0, 3*10, "Font_11x18, green, lorem ipsum dolor sit amet", Font_11x18, ILI9341_GREEN, ILI9341_BLACK);
    ILI9341_WriteString(0, 3*10+3*18, "Font_16x26, blue, lorem ipsum dolor sit amet", Font_16x26, ILI9341_BLUE, ILI9341_BLACK);

    HAL_Delay(1000);
    ILI9341_InvertColors(true);
    HAL_Delay(1000);
    ILI9341_InvertColors(false);

    HAL_Delay(5000);

    // Check colors
    ILI9341_FillScreen(ILI9341_WHITE);
    ILI9341_WriteString(0, 0, "WHITE", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
    HAL_Delay(500);

    ILI9341_FillScreen(ILI9341_BLUE);
    ILI9341_WriteString(0, 0, "BLUE", Font_11x18, ILI9341_BLACK, ILI9341_BLUE);
    HAL_Delay(500);

    ILI9341_FillScreen(ILI9341_RED);
    ILI9341_WriteString(0, 0, "RED", Font_11x18, ILI9341_BLACK, ILI9341_RED);
    HAL_Delay(500);

    ILI9341_FillScreen(ILI9341_GREEN);
    ILI9341_WriteString(0, 0, "GREEN", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);
    HAL_Delay(500);

    ILI9341_FillScreen(ILI9341_CYAN);
    ILI9341_WriteString(0, 0, "CYAN", Font_11x18, ILI9341_BLACK, ILI9341_CYAN);
    HAL_Delay(500);

    ILI9341_FillScreen(ILI9341_MAGENTA);
    ILI9341_WriteString(0, 0, "MAGENTA", Font_11x18, ILI9341_BLACK, ILI9341_MAGENTA);
    HAL_Delay(500);

    ILI9341_FillScreen(ILI9341_YELLOW);
    ILI9341_WriteString(0, 0, "YELLOW", Font_11x18, ILI9341_BLACK, ILI9341_YELLOW);
    HAL_Delay(500);

    ILI9341_FillScreen(ILI9341_BLACK);
    ILI9341_WriteString(0, 0, "BLACK", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    HAL_Delay(500);

    UART_Printf("Drawing image...\r\n");
    ILI9341_DrawImage((ILI9341_WIDTH - 240) / 2, (ILI9341_HEIGHT - 240) / 2, 240, 240, (const uint16_t*)test_img_240x240);
    UART_Printf("...done.\r\n");
    HAL_Delay(3000);

#ifdef INCLUDE_TOUCH
    ILI9341_FillScreen(ILI9341_BLACK);
    ILI9341_WriteString(0, 0, "Touchpad test.  Draw something!", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    HAL_Delay(1000);
    ILI9341_FillScreen(ILI9341_BLACK);
#endif

    // Not sure why the original demo code kept DeInit/Init'ing the SPI
    /*
    if(HAL_SPI_DeInit(&ILI9341_SPI_PORT) != HAL_OK) {
        UART_Printf("HAL_SPI_DeInit failed 2!\r\n");
        return;
    }

    ILI9341_SPI_PORT.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;

    if(HAL_SPI_Init(&ILI9341_SPI_PORT) != HAL_OK) {
        UART_Printf("HAL_SPI_Init failed 2!\r\n");
        return;
    }
    */

#ifdef INCLUDE_TOUCH
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

#define ILI9341_RES_Pin       GPIO_PB5_SPI2_RESET_Pin
#define ILI9341_RES_GPIO_Port GPIO_PB5_SPI2_RESET_GPIO_Port
#define ILI9341_CS_Pin        GPIO_PA15_SPI2_CS_Pin
#define ILI9341_CS_GPIO_Port  GPIO_PA15_SPI2_CS_GPIO_Port
#define ILI9341_DC_Pin        GPIO_PB8_SPI2_DC_Pin
#define ILI9341_DC_GPIO_Port  GPIO_PB8_SPI2_DC_GPIO_Port


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
  spidma_init(&DISPLAY_SPI);

  UART_Printf("Starting init...\r\n");
  init();

  UART_Printf("Starting loop...\r\n");
  while (1) {
    loop();
  }
}
