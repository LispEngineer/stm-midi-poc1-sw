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
#define DISPLAY_SPIP (&(DISPLAY_SPI))
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
uint8_t init_10[]   = { 0xC7 }; // VCM CONTROL 2
uint8_t init_10d[]  = { 0x86 };
uint8_t init_11[]   = { 0x36 }; // MEMORY ACCESS CONTROL
uint8_t init_11d[]  = { 0x48 };
uint8_t init_12[]   = { 0x3A }; // PIXEL FORMAT
uint8_t init_12d[]  = { 0x55 };
uint8_t init_13[]   = { 0xB1 }; // FRAME RATIO CONTROL, STANDARD RGB COLOR
uint8_t init_13d[]  = { 0x00, 0x18 };
uint8_t init_14[]   = { 0xB6 }; // DISPLAY FUNCTION CONTROL
uint8_t init_14d[]  = { 0x08, 0x82, 0x27 };
uint8_t init_15[]   = { 0xF2 }; // 3GAMMA FUNCTION DISABLE
uint8_t init_15d[]  = { 0x00 };
uint8_t init_16[]   = { 0x26 }; // GAMMA CURVE SELECTED
uint8_t init_16d[]  = { 0x01 };
uint8_t init_17[]   = { 0xE0 }; // POSITIVE GAMMA CORRECTION
uint8_t init_17d[]  = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                        0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
uint8_t init_18[]   = { 0xE1 }; // NEGATIVE GAMMA CORRECTION
uint8_t init_18d[]  = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                        0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
uint8_t init_19[]   = { 0x11 }; // EXIT SLEEP
uint8_t init_20[]   = { 0x29 }; // TURN ON DISPLAY
uint8_t init_21[]   = { 0x36 }; // MADCTL
uint8_t init_21d[]  = { ILI9341_ROTATION };

/*
uint8_t init_[]   = {  };
uint8_t init_d[]  = {  };
*/

static void spidma_ili9341_init(spidma_config_t *spi) {
  // Queue all the commands to do a full reset
  spidma_queue(spi, SPIDMA_DESELECT, 0, 0, 100);
  spidma_queue(spi, SPIDMA_SELECT, 0, 0, 200);

  spidma_queue(spi, SPIDMA_RESET, 0, 0, 300);
  spidma_queue(spi, SPIDMA_DELAY, 5 + 1, 0, 400); // Wait 5 milliseconds
  spidma_queue(spi, SPIDMA_UNRESET, 0, 0, 500);

  // Display initialization based upon https://github.com/afiskon/stm32-ili9341

  // SOFTWARE RESET
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_0), init_0, 600);
  spidma_queue(spi, SPIDMA_DELAY, 1000 + 1, 0, 700); // Wait 1000 milliseconds

  // POWER CONTROL A
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_1), init_1, 800);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_1d), init_1d, 900);

  // POWER CONTROL B
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_2), init_2, 1000);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_2d), init_2d, 1100);

  // DRIVER TIMING CONTROL A
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_3), init_3, 1200);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_3d), init_3d, 1300);

  // DRIVER TIMING CONTROL B
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_4), init_4, 1400);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_4d), init_4d, 1500);

  // POWER ON SEQUENCE CONTROL
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_5), init_5, 1600);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_5d), init_5d, 1700);

  // PUMP RATIO CONTROL
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_6), init_6, 1800);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_6d), init_6d, 1900);

  // POWER CONTROL,VRH[5:0]
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_7), init_7, 2000);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_7d), init_7d, 2100);

  // POWER CONTROL,SAP[2:0];BT[3:0]
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_8), init_8, 2200);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_8d), init_8d, 2300);

  // VCM CONTROL
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_9), init_9, 2400);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_9d), init_9d, 2500);

  // VCM CONTROL 2
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_10), init_10, 2600);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_10d), init_10d, 2700);

  // MEMORY ACCESS CONTROL
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_11), init_11, 2800);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_11d), init_11d, 2900);

  // PIXEL FORMAT
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_12), init_12, 3000);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_12d), init_12d, 3100);

  // FRAME RATIO CONTROL, STANDARD RGB COLOR
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_13), init_13, 3200);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_13d), init_13d, 3300);

  // DISPLAY FUNCTION CONTROL
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_14), init_14, 3400);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_14d), init_14d, 3500);

  // 3GAMMA FUNCTION DISABLE
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_15), init_15, 3600);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_15d), init_15d, 3700);

  // GAMMA CURVE SELECTED
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_16), init_16, 3800);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_16d), init_16d, 3900);

  // POSITIVE GAMMA CORRECTION
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_17), init_17, 4000);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_17d), init_17d, 4100);

  // NEGATIVE GAMMA CORRECTION
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_18), init_18, 4200);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_18d), init_18d, 4300);

  // EXIT SLEEP
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_19), init_19, 4400);
  spidma_queue(spi, SPIDMA_DELAY, 120 + 1, 0, 4500); // Wait 120 milliseconds

  // TURN ON DISPLAY
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_20), init_20, 4500);

  // MADCTL
  spidma_queue(spi, SPIDMA_COMMAND, sizeof(init_21), init_21, 4600);
  spidma_queue(spi, SPIDMA_DATA, sizeof(init_21d), init_21d, 4700);

  spidma_queue(spi, SPIDMA_DESELECT, 0, 0, 4800);

  // Run the queue until it's empty
  while (spidma_check_activity(spi) != 0); // 0 = nothing to do
}


static uint8_t saw_col_cmd;
static uint8_t saw_row_cmd;
static uint8_t saw_write_cmd;
static uint8_t saw_col_buff[4];
static uint8_t saw_row_buff[4];

// Hacky DMA synchronous version
void spidma_ili9341_set_address_window(spidma_config_t *spi, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

  saw_col_buff[0] = (x0 >> 8) & 0xFF;
  saw_col_buff[1] = x0 & 0xFF;
  saw_col_buff[2] = (x1 >> 8) & 0xFF;
  saw_col_buff[3] = x1 & 0xFF;
  saw_row_buff[0] = (y0 >> 8) & 0xFF;
  saw_row_buff[1] = y0 & 0xFF;
  saw_row_buff[2] = (y1 >> 8) & 0xFF;
  saw_row_buff[3] = y1 & 0xFF;
  saw_col_cmd = 0x2A;
  saw_row_cmd = 0x2B;
  saw_write_cmd = 0x2C;

  // column address set - CASET
  spidma_queue(spi, SPIDMA_COMMAND, 1, &saw_col_cmd, 10000);
  spidma_queue(spi, SPIDMA_DATA, sizeof(saw_col_buff), saw_col_buff, 10010);

  // row address set - RASET
  spidma_queue(spi, SPIDMA_COMMAND, 1, &saw_row_cmd, 10020);
  spidma_queue(spi, SPIDMA_DATA, sizeof(saw_row_buff), saw_row_buff, 10030);

  // write to RAM
  spidma_queue(spi, SPIDMA_COMMAND, 1, &saw_write_cmd, 10040);

  // Run the queue until it's empty
  while (spidma_check_activity(spi) != 0); // 0 = nothing to do
}

#define FILL_RECT_BUFF_SZ 1024
#define FILL_RECT_BUFF_SHIFT 10
static uint16_t fill_rect_buff[FILL_RECT_BUFF_SZ];

void spidma_ili9341_fill_rectangle(spidma_config_t *spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  // clipping
  if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
  if((x + w - 1) >= ILI9341_WIDTH) w = ILI9341_WIDTH - x;
  if((y + h - 1) >= ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

  spidma_queue(spi, SPIDMA_SELECT, 0, 0, 20000);
  spidma_ili9341_set_address_window(spi, x, y, x+w-1, y+h-1);

  size_t pos = 0;
  size_t end_pos = w * h;
  // We need to flip the bytes of the color we are setting
  uint16_t inv_color = ((color & 0xFF) << 8) | (color >> 8);
  uint8_t repeats;
  size_t remainder;

  size_t buff_end = end_pos > FILL_RECT_BUFF_SZ ? FILL_RECT_BUFF_SZ : end_pos;

  // Set everything we're sending (up to buffer size) first
  for (pos = 0; pos < buff_end; pos++) {
      fill_rect_buff[pos] = inv_color;
  }
  // Calculate the # of repeats
  repeats = end_pos >> FILL_RECT_BUFF_SHIFT; // 1500 / 1024 = 1

  // Add all the repeats, if any
  if (repeats > 0) {
    remainder = end_pos - (repeats << FILL_RECT_BUFF_SHIFT);
    spidma_queue_repeats(spi, SPIDMA_DATA, FILL_RECT_BUFF_SZ * 2, (uint8_t *)fill_rect_buff, 20010, repeats - 1);
    end_pos = remainder;
  }
  if (end_pos > 0) {
    spidma_queue(spi, SPIDMA_DATA, end_pos * 2, (uint8_t *)fill_rect_buff, 20020);
  }

  spidma_queue(spi, SPIDMA_DESELECT, 0, 0, 20030);

  // Run the queue until it's empty
  while (spidma_check_activity(spi) != 0); // 0 = nothing to do
}

void spidma_ili9341_fill_screen(spidma_config_t *spi, uint16_t color) {
  spidma_ili9341_fill_rectangle(spi, 0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}


static void loop() {
    // Check border
    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_BLACK);

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
    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_BLACK);
    ILI9341_WriteString(0, 0, "Font_7x10, red on black, lorem ipsum dolor sit amet", Font_7x10, ILI9341_RED, ILI9341_BLACK);
    ILI9341_WriteString(0, 3*10, "Font_11x18, green, lorem ipsum dolor sit amet", Font_11x18, ILI9341_GREEN, ILI9341_BLACK);
    ILI9341_WriteString(0, 3*10+3*18, "Font_16x26, blue, lorem ipsum dolor sit amet", Font_16x26, ILI9341_BLUE, ILI9341_BLACK);

    HAL_Delay(1000);
    ILI9341_InvertColors(true);
    HAL_Delay(1000);
    ILI9341_InvertColors(false);

    HAL_Delay(5000);

    // Check colors
    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_WHITE);
    ILI9341_WriteString(0, 0, "WHITE", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
    HAL_Delay(500);

    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_BLUE);
    ILI9341_WriteString(0, 0, "BLUE", Font_11x18, ILI9341_BLACK, ILI9341_BLUE);
    HAL_Delay(500);

    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_RED);
    ILI9341_WriteString(0, 0, "RED", Font_11x18, ILI9341_BLACK, ILI9341_RED);
    HAL_Delay(500);

    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_GREEN);
    ILI9341_WriteString(0, 0, "GREEN", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);
    HAL_Delay(500);

    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_CYAN);
    ILI9341_WriteString(0, 0, "CYAN", Font_11x18, ILI9341_BLACK, ILI9341_CYAN);
    HAL_Delay(500);

    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_MAGENTA);
    ILI9341_WriteString(0, 0, "MAGENTA", Font_11x18, ILI9341_BLACK, ILI9341_MAGENTA);
    HAL_Delay(500);

    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_YELLOW);
    ILI9341_WriteString(0, 0, "YELLOW", Font_11x18, ILI9341_BLACK, ILI9341_YELLOW);
    HAL_Delay(500);

    spidma_ili9341_fill_screen(DISPLAY_SPIP, ILI9341_BLACK);
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
  spidma_init(DISPLAY_SPIP);

  UART_Printf("Starting init...\r\n");
  spidma_ili9341_init(DISPLAY_SPIP);

  UART_Printf("Starting loop...\r\n");
  while (1) {
    loop();
  }
}
