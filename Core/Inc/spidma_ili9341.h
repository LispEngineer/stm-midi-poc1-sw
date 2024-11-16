/*
 * spidma_ili9341.h
 *
 *  Created on: 2024-11-14
 *  Updated on: 2024-11-16
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#ifndef INC_SPIDMA_ILI9341_H_
#define INC_SPIDMA_ILI9341_H_

#include <stdbool.h>

// ILI9341 configuration
// Adapted from https://github.com/afiskon/stm32-ili9341

// Color definitions
#define ILI9341_BLACK   0x0000
#define ILI9341_BLUE    0x001F
#define ILI9341_RED     0xF800
#define ILI9341_GREEN   0x07E0
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_WHITE   0xFFFF
#define ILI9341_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04

// default orientation
/*
#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320
#define ILI9341_ROTATION (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR)
*/

// rotate right
#define ILI9341_WIDTH  320
#define ILI9341_HEIGHT 240
#define ILI9341_ROTATION (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)

// rotate left
/*
#define ILI9341_WIDTH  320
#define ILI9341_HEIGHT 240
#define ILI9341_ROTATION (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)
*/

// upside down
/*
#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320
#define ILI9341_ROTATION (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR)
*/

///////////////////////////////////////////////////
// Debugging counters

extern uint32_t ili_mem_alloc_failures;
extern size_t   ili_last_alloc_failure_size;
extern uint32_t ili_mem_allocs;
extern uint32_t ili_queue_failures;
extern uint32_t ili_characters_skipped;

///////////////////////////////////////////////////
// Functionality

void spidma_ili9341_init(spidma_config_t *spi);
void spidma_ili9341_set_address_window(spidma_config_t *spi, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void spidma_ili9341_fill_rectangle(spidma_config_t *spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void spidma_ili9341_fill_screen(spidma_config_t *spi, uint16_t color);
void spidma_ili9341_draw_pixel(spidma_config_t *spi, uint16_t x, uint16_t y, uint16_t color);
void spidma_ili9341_write_char(spidma_config_t *spi, uint16_t x, uint16_t y,
                                  char ch, FontDef font, uint16_t color, uint16_t bgcolor);
uint32_t spidma_ili9341_write_string(spidma_config_t *spi, uint16_t x, uint16_t y,
                                        const char *str, FontDef font, uint16_t color, uint16_t bgcolor);
void spidma_ili9341_draw_image(spidma_config_t *spi, uint16_t x, uint16_t y,
                                  uint16_t w, uint16_t h, const uint16_t* data,
                                  uint32_t copy_data);
void spidma_ili9341_invert(spidma_config_t *spi, bool invert);

#endif /* INC_SPIDMA_ILI9341_H_ */
