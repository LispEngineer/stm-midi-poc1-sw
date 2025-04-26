/*
 * realmain.h
 *
 *  Created on: Aug 25, 2024
 *      Author: Doug
 */

#ifndef INC_REALMAIN_H_
#define INC_REALMAIN_H_

#define USE_FAST_MEMORY
#ifdef USE_FAST_MEMORY
// When we set up our memory map, use these
#  define FAST_BSS __attribute((section(".fast_bss")))
#  define FAST_DATA __attribute((section(".fast_data")))
#else // Don't USE_FAST_MEMORY
#  define FAST_BSS
#  define FAST_DATA
#endif

#define USE_DMA_MEMORY
#ifdef USE_DMA_MEMORY
// When we set up our memory map, use these
#  define DMA_BSS __attribute((section(".dma_bss")))
#  define DMA_DATA __attribute((section(".dma_data")))
#else // Don't USE_DMA_MEMORY
#  define DMA_BSS
#  define DMA_DATA
#endif

////////////////////////////////////////////////////////
// UART port definitions

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


////////////////////////////////////////////////////////
// Functions



// Main entry point
void realmain();

// USART functions
void init_usart_dma_io();
size_t serial_transmit(const uint8_t *msg, uint16_t size);
void check_io();
size_t midi_transmit(const uint8_t *msg, uint16_t size);
uint16_t serial_read();


// Application functions
void printWelcomeMessage(void);
uint8_t read_user_input(void);


#endif /* INC_REALMAIN_H_ */
