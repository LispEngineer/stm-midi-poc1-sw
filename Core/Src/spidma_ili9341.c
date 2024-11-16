/*
 * spidma_ili9341.c
 *
 * Commands to control an ILI9341 SPI display, sent using
 * my SPIDMA queueing mechanism.
 *
 *  Created on: 2024-11-14
 *  Updated on: 2024-11-15
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
#include <stdlib.h>
#include "main.h"
#include "spidma.h"
#include "fonts.h"
#include "spidma_ili9341.h"

uint32_t ili_mem_alloc_failures = 0;
uint32_t ili_mem_allocs = 0;
size_t ili_last_alloc_failure_size;
uint32_t ili_queue_failures = 0;

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

/*
 * Queue SPIDMA commands to fully initialize the ILI9341 display.
 * This also means that the SPIDMA queue needs to be deep enough
 * (which is some 40-odd right now).
 *
 * Selects the display but leaves the SPI display unselected.
 *
 * NOTE: After this is called, you should probably run these commands
 * all synchronously by calling: spidma_empty_queue()
 *
 * Display initialization based upon https://github.com/afiskon/stm32-ili9341 .
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
}


/*
 * Set the next data write to a certain rectangle of pixel memory.
 */
void spidma_ili9341_set_address_window(spidma_config_t *spi, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

  // We allocate a buffer for the entire multi-step transaction and free it
  // after the last bit of it is used up.
  uint32_t queued;
  const size_t SAW_BUFF_SIZE = 1 + 4 + 1 + 4 + 1;
  uint8_t *buff = (uint8_t *)malloc(SAW_BUFF_SIZE);
  ili_mem_allocs++;

  // Check return value for memory exhaustion
  if (NULL == buff) {
    // Toggle the blue LED
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_12);
    ili_mem_alloc_failures++;
    ili_last_alloc_failure_size = SAW_BUFF_SIZE;
    return;
  }

  // Note that we use a lot of magic numbers here, sorry
  buff[1] = 0x2A; // column address set - CASET
  buff[2] = (x0 >> 8) & 0xFF;
  buff[3] = x0 & 0xFF;
  buff[4] = (x1 >> 8) & 0xFF;
  buff[5] = x1 & 0xFF;
  buff[6] = 0x2B; // row address set - RASET
  buff[7] = (y0 >> 8) & 0xFF;
  buff[8] = y0 & 0xFF;
  buff[9] = (y1 >> 8) & 0xFF;
  buff[10] = y1 & 0xFF;
  // We send the head of the buffer as the LAST command
  // so that the pointer to be freed is correct.
  buff[0] = 0x2C; // write to RAM

  // column address set - CASET
  spidma_queue(spi, SPIDMA_COMMAND, 1, buff + 1, 10000);
  spidma_queue(spi, SPIDMA_DATA, 4, buff + 2, 10010);

  // row address set - RASET
  spidma_queue(spi, SPIDMA_COMMAND, 1, buff + 6, 10020);
  spidma_queue(spi, SPIDMA_DATA, 4, buff + 7, 10030);

  // write to RAM
  // no repeats, and yes please free this buffer at the end.
  // (Hence, this must be the original malloc'd buffer!)
  queued = spidma_queue_repeats(spi, SPIDMA_COMMAND, 1, buff, 10040, 0, 1);

  if (!queued) {
    // Memory won't be freed, so we have a memory leak. We need a backup
    // mechanism for freeing memory.
    ili_queue_failures++;
  }
}

// The size of this buffer must be:
// Not less than 1/250th of the size of the display memory
// (in terms of number of pixels), because we can only
// repeat up to 255 times.
// Since we're 76,800 pixels (240x320), 1024 would provide
// us 1/75th, so we're good.
// We use a power-of-two size so we can divide
// quickly and easily using a shift.
#define FILL_RECT_BUFF_SZ 1024
#define FILL_RECT_BUFF_SHIFT 10

/*
 * Fill a rectangle of the display with a fixed color.
 *
 * Does not queue any DE/SELECT commands.
 */
void spidma_ili9341_fill_rectangle(spidma_config_t *spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  // clipping
  if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
  if ((x + w - 1) >= ILI9341_WIDTH) w = ILI9341_WIDTH - x;
  if ((y + h - 1) >= ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

  spidma_ili9341_set_address_window(spi, x, y, x+w-1, y+h-1);

  size_t pos = 0;
  size_t end_pos = w * h;
  // We need to flip the bytes of the color we are setting
  uint16_t inv_color = ((color & 0xFF) << 8) | (color >> 8);
  uint8_t repeats;
  size_t remainder;
  uint8_t auto_free;
  uint32_t queued = 1;

  size_t buff_end = end_pos > FILL_RECT_BUFF_SZ ? FILL_RECT_BUFF_SZ : end_pos;
  uint16_t *fill_rect_buff = (uint16_t *)malloc(buff_end * sizeof(uint16_t));
  ili_mem_allocs++;

  // Check return value for memory exhaustion
  if (NULL == fill_rect_buff) {
    // Toggle the blue LED
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_12);
    ili_mem_alloc_failures++;
    ili_last_alloc_failure_size = buff_end * sizeof(uint16_t);
    return;
  }

  // Set everything we're sending (up to buffer size) first
  for (pos = 0; pos < buff_end; pos++) {
      fill_rect_buff[pos] = inv_color;
  }
  // Calculate the # of repeats
  repeats = end_pos >> FILL_RECT_BUFF_SHIFT; // 1500 / 1024 = 1
  remainder = end_pos - (repeats << FILL_RECT_BUFF_SHIFT);
  auto_free = remainder == 0;

  // Add all the repeats, if any
  if (repeats > 0) {
    // If this is a perfect number of repeats, we need to free
    // Final 0 means "don't auto-free this buffer"
    queued = spidma_queue_repeats(spi, SPIDMA_DATA, FILL_RECT_BUFF_SZ * 2, (uint8_t *)fill_rect_buff, 20010, repeats - 1, auto_free);
  }

  // If there is non-perfect multiple, then we need to send the final
  // pixels out and then also free the buffer
  if (remainder > 0) {
    queued = spidma_queue_repeats(spi, SPIDMA_DATA, remainder * 2, (uint8_t *)fill_rect_buff, 20020, 0, 1); // No repeats & auto-free
  }

  if (!queued) {
    // Memory won't be freed, so we have a memory leak. We need a backup
    // mechanism for freeing memory.
    ili_queue_failures++;
  }
}

/*
 * Fills the entire screen (per defined screen size) with a given color.
 *
 * TODO: Make the screen size configurable in spidma_config_t?
 */
void spidma_ili9341_fill_screen(spidma_config_t *spi, uint16_t color) {
  spidma_ili9341_fill_rectangle(spi, 0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

/*
 * Draws a single pixel of a single color on the screen.
 *
 * NOTE: Super inefficiently implemented. Try not to use this. :)
 * It's more of a proof of concept than something useful.
 */
void spidma_ili9341_draw_pixel(spidma_config_t *spi, uint16_t x, uint16_t y, uint16_t color) {
  if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT))
      return;

  spidma_ili9341_fill_rectangle(spi, x, y, 1, 1, color);
}

/*
 * Write a single character to the screen at the specified location,
 * with the specified foreground and background colors.
 * The font size is specified in FontDef. This allocates w * h * 2 bytes
 * of memory.
 */
// TODO: make this take a buffer to use to write to.
// If not provided, then allocate one and auto-free it.
// If provided, do NOT auto-free it unless the caller tells us to.
void spidma_ili9341_write_char(spidma_config_t *spi, uint16_t x, uint16_t y,
                                  char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
  uint32_t i, b, j;
  uint32_t queued;
  size_t buff_size = sizeof(uint16_t) * font.height * font.width;
  uint16_t *buff = (uint16_t *)malloc(buff_size);
  ili_mem_allocs++;

  // Check return value for memory exhaustion
  if (NULL == buff) {
    // Toggle the blue LED
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_12);
    ili_mem_alloc_failures++;
    ili_last_alloc_failure_size = buff_size;
    return;
  }

  // We need to flip the bytes of the color we are setting
  color = ((color & 0xFF) << 8) | (color >> 8);
  bgcolor = ((bgcolor & 0xFF) << 8) | (bgcolor >> 8);

  // 32 = space, we don't have glyphs for control characters
  size_t font_data_start = (ch - 32) * font.height;

  // Prepare to blast our character out in a single write.
  // Each bit in the font turns into a 2-byte word that
  // we send to the screen of either the foreground or background
  // color.
  uint16_t *bp = buff;
  for (i = 0; i < font.height; i++) {
    b = font.data[font_data_start + i];
    for (j = 0; j < font.width; j++) {
      *bp = ((b << j) & 0x8000) ? color : bgcolor;
      bp++;
    }
  }

  spidma_ili9341_set_address_window(spi, x, y, x + font.width - 1, y + font.height - 1);
  queued = spidma_queue_repeats(spi, SPIDMA_DATA, buff_size, (uint8_t *)buff, 30000, 0, 1); // Don't repeat & auto-free

  if (!queued) {
    // Memory won't be freed, so we have a memory leak. We need a backup
    // mechanism for freeing memory.
    ili_queue_failures++;
  }
}

/*
 * Returns next drawing position:
 * y in the top 16 bits
 * x in the bottom 16 bits
 *
 * It may be that the next drawing position would not actually fit
 * fully on the screen - the caller needs to check that
 * (new y + font.height > screen height) means off the screen
 *
 * Does not queue any DE/SELECT commands.
 *
 * Realize that this could use a lot of RAM and fill up
 * the SPI transmit queue if we have a lot of characters.
 * So don't send too many characters at a time...
 *
 * TODO: Make this write the string by lines, using a single buffer
 * per line. Render the characters into a line buffer, and do a single DMA
 * transfer of that buffer to the screen.
 *
 */
uint32_t spidma_ili9341_write_string(spidma_config_t *spi, uint16_t x, uint16_t y,
                                        const char *str, FontDef font, uint16_t color, uint16_t bgcolor) {
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
// TODO: Enhance SPIDMA to be able to do DMA direct from flash by having
// the SPIDMA figure that out in its queue manager, and copy the flash bit
// by bit into RAM and then DMA'ing it out to SPI without
void spidma_ili9341_draw_image(spidma_config_t *spi, uint16_t x, uint16_t y,
                                  uint16_t w, uint16_t h, const uint16_t *data,
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

  spidma_ili9341_set_address_window(spi, x, y, x + w - 1, y + h - 1);

  // We can only send DMA data in a certain chunk size at a time.
  // In this case, it's 16-bits (65535).
  while (data_bytes > 0) {
    uint16_t cur_chunk = data_bytes > chunk_size ? chunk_size : data_bytes;
    spidma_queue(spi, SPIDMA_DATA, cur_chunk, (uint8_t *)source, 50010);
    source += cur_chunk;
    data_bytes -= cur_chunk;
  }

}


static uint8_t ili9341_invert_on = 0x21;  // INVON
static uint8_t ili9341_invert_off = 0x20; // INVOFF

/*
 * Send a command to invert or uninvert the ILI9341 display.
 */
void spidma_ili9341_invert(spidma_config_t *spi, bool invert) {
  spidma_queue(spi, SPIDMA_COMMAND, 1, invert ? &ili9341_invert_on : &ili9341_invert_off, 60010);

  // Run the queue until it's empty
  spidma_empty_queue(spi);
}
