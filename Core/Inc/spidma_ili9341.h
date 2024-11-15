/*
 * spidma_ili9341.h
 *
 *  Created on: 2024-11-14
 *  Updated on: 2024-11-14
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#ifndef INC_SPIDMA_ILI9341_H_
#define INC_SPIDMA_ILI9341_H_

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
