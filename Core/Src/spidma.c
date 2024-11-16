/*
 * spidma.c
 *
 *  Created on: 2024-11-11
 *  Updated on: 2024-11-16
 *      Author: Douglas P. Fields, Jr. - symbolics@lisp.engineer
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
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
 *   * Allow a buffer to be auto-free'd
 *   * If the buffer cannot be auto-free'd because the SPI queue is full,
 *     add it to a backup freeing queue and free it later when the SPI queue
 *     is empty, to reduce likelihood of memory leaks.
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
 * Any request for an SPI event with auto-freeing that could not be queued is
 * instead now queued
 * into a backup freeing queue, and whenever the SPI queue is empty we free
 * everything in this backup free queue. While we're at it, we also
 * add free requests to the backup free queue if we can't add them to the main
 * freeing queue.
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
 */

#include <stdlib.h> // free()
#include <stdbool.h>
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
  // TODO: Check return value
  is_sending = 0;
  current_sending_spi = NULL;

  // Set up the queues
  spi->head_entry = 0;
  spi->tail_entry = 0;
  spi->entry_queue_failures = 0;
  spi->head_free = 0;
  spi->tail_free = 0;
  spi->free_queue_failures = 0;
  spi->head_backup_free = 0;
  spi->tail_backup_free = 0;
  spi->backup_free_queue_failures = 0;
  spi->mem_frees = 0;
  spi->backup_frees = 0;

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
bool spidma_is_dma_ready(spidma_config_t *spi) {
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
spidma_return_value_t spidma_write(spidma_config_t *spi, uint8_t *buff, size_t buff_size) {
  if (buff_size > 65535) {
    return SDRV_TOO_BIG;
  }

  // Check if DMA is in use
  if (!spidma_is_dma_ready(spi)) {
    // Oops, DMA is in use
    return SDRV_DMA_BUSY;
  }

  // Start a DMA transfer; set our status for the send complete callback
  is_sending = 1;
  current_sending_spi = spi;
  HAL_StatusTypeDef retval = HAL_SPI_Transmit_DMA(spi->spi, buff, buff_size);

  if (retval == HAL_OK) {
    if (spi->synchronous) {
      spidma_wait_for_completion(spi);
    }
    return SDRV_OK;
  }

  // One of the HAL-not-OK values shifted to the next nibble
  spi->last_hal_error = retval;
  is_sending = 0;
  current_sending_spi = NULL;
  return SDRV_HAL_ERROR;
}

/** This asserts the command signal and then sends the specified data
 * using spidma_write().
 */
spidma_return_value_t spidma_write_command(spidma_config_t *spi, uint8_t *buff, size_t buff_size) {
  // Check if DMA is in use
  if (!spidma_is_dma_ready(spi)) {
    // Oops, DMA is in use
    return SDRV_DMA_BUSY;
  }

  HAL_GPIO_WritePin(spi->bank_dc, spi->pin_dc, GPIO_PIN_RESET);
  return spidma_write(spi, buff, buff_size);
}


/** This asserts the data signal and then sends the specified data
 * using spidma_write().
 */
spidma_return_value_t spidma_write_data(spidma_config_t *spi, uint8_t *buff, size_t buff_size) {
  // Check if DMA is in use
  if (!spidma_is_dma_ready(spi)) {
    // Oops, DMA is in use
    return SDRV_DMA_BUSY;
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

/*
 * Calculate the next queue entry from the current one.
 * We require the queue size to be a power of two so we can
 * do our modulo wrap-around with a simple AND operator.
 */
static inline spiq_size_t next_entry(spiq_size_t x) {
  return (spiq_size_t)((x + (spiq_size_t)1) & SPI_ENTRY_MASK);
}



///////////////////////////////////////////////////////////////////////////////////
// Backup free queue functions


/*
 * Returns queueing status. We ignore requests to queue NULLs.
 *
 * We add something by incrementing the tail.
 * We read from the head.
 */
static spidma_return_value_t spidma_backup_free_queue(spidma_config_t *spi, void *buff) {
  if (NULL == buff) {
    // We do not add nulls, even if requested, but we pretend
    // it worked
    return SDRV_QUEUE_IGNORED;
  }

  spiq_size_t c = spi->tail_backup_free;
  spiq_size_t n = next_entry(c);

  // Check if full
  if (spi->head_backup_free == n) {
    spi->backup_free_queue_failures++;
    return SDRV_QUEUE_FULL;
  }
  // Add entry
  spi->backup_free_entries[c] = buff;
  spi->tail_backup_free = n;

  return SDRV_OK;
}

/*
 * Returns NULL if there is nothing to dequeue.
 */
static void *spidma_backup_free_dequeue(spidma_config_t *spi) {
  if (spi->head_backup_free == spi->tail_backup_free) {
    // Queue is empty
    return NULL;
  }

  void *retval = spi->backup_free_entries[spi->head_backup_free];

  spi->head_backup_free = next_entry(spi->head_backup_free);
  return retval;
}

///////////////////////////////////////////////////////////////////////////////////
// Sending queue functions

/*
 * Checks if the DMA sending queue is full. It's full when
 * the tail is directly behind the head.
 */
static inline bool spidma_is_queue_full(spidma_config_t *spi) {
  return spi->head_entry == next_entry(spi->tail_entry);
}

/*
 * Returns status of queueing attempt.
 *
 * We add something by incrementing the tail.
 * We read from the head.
 */
spidma_return_value_t spidma_queue_repeats(spidma_config_t *spi, uint8_t type, uint16_t buff_size,
                                uint8_t *buff, uint32_t identifier, uint8_t repeats,
                                uint8_t should_free) {

  spiq_size_t c = spi->tail_entry;
  spiq_size_t n = next_entry(c);
  spidma_entry_t *entry;

  // check if we have room.
  // We are out of space when the tail is right behind the head.
  if (spi->head_entry == n) {
    spi->entry_queue_failures++;
    if (should_free) {
      // We need to queue this up for freeing, regardless,
      // when we are sure it isn't in use.
      // (Sometimes a buffer is used for multiple queue
      // entries and is only marked for freeing in the last
      // use queue'd.)
      spidma_return_value_t queued = spidma_backup_free_queue(spi, buff);
      // TODO: Check if that worked, return a special message?
    }
    return SDRV_QUEUE_FULL;
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

  return SDRV_OK;
}

/** Queues this buffer for sending with 0 repeats and no freeing */
spidma_return_value_t spidma_queue(spidma_config_t *spi, uint8_t type, uint16_t buff_size,
                                    uint8_t *buff, uint32_t identifier) {
  return spidma_queue_repeats(spi, type, buff_size, buff, identifier, 0, 0);
}

/*
 * Returns queueing status.
 *
 * We ignore requests to queue a NULL for freeing. :)
 *
 * We add something by incrementing the tail.
 * We read from the head.
 */
spidma_return_value_t spidma_free_queue(spidma_config_t *spi, void *buff) {
  if (NULL == buff) {
    // We do not add nulls, even if requested, but we pretend
    // it worked
    return SDRV_QUEUE_IGNORED;
  }

  spiq_size_t c = spi->tail_free;
  spiq_size_t n = next_entry(c);

  // Check if full
  if (spi->head_free == n) {
    spi->free_queue_failures++;
    // TODO: Add this to the backup free queue instead?
    // If that also fails, we have big problems and the
    // caller has to deal with it.
    return SDRV_QUEUE_FULL;
  }
  // Add entry
  spi->free_entries[c] = buff;
  spi->tail_free = n;

  return SDRV_OK;
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

/** Free all the memory waiting for freeing in our backup queue. */
static void spidma_do_backup_free_queue_freeing(spidma_config_t *spi) {
  void *freeable;

  // Free all memory waiting to be freed
  while ((freeable = spidma_backup_free_dequeue(spi)) != NULL) {
    free(freeable);
    spi->mem_frees++;
    spi->backup_frees++;
  }
}

/*
 * Processes the next queue entry, if necessary,
 * or continues waiting.
 *
 * Return values:
 * SDAS_NOTHING - nothing to do
 * SDAS_IN_DELAY - we are in a delay
 * SDAS_ENDED_DELAY_NOTHING - we ended a delay and have nothing more to do
 * SDAS_DMA_BUSY - DMA is busy sending still
 * SDAS_INVALID_NOTHING - invalid type dequeued; nothing done
 * SDAS_DELAY_STARTED - delay begun
 * SDAS_DMA_STARTED - SPI DMA transfer begun
 * SDAS_AUX_SET - SPI aux pin set (Select, Reset)
 * FIXME: Make these return values into an enum
 */
spidma_activity_status_t spidma_check_activity(spidma_config_t *spi) {
  spidma_activity_status_t nothing_to_do = SDAS_NOTHING;
  spidma_activity_status_t retval;
  void *freeable;

  // Free all memory waiting to be freed
  while ((freeable = spidma_free_dequeue(spi)) != NULL) {
    free(freeable);
    spi->mem_frees++;
  }

  // Handle our delay function
  if (spi->in_delay) {
    if (spi->delay_until == HAL_GetTick()) {
      // We are done delaying
      spi->in_delay = 0;
      nothing_to_do = SDAS_ENDED_DELAY_NOTHING;
      // Fall through to do the next thing
    } else {
      // Continue delaying - do nothing here
      return SDAS_IN_DELAY;
    }
  }

  // Check if we can do anything
  if (is_sending) {
    return SDAS_DMA_BUSY;
  }

  if (spi->head_entry == spi->tail_entry) {
    // Queue is empty
    // Do our backup queue free'ing.
    spidma_do_backup_free_queue_freeing(spi);
    // TODO: Also mark the backup free queue length
    // and clear it of that many entries every entry queue length
    // entries processed.
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
    retval = SDAS_DMA_STARTED;
    break;
  case SPIDMA_COMMAND:
    spidma_write_command(spi, e->buff, e->buff_size);
    // TODO: Check return value
    retval = SDAS_DMA_STARTED;
    break;
  case SPIDMA_UNCHANGED:
    spidma_write(spi, e->buff, e->buff_size);
    retval = SDAS_DMA_STARTED;
    break;
  case SPIDMA_DELAY:
    spi->in_delay = 1;
    spi->delay_until = HAL_GetTick() + e->buff_size;
    retval = SDAS_DELAY_STARTED;
    break;
  case SPIDMA_RESET:
    spidma_reset(spi);
    retval = SDAS_AUX_SET;
    break;
  case SPIDMA_UNRESET:
    spidma_dereset(spi);
    retval = SDAS_AUX_SET;
    break;
  case SPIDMA_SELECT:
    spidma_select(spi);
    retval = SDAS_AUX_SET;
    break;
  case SPIDMA_DESELECT:
    spidma_deselect(spi);
    retval = SDAS_AUX_SET;
    break;
  default:
    retval = SDAS_INVALID_NOTHING;
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

  while (spidma_check_activity(spi) != SDAS_NOTHING) {
    retval++;
  }

  return retval;
}

spiq_size_t spidma_queue_length(spidma_config_t *spi) {
  return (spi->tail_entry - spi->head_entry) & SPI_ENTRY_MASK;
}

/*
 * Returns the number of queue entries we can submit before
 * we are full.
 */
spiq_size_t spidma_queue_remaining(spidma_config_t *spi) {
  // We lose one entry because of the need to differentiate full
  // from empty.
  return NUM_SPI_ENTRIES - spidma_queue_length(spi) - 1;
}

