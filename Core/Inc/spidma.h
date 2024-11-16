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

#define NUM_SPI_ENTRIES ((size_t)64)   // This must be a power of two
#define SPI_ENTRY_MASK  ((size_t)0x3F) // A mask of the number of bits to hold the value above from 0 to that minus 1
typedef uint16_t spiq_size_t;

typedef enum spidma_entry_type {
  SPIDMA_DATA,      // Set the D/C flag to Data
  SPIDMA_COMMAND,   // Set the D/C flag to Command
  SPIDMA_UNCHANGED, // Do not change the D/C flag setting
  SPIDMA_DELAY,     // buff_size is number of milliseconds
  SPIDMA_RESET,     // Assert the RESET signal (active low)
  SPIDMA_UNRESET,   // De-assert the RESET signal
  SPIDMA_SELECT,    // Assert the chip select signal (active low)
  SPIDMA_DESELECT   // De-assert the CS signal
} spidma_entry_type_t;

/*
 * This structure holds a single entry for things
 * that should be sent over the SPI bus in a queue.
 */
typedef struct spidma_entry {
  uint8_t  type;
  uint16_t buff_size;
  uint8_t *buff;
  // User-assigned identifier of this queue entry
  uint32_t identifier;
  // TODO: Should we have a flag to allow a free(buff)?
  // How many times should we repeat this entry?
  uint8_t  repeats;
  // Should we free the buff after we are done?
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
  uint8_t synchronous;

  // The HAL type for the SPI port we're using,
  // and the associated transfer DMA
  SPI_HandleTypeDef *spi;
  DMA_HandleTypeDef *dma_tx;

  // TODO: Flag set when we're sending - reset by interrupt
  // Just before we do anything, we set this to what we're doing
  spidma_entry_t current_entry;

  // Our sending buffer
  // Note that none of this code is thread-safe
  spidma_entry_t entries[NUM_SPI_ENTRIES];
  spiq_size_t head_entry; // When head == tail, queue is EMPTY
  spiq_size_t tail_entry;
  uint8_t  in_delay;
  uint32_t delay_until;

  // Our freeing buffer - memory we will free later
  void *      free_entries[NUM_SPI_ENTRIES];
  spiq_size_t head_free; // When head == tail, queue is EMPTY
  spiq_size_t tail_free;
} spidma_config_t;


void spidma_init(spidma_config_t *);

// Functions to handle the CS and RESET pins
void spidma_select(spidma_config_t *);
void spidma_deselect(spidma_config_t *);
void spidma_reset(spidma_config_t *);
void spidma_dereset(spidma_config_t *);

// Functions to actually send data over the SPI; the data/command
// GPIO is set by two of these and ignored by the third.
uint32_t spidma_write(spidma_config_t *, uint8_t *buff, size_t buff_size);
uint32_t spidma_write_command(spidma_config_t *, uint8_t *buff, size_t buff_size);
uint32_t spidma_write_data(spidma_config_t *, uint8_t *buff, size_t buff_size);

// Helper that just spins until there is no DMA running
// (the busy flag is reset by interrupt, so don't disable interrupts).
void spidma_wait_for_completion(spidma_config_t *);

// SPI transmit queue functions
uint32_t spidma_queue(spidma_config_t *, uint8_t, uint16_t, uint8_t *, uint32_t); // 0 repeats, no freeing
uint32_t spidma_queue_repeats(spidma_config_t *, uint8_t, uint16_t, uint8_t *,
                                uint32_t, uint8_t repeats, uint8_t should_free);

// This needs to be called regularly to keep the SPI queue emptied.
uint32_t spidma_check_activity(spidma_config_t *spi);

// SPI memory free queue functions
uint32_t spidma_free_queue(spidma_config_t *spi, void *buff);
void *spidma_free_dequeue(spidma_config_t *spi);

// TODO: Function to drain the SPI queue in a busy loop

#endif /* INC_SPIDMA_H_ */
