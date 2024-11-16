/*
 * spidma_ili9341.c
 *
 * Commands to control an ILI9341 SPI display, sent using
 * my SPIDMA queueing mechanism.
 *
 *  Created on: 2024-11-14
 *  Updated on: 2024-11-14
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 *
 * Adapted from https://github.com/afiskon/stm32-ili9341
 */
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "spidma.h"
#include "fonts.h"
#include "spidma_ili9341.h"

static uint8_t init_0[]   = { 0x01 }; // Command SOFTWARE RESET
static uint8_t init_1[]   = { 0xCB }; // Command POWER CONTROL A
static uint8_t init_1d[]  = { 0x39, 0x2C, 0x00, 0x34, 0x02 }; // Data for above
static uint8_t init_2[]   = { 0xCF }; // POWER CONTROL B
static uint8_t init_2d[]  = { 0x00, 0xC1, 0x30 };
static uint8_t init_3[]   = { 0xE8 }; // DRIVER TIMING CONTROL A
static uint8_t init_3d[]  = { 0x85, 0x00, 0x78 };
static uint8_t init_4[]   = { 0xEA }; // DRIVER TIMING CONTROL B
static uint8_t init_4d[]  = { 0x00, 0x00 };
static uint8_t init_5[]   = { 0xED }; // POWER ON SEQUENCE CONTROL
static uint8_t init_5d[]  = { 0x64, 0x03, 0x12, 0x81 };
static uint8_t init_6[]   = { 0xF7 }; // PUMP RATIO CONTROL
static uint8_t init_6d[]  = { 0x20 };
static uint8_t init_7[]   = { 0xC0 }; // POWER CONTROL,VRH[5:0]
static uint8_t init_7d[]  = { 0x23 };
static uint8_t init_8[]   = { 0xC1 }; // POWER CONTROL,SAP[2:0];BT[3:0
static uint8_t init_8d[]  = { 0x10 };
static uint8_t init_9[]   = { 0xC5 }; // VCM CONTROL
static uint8_t init_9d[]  = { 0x3E, 0x28 };
static uint8_t init_10[]   = { 0xC7 }; // VCM CONTROL 2
static uint8_t init_10d[]  = { 0x86 };
static uint8_t init_11[]   = { 0x36 }; // MEMORY ACCESS CONTROL
static uint8_t init_11d[]  = { 0x48 };
static uint8_t init_12[]   = { 0x3A }; // PIXEL FORMAT
static uint8_t init_12d[]  = { 0x55 };
static uint8_t init_13[]   = { 0xB1 }; // FRAME RATIO CONTROL, STANDARD RGB COLOR
static uint8_t init_13d[]  = { 0x00, 0x18 };
static uint8_t init_14[]   = { 0xB6 }; // DISPLAY FUNCTION CONTROL
static uint8_t init_14d[]  = { 0x08, 0x82, 0x27 };
static uint8_t init_15[]   = { 0xF2 }; // 3GAMMA FUNCTION DISABLE
static uint8_t init_15d[]  = { 0x00 };
static uint8_t init_16[]   = { 0x26 }; // GAMMA CURVE SELECTED
static uint8_t init_16d[]  = { 0x01 };
static uint8_t init_17[]   = { 0xE0 }; // POSITIVE GAMMA CORRECTION
static uint8_t init_17d[]  = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                                0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
static uint8_t init_18[]   = { 0xE1 }; // NEGATIVE GAMMA CORRECTION
static uint8_t init_18d[]  = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                                0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
static uint8_t init_19[]   = { 0x11 }; // EXIT SLEEP
static uint8_t init_20[]   = { 0x29 }; // TURN ON DISPLAY
static uint8_t init_21[]   = { 0x36 }; // MADCTL
static uint8_t init_21d[]  = { ILI9341_ROTATION };

/*
static uint8_t init_[]   = {  };
static uint8_t init_d[]  = {  };
*/

void spidma_ili9341_init(spidma_config_t *spi) {
  // Queue all the commands to do a full reset
  spidma_queue(spi, SPIDMA_DESELECT, 0, 0, 100);
#ifdef INCLUDE_TOUCH
  ILI9341_TouchUnselect();
#endif
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
  if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
  if ((x + w - 1) >= ILI9341_WIDTH) w = ILI9341_WIDTH - x;
  if ((y + h - 1) >= ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

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
    // Final 0 means "don't auto-free this buffer"
    spidma_queue_repeats(spi, SPIDMA_DATA, FILL_RECT_BUFF_SZ * 2, (uint8_t *)fill_rect_buff, 20010, repeats - 1, 0);
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

void spidma_ili9341_draw_pixel(spidma_config_t *spi, uint16_t x, uint16_t y, uint16_t color) {
  if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT))
      return;

  spidma_ili9341_fill_rectangle(spi, x, y, 1, 1, color);
}

static uint16_t char_buf[FILL_RECT_BUFF_SZ];

void spidma_ili9341_write_char(spidma_config_t *spi, uint16_t x, uint16_t y,
                                  char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
  uint32_t i, b, j;
  size_t pos = 0;

  if (((font.width * font.height) >> FILL_RECT_BUFF_SHIFT) > 0) {
    // Font too big for our buffer, oh well
    return;
  }

  // We need to flip the bytes of the color we are setting
  color = ((color & 0xFF) << 8) | (color >> 8);
  bgcolor = ((bgcolor & 0xFF) << 8) | (bgcolor >> 8);

  // Prepare to blast our character out in a single write
  for (i = 0; i < font.height; i++) {
    b = font.data[(ch - 32) * font.height + i];
    for (j = 0; j < font.width; j++) {
      char_buf[pos] = ((b << j) & 0x8000) ? color : bgcolor;
      pos++;
    }
  }

  spidma_ili9341_set_address_window(spi, x, y, x + font.width - 1, y + font.height - 1);
  spidma_queue(spi, SPIDMA_DATA, pos * 2, (uint8_t *)char_buf, 30000);

  // Run the queue until it's empty
  while (spidma_check_activity(spi) != 0); // 0 = nothing to do
}


/*
 * Returns next drawing position:
 * y in the top 16 bits
 * x in the bottom 16 bits
 *
 * It may be that the next drawing position would not actually fit
 * fully on the screen - the caller needs to check that
 * (new y + font.height > screen height) means off the screen
 */
uint32_t spidma_ili9341_write_string(spidma_config_t *spi, uint16_t x, uint16_t y,
                                        const char *str, FontDef font, uint16_t color, uint16_t bgcolor) {
  spidma_queue(spi, SPIDMA_SELECT, 0, 0, 40000);

  while (*str) {
    if (x + font.width >= ILI9341_WIDTH) {
      x = 0;
      y += font.height;
      if (y + font.height >= ILI9341_HEIGHT) {
        // Running off the screen - so just stop drawing
        break;
      }

      if (*str == ' ') {
        // skip spaces in the beginning of the new line
        str++;
        continue;
      }
    }

    spidma_ili9341_write_char(spi, x, y, *str, font, color, bgcolor);
    x += font.width;
    str++;
  }

  spidma_queue(spi, SPIDMA_DESELECT, 0, 0, 40000);

  // Run the queue until it's empty
  while (spidma_check_activity(spi) != 0); // 0 = nothing to do

  // Return the next character position - which may be off the screen
  return (y << 16) | x;
}


// TODO: Get rid of this HUGE memory suck
static uint16_t image_buffer[320*240];
static const size_t chunk_size = 65000;

/*
 * OLD:
 * data must be a pointer into DMA-from-able memory.
 * This does NOT seem to include Flash in STM32F7.
 */
void spidma_ili9341_draw_image(spidma_config_t *spi, uint16_t x, uint16_t y,
                                  uint16_t w, uint16_t h, const uint16_t* data,
                                  uint32_t copy_data) {
  // Limits check
  if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT))
    return;
  if ((x + w - 1) >= ILI9341_WIDTH)
    return;
  if ((y + h - 1) >= ILI9341_HEIGHT)
    return;

  const uint8_t *source;

  // This was not working via DMA when drawing the data directly out of Flash, so
  // we have to copy it to main memory where DMA can transfer from
  size_t data_bytes = sizeof(uint16_t) * w * h;
  if (copy_data) {
    memcpy(image_buffer, data, data_bytes);
    source = (uint8_t *)image_buffer;
  } else {
    source = (uint8_t *)data;
  }

  spidma_queue(spi, SPIDMA_SELECT, 0, 0, 50000);
  spidma_ili9341_set_address_window(spi, x, y, x + w - 1, y + h - 1);

  // We can only send DMA data in a certain chunk size at a time.
  // In this case, it's 16-bits (65535).
  while (data_bytes > 0) {
    uint16_t cur_chunk = data_bytes > chunk_size ? chunk_size : data_bytes;
    spidma_queue(spi, SPIDMA_DATA, cur_chunk, (uint8_t *)source, 50010);
    source += cur_chunk;
    data_bytes -= cur_chunk;
  }

  spidma_queue(spi, SPIDMA_DESELECT, 0, 0, 50020);

  // Run the queue until it's empty
  while (spidma_check_activity(spi) != 0); // 0 = nothing to do
}


static uint8_t ili9341_invert_on = 0x21;  // INVON
static uint8_t ili9341_invert_off = 0x20; // INVOFF

void spidma_ili9341_invert(spidma_config_t *spi, bool invert) {
  spidma_queue(spi, SPIDMA_SELECT, 0, 0, 60000);
  spidma_queue(spi, SPIDMA_COMMAND, 1, invert ? &ili9341_invert_on : &ili9341_invert_off, 60010);
  spidma_queue(spi, SPIDMA_DESELECT, 0, 0, 60020);

  // Run the queue until it's empty
  while (spidma_check_activity(spi) != 0); // 0 = nothing to do
}

