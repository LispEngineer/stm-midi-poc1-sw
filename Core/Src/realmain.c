/*
 * realmain.c
 * Since the main.c is auto-generated by STM32CubeIDE
 * this is the real "main" file for stuff that is not
 * auto-generated.
 *
 *  Created on: 2024-08-25
 *  Updated on: 2024-11-05
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "stm32f7xx_hal.h"
#include "stm32f7xx_ll_usart.h"
#include "string.h" // STM32 Core
#include "main.h"
#include "realmain.h"
#include "ringbuffer.h"
#include "midi.h"
#include "tonegen.h"

#define FAST_BSS __attribute((section(".fast_bss")))
#define FAST_DATA __attribute((section(".fast_data")))

#define WELCOME_MSG "Doug's MIDI console v6\r\n"
#define MAIN_MENU   "Options:\r\n" \
                     "\t1. Toggle LD1 Green LED\r\n" \
                     "\t2. Read USER BUTTON status\r\n" \
                     "\t4. Print counters\r\n" \
                     "\tqw. Pause/start sound\r\n" \
                     "\t(. Use all mem\r\n" \
                     "\t). Stack overflow\r\n" \
                     "\t~. Print this message"
#define PROMPT "\r\n> "
#define NOTE_ON_START  "\x90\x3C\x40"
#define NOTE_OFF_START "\x80\x3C\x40"

#define I2S_BUFFER_SIZE 128

uint8_t NOTE_ON[] = NOTE_ON_START;
uint8_t NOTE_OFF[] = NOTE_OFF_START;

// From main.c
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart6;
extern I2S_HandleTypeDef hi2s1;

static uint32_t overrun_errors = 0;
static uint32_t uart_error_callbacks = 0;
static uint32_t usart3_interrupts = 0;
static uint32_t midi_overrun_errors = 0;
static uint32_t loops_per_tick;

// I/O buffers: Serial and MIDI, in & out
FAST_BSS char s_i_buff[16];
FAST_BSS ring_buffer_t s_i_rb;
FAST_BSS char m_i_buff[16];
FAST_BSS ring_buffer_t m_i_rb;
FAST_BSS char s_o_buff[256];
FAST_BSS ring_buffer_t s_o_rb;
FAST_BSS char m_o_buff[32];
FAST_BSS ring_buffer_t m_o_rb;

// MIDI input parsers
FAST_BSS midi_stream midi_stream_0;

// Test Fast Data
FAST_DATA char test_fast_string[] = "This is a fast string test.";
FAST_DATA size_t tfs_len = sizeof(test_fast_string) - 1;

// Tone Generator
FAST_DATA tonegen_state tonegen1;

// I2S output buffer for DMA
// TODO: Move to SRAM2 which will only be used for DMA
int16_t i2s_buff[I2S_BUFFER_SIZE];
// Which half of the buffer are we writing to
// next?
FAST_DATA static volatile int16_t *i2s_buff_write;
// Can we write the next half of the buffer now?
FAST_DATA int i2s_write_available;

/** Set up all our i/o buffers */
void init_ring_buffers() {
  ring_buffer_init(&s_i_rb, s_i_buff, sizeof(s_i_buff));
  ring_buffer_init(&s_o_rb, s_o_buff, sizeof(s_o_buff));
  ring_buffer_init(&m_i_rb, m_i_buff, sizeof(m_i_buff));
  ring_buffer_init(&m_o_rb, m_o_buff, sizeof(m_o_buff));
}

/** initialize our MIDI parsers */
void init_midi_buffers() {
  midi_stream_init(&midi_stream_0);
}

/** Read any waiting input from this USART and stick it in the
 * input ring_buffer. Write any output to the USART if it is ready.
 */
void check_uart(USART_TypeDef *usart, ring_buffer_t *in_rb, ring_buffer_t *out_rb) {
  char c;

  // Check for serial output waiting to go
  if (ring_buffer_num_items(out_rb) > 0) {
    // Check if we can send the item now
    if (LL_USART_IsActiveFlag_TXE(usart)) {
      ring_buffer_dequeue(out_rb, &c);
      LL_USART_TransmitData8(usart, c);
    }
  }

  // Check for serial input waiting to be read
  if (LL_USART_IsActiveFlag_RXNE(usart)) {
    c = LL_USART_ReceiveData8(usart);
    ring_buffer_queue(in_rb, c);
  }
}

/** If there is input ready, pull it into our input buffers.
 * If there is output waiting, send it when possible.
 * If the input buffers are full, increase counters.
 */
void check_io() {
  // Serial port
  check_uart(huart3.Instance, &s_i_rb, &s_o_rb);
  // MIDI port
  check_uart(huart6.Instance, &m_i_rb, &m_o_rb);
}

/** Queues data to be sent over our serial output. */
void serial_transmit(const uint8_t *msg, uint16_t size) {
  // TODO: Check for send buffer overflow
  ring_buffer_queue_arr(&s_o_rb, (const char *)msg, (ring_buffer_size_t)size);
}

/** Queues data to be sent over our serial output. */
void midi_transmit(const uint8_t *msg, uint16_t size) {
  // TODO: Check for send buffer overflow
  ring_buffer_queue_arr(&m_o_rb, (const char *)msg, (ring_buffer_size_t)size);
}


/** Returns >= 256 if there is nothing to be read;
 * otherwise returns a uint8_t of what is next to be read.
 */
uint16_t serial_read() {
  uint8_t c;

  if (!ring_buffer_dequeue(&s_i_rb, (char *)&c)) {
    return 256;
  }
  return c;
}

void printWelcomeMessage(void) {
  serial_transmit((uint8_t *)"\r\n\r\n", strlen("\r\n\r\n"));
  serial_transmit((uint8_t *)">>>", 3);
  serial_transmit((uint8_t *)test_fast_string, tfs_len);
  serial_transmit((uint8_t *)"<<<\r\n", 5);
  serial_transmit((uint8_t *)WELCOME_MSG, strlen(WELCOME_MSG));
  serial_transmit((uint8_t *)MAIN_MENU, strlen(MAIN_MENU));
}


static int prompted = 0;

/** Prompts for input for each input.
 * Returns the ASCII code received, or 0 if no character received.
 * Yes, that makes it ambiguous on receiving an ASCII NUL.
 */
uint8_t read_user_input(void) {
  uint16_t c;

  if (!prompted) {
    serial_transmit((uint8_t*)PROMPT, strlen(PROMPT));
    prompted = 1;
  }

  c = serial_read();

  // No character waiting for input
  if (c >= 0x100) {
    return 0;
  }

  prompted = 0;
  return (uint8_t)c;
}

/** intentionally overflow the stack to see what happens */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"
void stack_overflow_test(void) {
  stack_overflow_test();
}
#pragma GCC diagnostic pop

/** Allocate memory until we get to ENOMEM */
void alloc_test() {
  char msg[40];
  uint32_t t;

  void *m;
  size_t amount = 10240;

  do {
    m = malloc(amount);
    if (NULL == m) {
      snprintf(msg, sizeof(msg) - 1, "OOM: %d; amt: %u\r\n", errno, amount);
      amount >>= 1;
    } else {
      snprintf(msg, sizeof(msg) - 1, "Addr: %08lX; amt: %u\r\n", (uint32_t)m, amount);
    }
    serial_transmit((uint8_t *)msg, strlen(msg));

    // Send I/O and delay before doing this again
    t = HAL_GetTick();
    do {
      check_io();
    } while (HAL_GetTick() < t + 100);
  } while (m != NULL || amount > 0);
}

/** Interprets numbers as menu options.
 * Interprets letters as notes to send via MIDI.
 * Ignores the rest.
 * Returns 0 on no valid input.
 * Returns 1 on processed input.
 * Returns 2 if we should re-display the menu.
 */
uint8_t process_user_input(uint8_t opt) {
  int l;

  if (opt == 0) {
    return 0;
  }

  char msg[40];

  serial_transmit(&opt, 1);

  switch (opt) {
  case '1':
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    break;
  case '2':
    l = snprintf(msg, sizeof(msg) - 1, "\r\nUSER BUTTON status: %s",
                  HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) != GPIO_PIN_RESET ? "PRESSED" : "RELEASED");
    serial_transmit((uint8_t*)msg, l);
    break;
  case '4':
    l = snprintf(msg, sizeof(msg) - 1, "\r\nUA3I: %lu, ORE: %lu, MIDI_ORE: %lu, ",
                  usart3_interrupts, overrun_errors, midi_overrun_errors);
    serial_transmit((uint8_t*)msg, l);
    l = snprintf(msg, sizeof(msg) - 1, "LPT: %lu\r\n", loops_per_tick);
    serial_transmit((uint8_t*)msg, l);
    break;
  case 'q':
    // (+) Pause the DMA Transfer using HAL_I2S_DMAPause()
    HAL_I2S_DMAPause(&hi2s1);
    break;
  case 'w':
    // (+) Resume the DMA Transfer using HAL_I2S_DMAResume()
    HAL_I2S_DMAResume(&hi2s1);
    break;
  case '(':
    // Use all memory
    alloc_test();
    break;
  case ')':
    stack_overflow_test();
    break;
  case '~':
  case '`':
    // Re-print the options
    return 2;
  default:
    return 0;
  }

  return 1;
}

/** Reads from our MIDI input ring buffer.
 * Returns 0-255 when byte received.
 * Returns 256+ when no byte received.
 */
uint16_t read_midi(void) {
  uint8_t c;

  if (!ring_buffer_dequeue(&m_i_rb, (char *)&c)) {
    // No input available
    return (uint16_t)0x100U;
  }
  return (uint16_t)c;
}

/**
 * Processes a received MIDI byte.
 * For now, just print it (which will certainly lead to overrun).
 */
void process_midi(uint8_t midi_byte) {
  char msg[64];
  midi_message mm;

  // Don't print every byte anymore
  /*
  snprintf(msg, sizeof(msg) - 1, "\r\nMIDI byte: %02X\r\n", midi_byte);
  serial_transmit((uint8_t *)msg, strlen(msg));
  */

  if (midi_stream_receive(&midi_stream_0, midi_byte, &mm)) {
    // Received a full MIDI message
    midi_snprintf(msg, sizeof(msg) - 1, &mm);
    serial_transmit((uint8_t *)msg, strlen(msg));
    serial_transmit((uint8_t *)"\r\n", 2);
  }
}

// I2S Callbacks ///////////////////////////////////////////////////////////////

/** We have transmitted half the data; we can now re-fill the front
 * half of the buffer.
 */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
  i2s_buff_write = i2s_buff;
  i2s_write_available = 1;
}

/** We've transmitted all the data; we can start filling the
 * second half.
 */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  i2s_buff_write = &i2s_buff[I2S_BUFFER_SIZE / 2];
  i2s_write_available = 1;
}

/** Fill our send buffer with the next BUFFER_SIZE / 2
 * amount of stuff to do.
 */
void fill_i2s_data() {
  int16_t *next_sample_loc = (int16_t *)i2s_buff_write; // Remove volatility

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 1); // Red LED

  for (int i = 0; i < I2S_BUFFER_SIZE / 2; i++) {
    *next_sample_loc = tonegen_next_sample(&tonegen1);
    next_sample_loc++;
  }

  // TODO: Deal with race condition - what if the buffer empties
  // while we're doing this? Should we set the flag to 0 at the
  // very start?
  i2s_write_available = 0;
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 0); // Red LED
}

///////////////////////////////////////////////////////////////////////////////

// When > 127, no current note
static uint8_t current_midi_note;

/** Reads any pending MIDI inputs.
 *
 * For note On: Sets the frequency and amplitude and switches
 * the tone generator to that. Records the current playing note number.
 *
 * For note Off: If currently playing note number, turns it off.
 * Otherwise ignores it.
 */
void check_midi_synth() {
  uint16_t midi_in = read_midi();
  midi_message mm;
  char msg[64];

  if (midi_in <= 255) {
    if (midi_stream_receive(&midi_stream_0, midi_in, &mm)) {
      // Received a full MIDI message

      // Update our notes playing
      if ((mm.type & 0xF0) == MIDI_NOTE_ON) {
        current_midi_note = mm.note;
        if (current_midi_note > 127) current_midi_note = 127;
        // TODO: Set amplitude by velocity
        tonegen_set(&tonegen1, midi_note_freqX100[current_midi_note] / 100, mm.velocity * 250);
      } else if ((mm.type & 0xF0) == MIDI_NOTE_OFF) {
        if (current_midi_note == mm.note) {
          tonegen_set(&tonegen1, tonegen1.desired_freq, 0);
        }
      }

      // And show what we received
      midi_snprintf(msg, sizeof(msg) - 1, &mm);
      serial_transmit((uint8_t *)msg, strlen(msg));
      serial_transmit((uint8_t *)"\r\n", 2);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void realmain() {
  uint16_t opt = 0;
  uint32_t last_overrun_errors = overrun_errors;
  uint32_t last_usart3_interrupts = usart3_interrupts;
  char msg[36];
  uint32_t counter = 0;
  const uint32_t end_counter = 1000000;
  uint8_t processed_input;
  uint32_t cur_tick;
  uint32_t last_tick = HAL_GetTick();
  uint32_t tick_counter = 0;

  // test_i2s();

  init_ring_buffers();
  init_midi_buffers();
  tonegen_init(&tonegen1, 32000);
  tonegen_set(&tonegen1, 1024, 0); // Frequency, Amplitude

  // Start the DMA streams for I²S
  // HAL_I2S_Transmit_DMA(&hi2s1, triangle_wave, sizeof(triangle_wave) / sizeof(triangle_wave[0]));
  HAL_I2S_Transmit_DMA(&hi2s1, (uint16_t *)i2s_buff, I2S_BUFFER_SIZE);

  printMessage:
  printWelcomeMessage();

  while (1) {
    // Always check for I/O available for read/write
    check_io();

    // Handle our MIDI state machine
    check_midi_synth();

    if (i2s_write_available) {
      fill_i2s_data();
    }

    // Now do everything in an entirely non-blocking way
    opt = read_user_input();
    processed_input = process_user_input(opt);

    // Put a dot every N (million) times through this loop
    counter++;
    if (counter >= end_counter) {
      counter = 0;
      serial_transmit((uint8_t *)".", 1);
    }

    // count how many times through the loop we get per tick
    cur_tick = HAL_GetTick();
    if (cur_tick != last_tick) {
      // As of this version of my code, this is usually 114 loops per tick,
      // which means about 114 kHz through this loop. Not bad.
      loops_per_tick = tick_counter;
      tick_counter = 0;
      last_tick = cur_tick;
    } else {
      tick_counter++;
    }

    if (overrun_errors != last_overrun_errors) {
      snprintf(msg, sizeof(msg) - 1, "\r\nORE: %lu\r\n", overrun_errors);
      last_overrun_errors = overrun_errors;
      serial_transmit((uint8_t *)msg, strlen(msg));
    }
    if (usart3_interrupts != last_usart3_interrupts) {
      snprintf(msg, sizeof(msg) - 1, "\r\nUA3I: %lu\r\n", usart3_interrupts);
      last_usart3_interrupts = usart3_interrupts;
      serial_transmit((uint8_t *)msg, strlen(msg));
    }

    if (processed_input == 2) {
      goto printMessage;
    }
  }
} // realmain()



////////////////////////////////////////////////////////////////////////////////////////
// Callbacks


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  // We just want to clear an overrun error and acknowledge it happened.
  // SEE: https://electronics.stackexchange.com/questions/376104/smt32-hal-uart-crash-on-possible-overrun
  // SEE: https://github.com/micropython/micropython/issues/3375


  if (huart == &huart3) {
    // Serial terminal via ST-Link
    usart3_interrupts++;

    // The earlier code will have set the HAL_UART_ERROR_ORE flag IF an OverRunError happened.
    // See the code commented: UART Over-Run interrupt occurred

    if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
      uart_error_callbacks++;
    }

  } else if (huart == &huart6) {
    // MIDI
    if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
      midi_overrun_errors++;
    }
    // midi_overrun_errors++;
  }
  // Re-enable the interrupts (not sure if this is necessary)
  // This did not work when in the "if" above (it never got invoked).
  __HAL_UART_ENABLE_IT(huart, UART_IT_ERR);
}

