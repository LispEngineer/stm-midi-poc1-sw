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
 * IMPORTANT NOTES:
 * 1. This currently is not remotely thread safe, but that's
 *    okay for now because the current target is non-threaded
 *    and single-core
 * 2. This currently will NOT work with multiple instances
 *    due to having a single interrupt handler with no way to
 *    determine which spidma_config_t's interrupt is being handled.
 *    (Not having closures in plain C is a bit annoying.)
 *
 *  Created on: 2024-11-11
 *  Updated on: 2024-11-15
 *      Author: Douglas P. Fields, Jr. - symbolics@lisp.engineer
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#include <stdlib.h> // free()
#include "stm32f7xx_hal.h"
#include "spidma.h"

// Our console U(S)ART
extern UART_HandleTypeDef huart2;

// This is a hack until we figure out how to put it into the spidma_config_t
static volatile uint32_t is_sending = 0;
static spidma_config_t *current_sending_spi = NULL; // This would also initialized to NULL

/*
 * Callback function when an SPI DMA transfer has completed,
 * mediated by the STM32 HAL. This is an interrupt handler.
 *
 * If the current send wants us to do a memory free(), we
 * queue that up for later processing.
 *
 * We gratituously blink an LED for now just to show it working.
 * FIXME: Remove LED blinking.
 */
static void spi_transfer_complete(SPI_HandleTypeDef *hspi) {

  // If we're not the sending SPI interrupt, do nothing
  if (current_sending_spi == NULL || hspi != current_sending_spi->spi) {
    return;
  }

  // If the user wants to free this memory, queue it for freeing
  // in the non-interrupt thread
  if (current_sending_spi->current_entry.repeats == 0 &&
      current_sending_spi->current_entry.should_free) {
    spidma_free_queue(current_sending_spi, current_sending_spi->current_entry.buff);
  }

  // Flash or transmit stuff for gratuitous purposes
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14); // Green LED
  // HAL_UART_Transmit(&huart2, (uint8_t *)"+", 1, HAL_MAX_DELAY);

  is_sending = 0;
  current_sending_spi = NULL;
}

/**
 * Initialize the DMA transfers and the
 * state of the SPI-supporting GPIO pins,
 * as well as the sending queue,
 * freeing queue, and in_delay flag.
 *
 * There can be only ONE spidma_config_t for each
 * (SPI_HandleTypeDef *) because we (may) use that to look up the
 * spidma_config_t later in an interrupt function.
 *
 * The caller is expected to have initialized these fields:
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
 */
void spidma_init(spidma_config_t *spi) {
  // TODO: Make this a closure that knows which spidma_config_t was in use
  // (or at the least, a lookup table from the hspi to the spidma_config_t)
  HAL_SPI_RegisterCallback(spi->spi, HAL_SPI_TX_COMPLETE_CB_ID, spi_transfer_complete);
  is_sending = 0;
  current_sending_spi = NULL;

  // Set up the queues
  spi->head_entry = 0;
  spi->tail_entry = 0;
  spi->head_free = 0;
  spi->tail_free = 0;

  // Set up the status flags
  spi->in_delay = 0;
}

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
  return !is_sending;
  // return HAL_DMA_GetState(spi->dma_tx) == HAL_DMA_STATE_READY;
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

  // Start a DMA transfer; set our status for the send complete callback
  is_sending = 1;
  current_sending_spi = spi;
  HAL_StatusTypeDef retval = HAL_SPI_Transmit_DMA(spi->spi, buff, buff_size);

  if (retval == HAL_OK) {
    if (spi->synchronous) {
      spidma_wait_for_completion(spi);
    }
    return 0;
  }

  // One of the HAL-not-OK values shifted to the next nibble
  is_sending = 0;
  current_sending_spi = NULL;
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
void spidma_wait_for_completion(spidma_config_t *spi) {
  while (!spidma_is_dma_ready(spi));
}



///////////////////////////////////////////////////////////////////////////////////
// Sending queue functions

/*
 * Calculate the next queue entry from the current one.
 * We require the queue size to be a power of two so we can
 * do our modulo wrap-around with a simple AND operator.
 */
static inline spiq_size_t next_entry(spiq_size_t x) {
  return (spiq_size_t)((x + (spiq_size_t)1) & SPI_ENTRY_MASK);
}

/*
 * Checks if the DMA sending queue is full. It's full when
 * the tail is directly behind the head.
 */
static inline uint32_t spidma_is_queue_full(spidma_config_t *spi) {
  return spi->head_entry == next_entry(spi->tail_entry);
}

/*
 * Returns 0 if we cannot add a queue entry.
 *
 * We add something by incrementing the tail.
 * We read from the head.
 */
uint32_t spidma_queue_repeats(spidma_config_t *spi, uint8_t type, uint16_t buff_size,
                                uint8_t *buff, uint32_t identifier, uint8_t repeats,
                                uint8_t should_free) {

  spiq_size_t c = spi->tail_entry;
  spiq_size_t n = next_entry(c);
  spidma_entry_t *entry;

  // check if we have room.
  // We are out of space when the tail is right behind the head.
  if (spi->head_entry == n) {
    return 0;
  }

  // Update the tail
  entry = &(spi->entries[c]);
  spi->tail_entry = n;

  // And save the entry's data
  entry->type = type;
  entry->buff = buff;
  entry->buff_size = buff_size;
  entry->identifier = identifier;
  entry->repeats = repeats;
  entry->should_free = should_free;

  return 1;
}

/** Queues this buffer for sending with 0 repeats and no freeing */
uint32_t spidma_queue(spidma_config_t *spi, uint8_t type, uint16_t buff_size,
                       uint8_t *buff, uint32_t identifier) {
  return spidma_queue_repeats(spi, type, buff_size, buff, identifier, 0, 0);
}

/*
 * Returns 0 if we cannot add a free'ing entry.
 *
 * We add something by incrementing the tail.
 * We read from the head.
 */
uint32_t spidma_free_queue(spidma_config_t *spi, void *buff) {
  if (NULL == buff) {
    // We do not add nulls, even if requested, but we pretend
    // it worked
    return 2;
  }

  spiq_size_t c = spi->tail_free;
  spiq_size_t n = next_entry(c);

  // Check if full
  if (spi->head_free == n) {
    return 0;
  }
  // Add entry
  spi->free_entries[c] = buff;
  spi->tail_free = n;

  return 1;
}

/*
 * Returns NULL if there is nothing to dequeue.
 */
void *spidma_free_dequeue(spidma_config_t *spi) {
  if (spi->head_free == spi->tail_free) {
    // Queue is empty
    return NULL;
  }

  void *retval = spi->free_entries[spi->head_free];

  spi->head_free = next_entry(spi->head_free);
  return retval;
}

/*
 * Processes the next queue entry, if necessary,
 * or continues waiting.
 *
 * Return values:
 * 0 - nothing to do
 * 1 - we are in a delay
 * 2 - we ended a delay and have nothing more to do
 * 3 - DMA is busy sending still
 * 4 - invalid type dequeued; nothing done
 * 5 - delay begun
 * 6 - SPI DMA transfer begun
 * 7 - SPI aux pin set (Select, Reset)
 * FIXME: Make these return values into an enum
 */
uint32_t spidma_check_activity(spidma_config_t *spi) {
  uint32_t nothing_to_do = 0;
  uint32_t retval;
  void *freeable;

  // Free all memory waiting to be freed
  while ((freeable = spidma_free_dequeue(spi)) != NULL) {
    free(freeable);
  }

  // Handle our delay function
  if (spi->in_delay) {
    if (spi->delay_until == HAL_GetTick()) {
      // We are done delaying
      spi->in_delay = 0;
      nothing_to_do = 2;
      // Fall through to do the next thing
    } else {
      // Continue delaying - do nothing here
      return 1;
    }
  }

  // Check if we can do anything
  if (is_sending) {
    return 3;
  }

  if (spi->head_entry == spi->tail_entry) {
    // Queue is empty
    return nothing_to_do;
  }

  // Blink a light for gratuitous sake
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15); // Red LED

  // Take an entry off the queue and start doing it
  spidma_entry_t *e = &(spi->entries[spi->head_entry]);
  spi->current_entry = *e;

  // Do our action
  switch (e->type) {
  case SPIDMA_DATA:
    spidma_write_data(spi, e->buff, e->buff_size);
    // TODO: Check return value
    retval = 6;
    break;
  case SPIDMA_COMMAND:
    spidma_write_command(spi, e->buff, e->buff_size);
    // TODO: Check return value
    retval = 6;
    break;
  case SPIDMA_UNCHANGED:
    spidma_write(spi, e->buff, e->buff_size);
    retval = 6;
    break;
  case SPIDMA_DELAY:
    spi->in_delay = 1;
    spi->delay_until = HAL_GetTick() + e->buff_size;
    retval = 5;
    break;
  case SPIDMA_RESET:
    spidma_reset(spi);
    retval = 7;
    break;
  case SPIDMA_UNRESET:
    spidma_dereset(spi);
    retval = 7;
    break;
  case SPIDMA_SELECT:
    spidma_select(spi);
    retval = 7;
    break;
  case SPIDMA_DESELECT:
    spidma_deselect(spi);
    retval = 7;
    break;
  default:
    retval = 4;
    break;
  }

  // There is a possible race condition. If the DMA is really small,
  // it could have finished by the time we get here.

  // Move our head ahead, unless we will repeat this one
  if (e->repeats == 0) {
    spi->head_entry = next_entry(spi->head_entry);
  } else {
    e->repeats--;
  }

  return retval;
}

/*
 * Keep calling check_activity until we've got no more activity.
 * Returns number of checks it took.
 */
uint32_t spidma_empty_queue(spidma_config_t *spi) {
  // Run the queue until it's empty
  uint32_t retval = 0;

  // 0 = nothing to do
  while (spidma_check_activity(spi) != 0) {
    retval++;
  }

  return retval;
}

spiq_size_t spidma_queue_length(spidma_config_t *spi) {
  return (spi->head_entry - spi->tail_entry) & SPI_ENTRY_MASK;
}

