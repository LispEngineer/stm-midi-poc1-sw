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

void realmain();

#endif /* INC_REALMAIN_H_ */
