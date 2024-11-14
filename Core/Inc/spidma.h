/*
 * spidma.h
 *
 *  Created on: 2024-11-11
 *  Updated on: 2024-11-12
 *      Author: Douglas P. Fields, Jr. - symbolics@lisp.engineer
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#ifndef INC_SPIDMA_H_
#define INC_SPIDMA_H_

typedef struct spidma_config {
  // The pins and banks for these SPI display signals:
  // CS - Chip Select (active low)
  // DC - Data or Command
  // RESET - Reset (active low)
  uint16_t pin_cs;
  uint16_t pin_dc;
  uint16_t pin_reset;

  GPIO_TypeDef *bank_cs;
  GPIO_TypeDef *bank_dc;
  GPIO_TypeDef *bank_reset;

  // Whether we should use these signals
  uint8_t use_cs;
  uint8_t use_reset;

  // Should we "fake" being synchronous by waiting for
  // the DMA to finish?
  uint8_t synchronous;

  // The HAL type for the SPI port we're using,
  // and the associated transfer DMA
  SPI_HandleTypeDef *spi;
  DMA_HandleTypeDef *dma_tx;

  // TODO: Flag set when we're sending - reset by interrupt
} spidma_config_t;


void spidma_init(spidma_config_t *);

// Low level functions
void spidma_select(spidma_config_t *);
void spidma_deselect(spidma_config_t *);
void spidma_reset(spidma_config_t *);
void spidma_dereset(spidma_config_t *);

uint32_t spidma_write(spidma_config_t *, uint8_t *buff, size_t buff_size);
uint32_t spidma_write_command(spidma_config_t *, uint8_t *buff, size_t buff_size);
uint32_t spidma_write_data(spidma_config_t *, uint8_t *buff, size_t buff_size);

void spidma_wait_for_completion(spidma_config_t *);

// SPI transmit queue functions

#endif /* INC_SPIDMA_H_ */
