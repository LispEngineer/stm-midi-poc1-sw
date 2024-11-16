/*
 * spidma.h
 *
 *  Created on: 2024-11-11
 *  Updated on: 2024-11-16
 *      Author: Douglas P. Fields, Jr. - symbolics@lisp.engineer
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 *
 * See .c file for full description.
 *
 * Configurable items in this file:
 * 1. Number of queue entries in the SPIDMA queue
 *    * This must be a multiple of 2
 * 2. TODO: define if you want various counters enabled
 *    (useful for debugging)
 */

#ifndef INC_SPIDMA_H_
#define INC_SPIDMA_H_

#define NUM_SPI_ENTRIES ((size_t)256)   // This must be a power of two
#define SPI_ENTRY_MASK  ((size_t)0xFF)  // A mask of the number of bits to hold the value above from 0 to that minus 1
typedef uint16_t spiq_size_t;

typedef enum spidma_entry_type {
  SPIDMA_DATA,      // Set the D/C flag to Data - and send data
  SPIDMA_COMMAND,   // Set the D/C flag to Command - and send data
  SPIDMA_UNCHANGED, // Do not change the D/C flag setting - and send data
  SPIDMA_DELAY,     // Delay before continuing the queue - buff_size is number of milliseconds
  SPIDMA_RESET,     // Assert the RESET signal (active low)
  SPIDMA_UNRESET,   // De-assert the RESET signal
  SPIDMA_SELECT,    // Assert the chip select signal (active low)
  SPIDMA_DESELECT   // De-assert the CS signal
} spidma_entry_type_t;

/*
* Return values:
* 0 - nothing to do
* 1 - we are in a delay
* 2 - we ended a delay and have nothing more to do
* 3 - DMA is busy sending still
* 4 - invalid type dequeued; nothing done
* 5 - delay begun
* 6 - SPI DMA transfer begun
* 7 - SPI aux pin set (Select, Reset)
*/
typedef enum spidma_activity_status {
  SDAS_NOTHING,
  SDAS_IN_DELAY,
  SDAS_ENDED_DELAY_NOTHING,
  SDAS_DMA_BUSY,
  SDAS_INVALID_NOTHING,
  SDAS_DELAY_STARTED,
  SDAS_DMA_STARTED,
  SDAS_AUX_SET
} spidma_activity_status_t;

typedef enum spidma_return_value {
  SDRV_QUEUE_FULL,
  SDRV_OK,
  SDRV_QUEUE_IGNORED, // Usually because we asked to queue something invalid like a NULL to free
  SDRV_TOO_BIG,
  SDRV_DMA_BUSY,
  SDRV_HAL_ERROR // HAL error - see last_hal_error
} spidma_return_value_t;


/*
 * This structure holds a single entry for things
 * that should be sent over the SPI bus in a queue.
 */
typedef struct spidma_entry {
  // User-assigned identifier of this queue entry
  uint32_t identifier;
  // What we should do
  spidma_entry_type_t type;
  // Buffer if we need to send something
  uint8_t *buff;
  uint16_t buff_size; // DMA can handle 65535 maximum send
  // How many times should we repeat this entry?
  uint8_t  repeats;
  // Should we free the buff after we are done?
  // TODO: Make this a bit-mask with other flags if needed
  uint8_t  should_free;
} spidma_entry_t;

/*
 * This contains everything we need to manage an SPI
 * connected display with additional GPIO pins for
 * chip select, command/data, and reset.
 * CS and RESET are active low.
 *
 * All transfers are done via a queue, which needs to
 * be serviced periodically.
 *
 * NOTE: This structure should be allocated in the fast
 * data RAM of the microcontroller because it will be
 * accessed several times during an SPI transfer complete
 * interrupt.
 */
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
  // FIXME: Remove this functionality
  uint8_t synchronous;

  // The HAL type for the SPI port we're using,
  // and the associated transfer DMA
  SPI_HandleTypeDef *spi;
  DMA_HandleTypeDef *dma_tx;

  // TODO: Flag set when we're sending - reset by interrupt
  // Just before we do anything using DMA, we set this to what we're doing,
  // so that after the DMA finishes, we know what we did and can take
  // the necessary action (e.g., freeing memory)
  spidma_entry_t current_entry;

  // Whenever we get a HAL error, we put it here
  HAL_StatusTypeDef last_hal_error;

  // Our sending ring buffer
  // Note that none of this code is thread-safe or re-entrant
  spidma_entry_t entries[NUM_SPI_ENTRIES];
  spiq_size_t head_entry; // When head == tail, queue is EMPTY
  spiq_size_t tail_entry;
  uint32_t entry_queue_failures;

  // Our delay handling variables
  uint8_t  in_delay;
  uint32_t delay_until;

  // Our freeing ring buffer - memory we will free
  // identified in an interrupt and actually free'd later
  void *      free_entries[NUM_SPI_ENTRIES];
  spiq_size_t head_free; // When head == tail, queue is EMPTY
  spiq_size_t tail_free;
  uint32_t free_queue_failures;

  void *      backup_free_entries[NUM_SPI_ENTRIES];
  spiq_size_t head_backup_free; // When head == tail, queue is EMPTY
  spiq_size_t tail_backup_free;
  uint32_t backup_free_queue_failures;

  uint32_t    mem_frees;
  uint32_t    backup_frees;
} spidma_config_t;


// Required to be called prior to anything else
void spidma_init(spidma_config_t *);

// Functions to handle the CS and RESET pins
void spidma_select(spidma_config_t *);
void spidma_deselect(spidma_config_t *);
void spidma_reset(spidma_config_t *);
void spidma_dereset(spidma_config_t *);

// Functions to actually send data over the SPI; the data/command
// GPIO is set by two of these and ignored by the third.
spidma_return_value_t spidma_write(spidma_config_t *, uint8_t *buff, size_t buff_size);
spidma_return_value_t spidma_write_command(spidma_config_t *, uint8_t *buff, size_t buff_size);
spidma_return_value_t spidma_write_data(spidma_config_t *, uint8_t *buff, size_t buff_size);

// Helper that just spins until there is no DMA running
// (the busy flag is reset by interrupt, so don't disable interrupts).
void spidma_wait_for_completion(spidma_config_t *);

// SPI transmit queue functions
spidma_return_value_t spidma_queue(spidma_config_t *, uint8_t, uint16_t, uint8_t *, uint32_t); // 0 repeats, no freeing
spidma_return_value_t spidma_queue_repeats(spidma_config_t *, uint8_t, uint16_t, uint8_t *,
                                uint32_t, uint8_t repeats, uint8_t should_free);
spiq_size_t spidma_queue_length(spidma_config_t *spi);
spiq_size_t spidma_queue_remaining(spidma_config_t *spi);


// This needs to be called regularly to keep the SPI queue emptied.
spidma_activity_status_t spidma_check_activity(spidma_config_t *spi);
// This busy waits until the queue is totally empty.
uint32_t spidma_empty_queue(spidma_config_t *spi);

// SPI memory free queue functions
spidma_return_value_t spidma_free_queue(spidma_config_t *spi, void *buff);
void *spidma_free_dequeue(spidma_config_t *spi);

// TODO: Function to drain the SPI queue in a busy loop

#endif /* INC_SPIDMA_H_ */
