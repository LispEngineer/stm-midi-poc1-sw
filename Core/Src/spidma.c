/*
 * spidma.c
 *
 * Routines to have a queue of things to transfer
 * (outbound only) via SPI, with a command/data
 * and chip select pin.
 *
 * Functionality:
 * * Enqueue things
 *   * Provide an error when things cannot be enqueued
 *   * Queue entries have a generic 32-bit value for end user identification
 *   * Queue entries have a monotonically increasing internal identifier
 *     which is returned to the caller
 * * Get notification when a queued thing starts or stops
 * * Queue status queries
 *   * Counter of how many things are queued
 *   * What is currently being sent
 * * Automatic submission of queued items to SPI DMA
 *   * First setting the CS and DC signals
 *
 * Non-features:
 * * Thread safety
 *
 *  Created on: 2024-11-11
 *  Updated on: 2024-11-12
 *      Author: Douglas P. Fields, Jr. - symbolics@lisp.engineer
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#include "stm32f7xx_hal.h"
#include "spidma.h"


/** Initialize the DMA transfers and the
 * state of the SPI pins
 */
void spidma_init(spidma_config_t *);

/** Set the chip select for this SPI device
 * (active low).
 */
inline void spidma_select(spidma_config_t *spi) {
  if (spi->use_cs) {
    HAL_GPIO_WritePin(spi->bank_cs, spi->pin_cs, GPIO_PIN_RESET);
  }
}

/** Reset the chip select for this SPI device
 * (inactive high).
 */
inline void spidma_deselect(spidma_config_t *spi) {
  if (spi->use_cs) {
    HAL_GPIO_WritePin(spi->bank_cs, spi->pin_cs, GPIO_PIN_SET);
  }
}

/** Assert the reset pin for this SPI device
 * (active low).
 */
inline void spidma_reset(spidma_config_t *spi) {
  if (spi->use_reset) {
    HAL_GPIO_WritePin(spi->bank_reset, spi->pin_reset, GPIO_PIN_RESET);
  }
}

/** Clear the reset pin for this SPI device
 * (inactive high).
 */
inline void spidma_dereset(spidma_config_t *spi) {
  if (spi->use_reset) {
    HAL_GPIO_WritePin(spi->bank_reset, spi->pin_reset, GPIO_PIN_SET);
  }
}

/** Returns non-zero if DMA channel is ready to send. */
uint32_t spidma_is_dma_ready(spidma_config_t *spi) {
  return HAL_DMA_GetState(spi->dma_tx) == HAL_DMA_STATE_READY;
}

/** Send a buffer of data over the SPI connection via DMA.
 *
 * This is a low-level routine and just starts the DMA transfer.
 *
 * NOTE: This can only be called when the DMA is not
 * transmitting anything.
 *
 * NOTE: The maximum size that can be sent is 65,535.
 *
 * Return value:
 * 0 - everything good
 * 1 - size too big
 * 2 - DMA in use
 * non-zero - other errors
 */
uint32_t spidma_write(spidma_config_t *spi, uint8_t *buff, size_t buff_size) {
  if (buff_size > 65535) {
    return 1U;
  }

  // Check if DMA is in use
  if (!spidma_is_dma_ready(spi)) {
    // Oops, DMA is in use
    return 2U;
  }

  // TODO: Start a DMA transfer
  HAL_StatusTypeDef retval = HAL_SPI_Transmit_DMA(spi->spi, buff, buff_size);

  if (retval == HAL_OK) {
    if (spi->synchronous) {
      while (!spidma_is_dma_ready(spi));
    }
    return 0;
  }

  // One of the HAL-not-OK values shifted to the next nibble
  return retval << 4;
}

/** This asserts the command signal and then sends the specified data
 * using spidma_write().
 */
uint32_t spidma_write_command(spidma_config_t *spi, uint8_t *buff, size_t buff_size) {
  // Check if DMA is in use
  if (!spidma_is_dma_ready(spi)) {
    // Oops, DMA is in use
    return 2U;
  }

  HAL_GPIO_WritePin(spi->bank_dc, spi->pin_dc, GPIO_PIN_RESET);
  return spidma_write(spi, buff, buff_size);
}


/** This asserts the data signal and then sends the specified data
 * using spidma_write().
 */
uint32_t spidma_write_data(spidma_config_t *spi, uint8_t *buff, size_t buff_size) {
  // Check if DMA is in use
  if (!spidma_is_dma_ready(spi)) {
    // Oops, DMA is in use
    return 2U;
  }

  HAL_GPIO_WritePin(spi->bank_dc, spi->pin_dc, GPIO_PIN_SET);
  return spidma_write(spi, buff, buff_size);
}

/** This does a busy wait, waiting for any DMA transfer
 * on the SPI channel to complete.
 *
 * TODO: Add a max timeout
 */
void spidma_wait_for_completion(spidma_config_t *) {
  // TODO: CODE ME
}
