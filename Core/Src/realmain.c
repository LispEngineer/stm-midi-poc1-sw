/*
 * realmain.c
 * Since the main.c is auto-generated by STM32CubeIDE
 * this is the real "main" file for stuff that is not
 * auto-generated.
 *
 *  Created on: 2024-08-25
 *  Updated on: 2025-01-18
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024-2025, Douglas P. Fields, Jr.
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
#include "spidma.h"
#include "spidma_ili9341.h"
#include "fonts.h"
#include "usartdma.h"

#ifdef USE_FAST_MEMORY
// When we set up our memory map, use these
#  define FAST_BSS __attribute((section(".fast_bss")))
#  define FAST_DATA __attribute((section(".fast_data")))
#else // Don't USE_FAST_MEMORY
#  define FAST_BSS
#  define FAST_DATA
#endif

#define SOFTWARE_VERSION "18"

#define WELCOME_MSG "Doug's MIDI v" SOFTWARE_VERSION "\r\n"
#define MAIN_MENU   "\t123. Toggle R/G/B LED\r\n" \
                     "\t4/5. Read BTN1/2\r\n" \
                     "\t6.   Counters\r\n" \
                     "\t7.   SPI info\r\n" \
                     "\tqw.  Pause/start I2S\r\n" \
                     "\ter.  Start/stop tone\r\n" \
                     "\tdf.  Send note on/off\r\n" \
                     "\ta.   Audio mute\r\n" \
                     "\tg/G. Gain 0/1\r\n" \
                     "\tx.   Reinit UARTs\r\n" \
                     "\t(.   Mem\r\n" \
                     "\t).   Stack\r\n" \
                     "\t~.   Menu"
#define PROMPT "\r\n> "
#define NOTE_ON_START  "\x90\x3C\x40"
#define NOTE_ON_START_LEN 3
#define NOTE_OFF_START "\x80\x3C\x40"
#define NOTE_OFF_START_LEN 3

// This is using LL API
#if 1
#define MIDI1_UART          USART1 // Low level USART - HAL would be huartN
#define MIDI1_DMA_RX        DMA2
#define MIDI1_DMA_RX_STREAM LL_DMA_STREAM_2
#define MIDI1_DMA_TX        DMA2
#define MIDI1_DMA_TX_STREAM LL_DMA_STREAM_7
#elif 0
#define MIDI1_UART          USART3 // Low level USART - HAL would be huartN
#define MIDI1_DMA_RX        DMA1
#define MIDI1_DMA_RX_STREAM LL_DMA_STREAM_1
#define MIDI1_DMA_TX        DMA1
#define MIDI1_DMA_TX_STREAM LL_DMA_STREAM_3
#else
#define MIDI1_UART          USART6 // Low level USART - HAL would be huartN
#define MIDI1_DMA_RX        DMA2
#define MIDI1_DMA_RX_STREAM LL_DMA_STREAM_1
#define MIDI1_DMA_TX        DMA2
#define MIDI1_DMA_TX_STREAM LL_DMA_STREAM_6
#endif

// This is using LL API
#define CONSOLE_UART          USART2 // Low level USART - HAL would be huart2
#define CONSOLE_DMA_RX        DMA1
#define CONSOLE_DMA_TX        DMA1
#define CONSOLE_DMA_RX_STREAM LL_DMA_STREAM_5
#define CONSOLE_DMA_TX_STREAM LL_DMA_STREAM_6

// This is using HAL API
#define I2S_BUFFER_SIZE 256
#define SOUND1          hi2s1

// This is using HAL API
#define DISPLAY_SPI  hspi2
#define DISPLAY_DMA  hdma_spi2_tx
// TODO: Multiplex this with the Touch Screen SPI?

// From main.c
// These two are needed if we use HAL UARTs
//extern UART_HandleTypeDef CONSOLE_UART; // Serial Console
//extern UART_HandleTypeDef MIDI1_UART; // MIDI 1
extern I2S_HandleTypeDef SOUND1;
extern SPI_HandleTypeDef DISPLAY_SPI;
extern DMA_HandleTypeDef DISPLAY_DMA;

const uint8_t NOTE_ON[] = NOTE_ON_START;
const uint8_t NOTE_OFF[] = NOTE_OFF_START;

static uint32_t overrun_errors = 0;
// static uint32_t uart_error_callbacks = 0;
static uint32_t usart3_interrupts = 0;
static uint32_t midi_overrun_errors = 0;
static uint32_t loops_per_tick;
static uint32_t midi_received = 0; // For receive interrupts

// MIDI1 I/O buffers
FAST_BSS uint8_t m1_o_buff1[32];
FAST_BSS uint8_t m1_o_buff2[sizeof(m1_o_buff1)];
FAST_BSS uint8_t m1_i_buff[32];
usart_dma_config_t midi1_io; // MIDI Input Receive

// Console I/O
FAST_BSS uint8_t c_i_buff[32];
FAST_BSS uint8_t c_o_buff1[512];
FAST_BSS uint8_t c_o_buff2[sizeof(c_o_buff1)];
usart_dma_config_t console_io;

// MIDI input parsers
FAST_BSS midi_stream midi_stream_0;

// Test Fast Data
FAST_DATA char test_fast_string[] = "Fast string!";
FAST_DATA size_t tfs_len = sizeof(test_fast_string) - 1;

// Tone Generator
FAST_BSS tonegen_state tonegen1;

// Display controller (SPI & DMA & GPIO)
FAST_BSS spidma_config_t spi_config;
spidma_config_t *spip;

// I2S output buffer for DMA
// TODO: Move to SRAM2 which will only be used for DMA
int16_t i2s_buff[I2S_BUFFER_SIZE];
// Which half of the buffer are we writing to
// next?
FAST_DATA static volatile int16_t *i2s_buff_write;
// Can we write the next half of the buffer now?
FAST_DATA int i2s_write_available;

void init_usart_dma_io() {
  // MIDI Port
  midi1_io.usartx = MIDI1_UART;
  midi1_io.dma_rx = MIDI1_DMA_RX;
  midi1_io.dma_rx_stream = MIDI1_DMA_RX_STREAM;
  midi1_io.dma_tx = MIDI1_DMA_TX;
  midi1_io.dma_tx_stream = MIDI1_DMA_TX_STREAM;
  midi1_io.rx_buf = m1_i_buff;
  midi1_io.rx_buf_sz = sizeof(m1_i_buff);
  midi1_io.tx_buf1 = m1_o_buff1;
  midi1_io.tx_buf2 = m1_o_buff2;
  midi1_io.tx_buf_sz = sizeof(m1_o_buff1);

  // TODO: Check return value
  udcr_init(&midi1_io);

  // Console
  console_io.usartx = CONSOLE_UART;
  console_io.dma_rx = CONSOLE_DMA_RX;
  console_io.dma_tx = CONSOLE_DMA_TX;
  console_io.dma_rx_stream = CONSOLE_DMA_RX_STREAM;
  console_io.dma_tx_stream = CONSOLE_DMA_TX_STREAM;
  console_io.rx_buf = c_i_buff;
  console_io.rx_buf_sz = sizeof(c_i_buff);
  console_io.tx_buf1 = c_o_buff1;
  console_io.tx_buf2 = c_o_buff2;
  console_io.tx_buf_sz = sizeof(c_o_buff1);

  // TODO: Check return value
  udcr_init(&console_io);
}

/** initialize our MIDI parsers */
void init_midi_buffers() {
  midi_stream_init(&midi_stream_0);
}

/** Queues data to be sent over our serial output.
 * Returns # of bytes queued to send.
 */
static inline size_t serial_transmit(const uint8_t *msg, uint16_t size) {
  // TODO: Check for send buffer overflow - if sent < size
  size_t sent = udcr_queue_bytes(&console_io, msg, size);
  return sent;
}

/*
 * Serial outputs are submitted for DMA send if possible.
 * Serial inputs are handled by DMA and not here anymore.
 */
void check_io() {
  // Serial port
  udcr_send_from_queue(&console_io);

  // MIDI port
  udcr_send_from_queue(&midi1_io);
}

/** Queues data to be sent over our MIDI output.
 * Returns bytes queued. */
static inline size_t midi_transmit(const uint8_t *msg, uint16_t size) {
  // TODO: Check for send buffer overflow
  return udcr_queue_bytes(&midi1_io, msg, size);
}

/** Returns >= 256 if there is nothing to be read;
 * otherwise returns a uint8_t of what is next to be read.
 */
static inline uint16_t serial_read() {
  return udcr_read_byte(&console_io);
}

void printWelcomeMessage(void) {
  serial_transmit((uint8_t *)"\r\n\r\n", strlen("\r\n\r\n"));
  serial_transmit((uint8_t *)">>>", 3);
  serial_transmit((uint8_t *)test_fast_string, tfs_len);
  serial_transmit((uint8_t *)"<<<\r\n", 5);
  serial_transmit((uint8_t *)WELCOME_MSG, strlen(WELCOME_MSG));
  serial_transmit((uint8_t *)MAIN_MENU, strlen(MAIN_MENU));
}

static void print_spi_queue_info(spidma_config_t *spi) {
  char buf[200];
  snprintf(buf, sizeof(buf),
              "ql: %d, qr: %d, sdqf: %lu, iliqf: %lu, ilics: %lu; "
              "fqf: %lu, bfqf: %lu; "
              "maf: %lu, sz: %u; ili_allocs: %lu, sd_frees: %lu\r\n",

              spidma_queue_length(spi), spidma_queue_remaining(spi),
              spi->entry_queue_failures,
              ili_queue_failures, ili_characters_skipped,

              spi->free_queue_failures, spi->backup_free_queue_failures,

              ili_mem_alloc_failures, ili_last_alloc_failure_size,
              ili_mem_allocs, spi->mem_frees);
  serial_transmit((uint8_t *)buf, strlen(buf));
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
  char msg[50];
  uint32_t t;

  void *m;
  size_t amount = 10240;
  size_t total = 0;

  do {
    m = malloc(amount);
    if (NULL == m) {
      snprintf(msg, sizeof(msg) - 1, "OOM: %d; amt: %u\r\n", errno, amount);
      amount >>= 1;
    } else {
      total += amount;
      snprintf(msg, sizeof(msg) - 1, "Addr: %08lX; amt: %u; total: %lx\r\n", (uint32_t)m, amount, (uint32_t)total);
    }
    serial_transmit((uint8_t *)msg, strlen(msg));

    // Send I/O and delay before doing this again
    t = HAL_GetTick();
    do {
      check_io();
    } while (HAL_GetTick() < t + 100);
  } while (m != NULL || amount > 0);
}

void clear_uart_flags() {
  char msg[64];
  snprintf(msg, sizeof(msg), "LBD: %c, PE: %c, NE: %c, ORE: %c, IDLE: %c",
           LL_USART_IsActiveFlag_LBD(MIDI1_UART) ? 'A' : ' ',
           LL_USART_IsActiveFlag_PE(MIDI1_UART) ? 'A' : ' ',
           LL_USART_IsActiveFlag_NE(MIDI1_UART) ? 'A' : ' ',
           LL_USART_IsActiveFlag_ORE(MIDI1_UART) ? 'A' : ' ',
           LL_USART_IsActiveFlag_IDLE(MIDI1_UART) ? 'A' : ' ');
  serial_transmit((uint8_t *)msg,  strlen(msg));

  LL_USART_ClearFlag_LBD(MIDI1_UART);
  LL_USART_ClearFlag_PE(MIDI1_UART);
  LL_USART_ClearFlag_NE(MIDI1_UART);
  LL_USART_ClearFlag_ORE(MIDI1_UART);
  LL_USART_ClearFlag_IDLE(MIDI1_UART);
}

/*
 * Handle interrupts from a USART. But it doesn't really tell us
 * what generated the interrupt!
 *
 * This does nothing as USART interrupts should be off.
 *
 * NOTE: Put this in FAST RAM.
 */
void ll_usart_interrupt_handler(USART_TypeDef *u) {
  // We don't do anything anymore, all I/O should be
  // DMA driven now and all interrupts should be
  // DMA driven as well.

  if (LL_USART_IsActiveFlag_RXNE(u)) {
    // Receive is not empty - clear receive by receiving it
	// We discard it now
    LL_USART_ReceiveData8(u);

  } else if (LL_USART_IsActiveFlag_ORE(u)) {
    // Over run error
    LL_USART_ClearFlag_ORE(u);
    overrun_errors++;

  } else if (LL_USART_IsActiveFlag_PE(u)) {
    LL_USART_ClearFlag_PE(u);
    // TODO: Increase parity errors

  } else if (LL_USART_IsActiveFlag_FE(u)) {
    // Framing error
    LL_USART_ClearFlag_FE(u);
    // TODO: Increase framing errors
  }
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

  char msg[100];

  serial_transmit(&opt, 1);

  switch (opt) {
  case '1':
    HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
    break;
  case '2':
    HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    break;
  case '3':
    HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
    break;
  case '4':
    l = snprintf(msg, sizeof(msg) - 1, "\r\nBTN1 status: %s",
                  // Button pressed pulls it down to 0
                  HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == GPIO_PIN_RESET ? "PRESSED" : "RELEASED");
    serial_transmit((uint8_t*)msg, l);
    break;
  case '5':
    l = snprintf(msg, sizeof(msg) - 1, "\r\nBTN2 status: %s",
                  // Button pressed pulls it down to 0
                  HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == GPIO_PIN_RESET ? "PRESSED" : "RELEASED");
    serial_transmit((uint8_t*)msg, l);
    break;
  case '6':
    l = snprintf(msg, sizeof(msg) - 1, "\r\nUA3I: %lu, ORE: %lu, MIDI_ORE: %lu, MIDI_RX: %lu, LPT: %lu\r\n",
                  usart3_interrupts, overrun_errors, midi_overrun_errors, midi_received, loops_per_tick);
    serial_transmit((uint8_t*)msg, l);
    break;
  case '7':
    print_spi_queue_info(spip);
    break;
  case 'a':
    HAL_GPIO_TogglePin(AUDIO_MUTE_GPIO_Port, AUDIO_MUTE_Pin);
    break;
  case 'q':
    // (+) Pause the DMA Transfer using HAL_I2S_DMAPause()
    HAL_I2S_DMAPause(&SOUND1);
    break;
  case 'w':
    // (+) Resume the DMA Transfer using HAL_I2S_DMAResume()
    HAL_I2S_DMAResume(&SOUND1);
    break;
  case 'e':
    // Start a tone
    tonegen_set(&tonegen1, 262, 25000);
    break;
  case 'r':
    // Stop tone
    tonegen_set(&tonegen1, 262, 0);
    break;
  case 'd':
    // Send note on
    midi_transmit(NOTE_ON, NOTE_ON_START_LEN);
    break;
  case 'f':
    // Send note off
    midi_transmit(NOTE_OFF, NOTE_OFF_START_LEN);
    break;
  case '(':
    // Use all memory
    alloc_test();
    break;
  case ')':
    stack_overflow_test();
    break;
  case 'x':
    // reinit_uarts();
    clear_uart_flags();
    break;
  case 'g':
    // FIXME: This is not working (not toggling)
    HAL_GPIO_TogglePin(HP_GAIN0_GPIO_Port, HP_GAIN0_Pin);
    l = snprintf(msg, sizeof(msg) - 1, "\r\nGAIN0 now: %d\r\n",
                 HAL_GPIO_ReadPin(HP_GAIN0_GPIO_Port, HP_GAIN0_Pin));
    serial_transmit((uint8_t *)msg, l);
    // TODO: Show gain status on display
    break;
  case 'G':
    HAL_GPIO_TogglePin(HP_GAIN1_GPIO_Port, HP_GAIN1_Pin);
    l = snprintf(msg, sizeof(msg) - 1, "\r\nGAIN1 now: %d\r\n",
                 HAL_GPIO_ReadPin(HP_GAIN1_GPIO_Port, HP_GAIN1_Pin));
    serial_transmit((uint8_t *)msg, l);
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
static inline uint16_t read_midi(void) {
  return udcr_read_byte(&midi1_io);
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

  for (int i = 0; i < I2S_BUFFER_SIZE / 2; i++) {
    *next_sample_loc = tonegen_next_sample(&tonegen1);
    next_sample_loc++;
  }

  // TODO: Deal with race condition - what if the buffer empties
  // while we're doing this? Should we set the flag to 0 at the
  // very start?
  i2s_write_available = 0;
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
  char msg[91];
  int msg_len;

  if (midi_in <= 0xFF) {
    /*
    snprintf(msg, sizeof(msg), "_%02X_", midi_in);
    serial_transmit((uint8_t *)msg, strlen(msg));
    */
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

      if (mm.type != 0xF8 && mm.type != 0xFE) {
        // And show what we received if it's not a clock message or Active Sensing message
        memset(msg, ' ', sizeof(msg));
        msg_len = midi_snprintf(msg, sizeof(msg) - 1, &mm);
        serial_transmit((uint8_t *)msg, strlen(msg));
        serial_transmit((uint8_t *)"\r\n", 2);
        // Extend message to full length
        msg[msg_len] = ' ';
        msg[sizeof(msg) - 1] = '\0';

        // And display it on the screen
        // Instead of filling a rectangle and then writing the string:
        // Always expand the string to the necessary length (2 full rows?) so it draws it correctly.
        // 320w x 240h
        // 320 at 7 per character = 45 wide
        // so we need 91 character long message including terminal NUL
        // This looks much better as there is no "black fill flicker"
        // spidma_ili9341_fill_rectangle(spip, 0, 0, ILI9341_WIDTH, Font_7x10.height * 2, ILI9341_BLACK);
        spidma_ili9341_write_string(spip, 0, 0, msg, Font_7x10, ILI9341_CYAN, ILI9341_BLACK);
      }
    }
  }
}


/*
 * Initialize our SPI/DMA display driver subsystem.
 * If initialziation fails, we go into an infinite loop
 * after sending a message on the serial console.
 */
void display_init() {
  // Configure new DMA driver
  spi_config.bank_cs = SPI2_CS_GPIO_Port;
  spi_config.pin_cs = SPI2_CS_Pin;
  spi_config.bank_dc = SPI2_DC_GPIO_Port;
  spi_config.pin_dc = SPI2_DC_Pin;
  spi_config.bank_reset = SPI2_RESET_GPIO_Port;
  spi_config.pin_reset = SPI2_RESET_Pin;
  spi_config.use_cs = 1;
  spi_config.use_reset = 1;
  spi_config.spi = &DISPLAY_SPI;
  spi_config.dma_tx = &DISPLAY_DMA;

  spip = &spi_config;

  if (spidma_init(spip) != SDRV_OK) {
    const char *msg = "spidma_init failed\r\n";
    for (int i = 0; i < strlen(msg); i++) {
      LL_USART_TransmitData8(CONSOLE_UART, (uint16_t)msg[i]);
    }
    while (1) {}
  }

  spidma_ili9341_init(spip);
  spidma_empty_queue(spip);
}

/*
 * Displays my personal logo and information upon startup for a few seconds.
 */
void show_intro(spidma_config_t *spi) {
  const char *msg = "Douglas P. Fields, Jr.";
  size_t len = strlen(msg);
  FontDef font = Font_11x18;
  uint16_t x, y;

  // TODO: Add my logo to the top with white parentheses and purple lambda

  spidma_queue(spi, SPIDMA_SELECT, 0, NULL, 1000000);
  spidma_ili9341_fill_screen(spi, ILI9341_BLACK);
  x = (ILI9341_WIDTH - len * font.width) >> 1;
  y = (ILI9341_HEIGHT - font.height) >> 1;
  spidma_ili9341_write_string(spi, x, y,
                              msg, font,
                              ILI9341_WHITE, ILI9341_BLACK);
  spidma_empty_queue(spi);

  msg = "symbolics@lisp.engineer";
  len = strlen(msg);
  y += font.height;
  x = (ILI9341_WIDTH - len * font.width) >> 1;
  spidma_ili9341_write_string(spi, x, y,
                              msg, font,
                              ILI9341_YELLOW, ILI9341_BLACK);
  spidma_empty_queue(spi);

  msg = "STM32 Synth EVT#2";
  len = strlen(msg);
  y += font.height * 2;
  x = (ILI9341_WIDTH - len * font.width) >> 1;
  spidma_ili9341_write_string(spi, x, y,
                              msg, font,
                              ILI9341_GREEN, ILI9341_BLACK);
  spidma_empty_queue(spi);

  msg = "Software v" SOFTWARE_VERSION;
  len = strlen(msg);
  y += font.height * 2;
  x = (ILI9341_WIDTH - len * font.width) >> 1;
  spidma_ili9341_write_string(spi, x, y,
                              msg, font,
                              ILI9341_MAGENTA, ILI9341_BLACK);
  spidma_empty_queue(spi);

  // HAL_Delay(5000);
}

uint32_t last_mute = 776655; // Pick a value not GPIO_PIN_RESET or _SET
uint32_t last_gain0 = 776655;
uint32_t last_gain1 = 776655;

/* Draws "MUTED" or not on the screen.
 * Also draws the gain state on the screen.
 * Only draws these things when they change.
 */
void draw_mute() {
  GPIO_PinState am = HAL_GPIO_ReadPin(AUDIO_MUTE_GPIO_Port, AUDIO_MUTE_Pin);
  GPIO_PinState g0 = HAL_GPIO_ReadPin(HP_GAIN0_GPIO_Port, HP_GAIN0_Pin);
  GPIO_PinState g1 = HAL_GPIO_ReadPin(HP_GAIN1_GPIO_Port, HP_GAIN1_Pin);

  if (am != last_mute || g0 != last_gain0 || g1 != last_gain1) {
    last_mute = am;
    last_gain0 = g0;
    last_gain1 = g1;
    if (am == GPIO_PIN_RESET) {
      spidma_ili9341_write_string(spip, 0, Font_11x18.height * 2,
                                  "muted", Font_11x18,
                                  ILI9341_RED, ILI9341_BLACK);
    } else {
      spidma_ili9341_write_string(spip, 0, Font_11x18.height * 2,
                                  ".....", Font_11x18,
                                  ILI9341_GREEN, ILI9341_BLACK);
    }

    spidma_ili9341_write_string(spip, Font_11x18.width * 7, Font_11x18.height * 2,
                                g1 ? "1" : "0", Font_11x18, ILI9341_GREEN, ILI9341_BLACK);
    spidma_ili9341_write_string(spip, Font_11x18.width * 8, Font_11x18.height * 2,
                                g0 ? "1" : "0", Font_11x18, ILI9341_GREEN, ILI9341_BLACK);
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

  init_usart_dma_io();
  init_midi_buffers();
  tonegen_init(&tonegen1, 32000);
  tonegen_set(&tonegen1, 1024, 0); // Frequency, Amplitude

  // Start our USART receiving and error interrupts
  LL_USART_EnableIT_RXNE(MIDI1_UART);
  LL_USART_EnableIT_RXNE(CONSOLE_UART);
  LL_USART_EnableIT_ERROR(MIDI1_UART);
  LL_USART_EnableIT_ERROR(CONSOLE_UART);
  // LL_USART_EnableIT_PE(MIDI1_UART);
  // LL_USART_EnableIT_PE(CONSOLE_UART);

  display_init();
  show_intro(spip);

  // Start the DMA streams for I²S
  HAL_I2S_Transmit_DMA(&SOUND1, (uint16_t *)i2s_buff, I2S_BUFFER_SIZE);

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

    // Handle our display
    spidma_check_activity(spip);

    // Update our mute display and gain status
    draw_mute();
    // TODO: Display state of I2S
    // TODO: Display state of tone generator

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

/*
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  // We just want to clear an overrun error and acknowledge it happened.
  // SEE: https://electronics.stackexchange.com/questions/376104/smt32-hal-uart-crash-on-possible-overrun
  // SEE: https://github.com/micropython/micropython/issues/3375


  if (huart == &CONSOLE_UART) {
    // Serial terminal via ST-Link
    usart3_interrupts++;

    // The earlier code will have set the HAL_UART_ERROR_ORE flag IF an OverRunError happened.
    // See the code commented: UART Over-Run interrupt occurred

    if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
      uart_error_callbacks++;
    }

  } else if (huart == &MIDI1_UART) {
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
*/
