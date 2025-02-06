# Doug's STM32 MIDI Synth Proof of Concept 1 - Software

* Author: [Douglas P. Fields, Jr.](mailto:symbolics@lisp.engineer)
* Copyright 2024, Douglas P. Fields Jr.
* License: [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0.txt)
* Last updated: 2025-01-26
* [Repo Self-Link](https://github.com/LispEngineer/stm-midi-poc1-sw)
* Portions used under other licenses
  * SPI Display - MIT License
  * STM Software / Generated Code - Delegated to LICENSE file, hence
    Apache 2.0
  * Memory allocator - Public Domain (memmgr.c/h)

Software for my 
[STM32 MIDI Synthesizer PoC 1](https://github.com/LispEngineer/stm-midi-poc1).


# Credits

* [SPI Display Software](https://github.com/afiskon/stm32-ili9341) 
  by Aleksander Alekseev under [MIT License](https://opensource.org/license/mit)
  * As enhanced by [my project](https://github.com/LispEngineer/stm-f7-spi-display)


# Software

* Windows 11
* STM32CubeIDE
* STM32 HAL libraries


# Hardware

* STM32F722RET6
  * [STM32F7x2 Overview](https://www.st.com/en/microcontrollers-microprocessors/stm32f7x2.html)
  * [STM32F722RE Page](https://www.st.com/en/microcontrollers-microprocessors/stm32f722re.html)
  * [STM32F722RET6 Part](https://estore.st.com/en/stm32f722ret6-cpn.html)
  * [STM32F722 Data Sheet](https://www.st.com/resource/en/datasheet/stm32f722ic.pdf)
    * DS11853 Rev 9 July 2022 as of this writing
  * Package: LQFP-64
  * Core: ARM Cortex-M7
* Audio DAC with headphone amplifier
  * DAC: [PCM5102A](TODO)
  * Amplifier: [PAM8908JER](TODO)
* MIDI in/out
  * Opto-isolator: 
    * EVT1: [6N137S-TA1](TODO)
    * EVT2: [TLP2362](TODO) - much smaller
* Power: USB-C
* Programming: SWD (with 5-pin header, no SWO)

Key STM32F722 Manuals:
* Data sheet - DS11853 Rev 9
* Errata - ES0360 Rev 5
* Reference manual - RM0431 Rev 3
* Programming manual - PM0253 Rev 5
* System architecture & performance - AN4667 DocID027643 Rev 4
* L1 Cache - AN4839 Rev 2
* STM32F7 HAL and LL - UM1905 Rev 5
* STM32CubeMX - UM1718

External hardware:
* ST-Link v3: SWD & UART (USART2 console)
  * [STLINK-V3SET](https://www.st.com/en/development-tools/stlink-v3set.html)
* USB Console: Inland FT232 TTL UART to USB board (when not using ST-Link)
  * [Similar to this one](https://www.ebay.com/itm/142909573493)
* SPI Display: [PJRC ILI9341](https://www.pjrc.com/store/display_ili9341_touch.html)

## Misc References

* [Embedded C Coding Standard](https://barrgroup.com/embedded-systems/books/embedded-c-coding-standard)
* [STM32 Gotchas](http://www.efton.sk/STM32/gotcha/)
* [Demystifying Arm GNU Toolchain Specs](https://metebalci.com/blog/demystifying-arm-gnu-toolchain-specs-nano-and-nosys/)
* [GNU Tools for STM32 12.3.rel1](https://github.com/STMicroelectronics/gnu-tools-for-stm32)
	* [ARM NewLib](https://github.com/STMicroelectronics/gnu-tools-for-stm32/tree/12.3.rel1/src/newlib/newlib) 4.2.0
	  * [NEWS file](https://sourceware.org/git/?p=newlib-cygwin.git;a=blob;f=newlib/NEWS;h=ce4391367b5d6c6aed58cc5b7ad531420f2c9d51;hb=HEAD)

# Pinout

See [`pinout-with-alternates.csv`](pinout-with-alternates.csv).
This was generated in the IOC editor,
Pinout & Configuration →
Pinout →
Export Pintout with Alt. Functions.

## SPI 

GPIO Pins
* CS (Chip Select) - PA15
* DC (Data or Command) - PB8
* RESET - PB5

Power (for PJRC ILI9341 Display)
* VCC - 3.3V (works despite spec sheet saying 3.6-5.5V)
* GND - GND
* LED - 3.3V

SPI MISO/MOSI/CLK - wired directly to SPI2 header

### SPI2 DMA

* DMA 1 Stream 4
  * Memory auto-increment
  * Mode Normal
  * Priority Medium
* NVIC
  * DMA 1 Stream 4 Global Interrupt
  * Set the priority just behind the audio I2S
    and ahead of the UART interrupts

### SPI2 DMA Callbacks

Setting HAL DMA callbacks doesn't call anything back when using 
HAL SPI DMA calls.

So, enable SPI callbacks:
* Edit IOC file ->
* Project Manager ->
* Advanced Settings ->
* Register Callback ->
* SPI: Enable 

[See example](https://community.st.com/t5/stm32cubemx-mcus/stm32f2xx-quot-use-hal-xx-register-callbacks-quot-where-to-set/td-p/262882).

Now the transfer complete callbacks for SPI transfers in DMA
mode works.
    
### DMA Notes for SPI - Flash

DMA directly from Flash to SPI
*does not work*. The HAL DMA transfer
never completes and the status just says
`BUSY` the whole time. This was evidenced
in the image DMA directly from text (const image data)
stored in the flash being unable to be DMA'd
to the display.

I don't know if this is *supposed* to work but I'm
just doing it wrong (e.g., using the flash alias address
instead of the original flash address, or something).

This was worked around by copying the flash to memory
and sending the data that way.


# Functionality

The software is adapted from my project
[Nucleo UART](https://github.com/LispEngineer/nucleo-uart), which has
this functionality:

* Decode incoming MIDI
* Create a single triangle wave tone based on last note on
  received
* Output the tone to the Audio DAC used in this board
* Display on the PJRC ILI9341 SPI display

## Sizes

In `Debug` build, as of this commit
(see Blame for the commit),
* text: 52k
* data: 732
* bss: 12k

## Cache performance

Empirical results: (Debug mode)
* All this is using just DTCM (64KB)
* Everything disabled, using AXI interface
  * Main loops per millisecond: 73-86
* I-Cache enabled, using AXI interface
  * Main loops per millisecond: 189-203
* I-Cache enabled, using AXI interface,
  instruction prefetch enabled (supposedly only for TCM)
  * Main loops per millisecond: 195-210
  * Not sure why this would matter
* I-Cache enabled, using AXI interface,
  instruction prefetch enabled (supposedly only for TCM),
  ART accelerator enabled (supposedly only for TCM)
  * Main loops per millisecond: 194-211
* I-Cache and D-Cache enabled,
  but no special code for DMA management
  due to using only 64kB DTCM
  * Main loops per millisecond: 243-260
* Current configuration, with DTCM, non-cached DMA RAM, and regular RAM,
  using the MPU to set the cache policy per memory area
  * Main loops per millisecond: 229-244

Conclusion:
* As far as I can tell, the I-Cache enabled, D-Cache disabled
  are basically all the same once the I-Cache is enabled.
* So, enable the I-Cache when using the AXI Flash interface.
* The D-Cache enabled adds another 20-25% performance at seemingly
  no cost.

Note:
* ART accelerator is for accessing Flash through TCM
  * Prefetch is for using Flash through TCM
* I-cache is for accessing Flash through AXI
* I don't think there is any benefit for enabling both - I think only one is used at a time
* By default STM32Cube sets things up for AXI Flash
* "The L1-cache can be a performance booster when used in conjunction with memory interfaces on AXI bus. This must not be confused with memories on the Tightly Couple Memory (TCM) interface, which are not cacheable."
  * AN4839 Rev 2 page 4
* "If the software is using cacheable memory regions for the DMA source/or destination buffers. The software must trigger a cache clean before starting a DMA operation to ensure that all the data are committed to the subsystem memory. After the DMA transfer complete, when reading the data from the peripheral, the software must perform a cache invalidate before reading the DMA updated memory region."
  * AN4839 Rev 2 page 8


# TODO

* Make all DMA memory reads and writes volatile
  so the compiler doesn't optimize them and forces
  a read from RAM (or Cache)
* ALMOST DONE: Set up a memory region that the
  MPU has marked non-cacheable for DMA. Do all DMA
  from there. This may necessitate:
  * 1. BSS allocation
  * 2. data allocation (initialized variables)
  * TODO STILL 3. Special heap with dynamic memory allocator
    since I dynamically allocate SPI DMA transmit
    buffers (e.g., [this one](https://github.com/embeddedartistry/libmemory))
* Analyze the allocations of `malloc()` in my application;
  they are all currently in `spidma_ili9341.c` and the corresponding
  `free()` are in `spidma.c`   


## DONE

* DONE Make the LED code toggle the LED on my board
* DONE Make the button reading code read a button on my board
* DONE Set up the clocks
* DONE Set up the UARTS
  * Console: 460,800, Overrun enable, DMA on RX Error enable
  * MIDI: 31,250 Bits/s, 8-N-1, overrun enable, DMA on RX Error enable
* DONE Set up NVIC
  * USARTx global interrupt (3, 6) with preemption priority (2, 3)
  * 3 bits for pre-emption
* DONE Update the text/functionality
* DONE Set up the DMA
  * SPIx_TX -> DMA 1 Stream x (was 3, 5)
  * Circular to Memory
  * Half Word both ways
  * Global interrupt & preemption priority ("Force DMA channels interrupts")
* DONE Set up I2S
  * Master Transmit
  * I2S Philips
  * 16 bits data on 16 bits frame
  * 32 KHz
  * Clock Polarity Low


# DMA UART Circular Receive Test

## Goal

Turn UART receive into free-running
DMA circular buffer receive.
Periodically check what the last byte written
by the DMA was, and read the bytes since we
last read until DMA's last written.

Need to know: Can I get the current write pointer
from the DMA system?

## Online notes

[Reference](https://community.st.com/t5/stm32-mcus-embedded-software/how-to-transmit-usart-data-with-dma-and-low-level-ll-functions/m-p/310350/highlight/true#M21869)
Steps to be considered (in order):
*    Use configuration from CubeMX
*    Configure DMA memory & peripheral addresses for UART and your buffer
*    Configure DMA length (number of bytes to transmit in your case)
*    Enable UART DMA TX request (not done in your example)
*    Enable UART TX mode
*    Enable UART itself
*    Enable DMA channel
*    Transmission will start

* [Doc](https://github.com/MaJerle/stm32-usart-uart-dma-rx-tx) that might help.

# EVT1 Hardware Test Notes

## Serial console

* Works
* Only the Ground pin needs to be connected
  * The 3.3V pin does *not* need a connection
  
## USART6

Connected this to `ubld.it` MIDI breakout:
* RX -> Rx
* TX -> Tx
* Power/Ground

Configured as such, MIDI in works fine.

## MIDI

### MIDI In

`sendmidi dev "U2MIDI Pro" on 64 100 +00:00:00.200 off 64 100`

* MIDI 1 and 2 IN (UARTs 1 & 3) work
  * WHEN REWIRED externally - see problems below

### MIDI Out

`receivemidi dev MidiView`

* MIDI 1 & 2 OUT work
  * When rewired externally - see problems below

## LEDs and Buttons

* All work


# EVT1 Hardware Problems Noted

Problems noted exist on both assembled boards.
Note one board is missing J5 so easy to differentiate.

## Tests remaining

* I2C
* SPI
* GPIO
* The other serial ports
  * Test MIDI with a `ubld.it` MIDI breakout
  * USART6 RX has been tested
* RTC
* Master clock output
* Wakeup

## P0

* MIDI In doesn't seem to be working on either port
  * I miswired the Audio Jack
    * [See Helpful Page](https://www.sameskydevices.com/blog/understanding-audio-jack-switches-and-schematics)
    * I got the tip and sleeve reversed
    * Using a patch cable to swap them makes the MIDI 1 work fine
  * This was fixed in EVT2 of the hardware
    
* Enabling/disabling audio mute while no sound is playing
  makes an audible clicking sound on headphone out
  * This is also audible on the line out
  * It does not occur when I2S audio out is stopped

## P1

* Console is USART2
  * Nucleo boards use UART3 (oops!)
  * (But note the console UART works)

# Debugging Notes

## FIXED: Gain0 Toggle not toggling

* Pin was set to GPIO_Input instead of GPIO_Output


## MIDI Input Fails after a while

For some unknown reason, the MIDI input now fails after a while.

I tried it with MIDI1 & 2 (USART1 & 3), and my external MIDI
adapter on USART6. 

It does not show any MIDI overrun errors.

Currently a complete mystery why this is happening.

Seems like it's getting some error condition as these make it work again:
  LL_USART_ClearFlag_LBD(MIDI1_UART);
  LL_USART_ClearFlag_PE(MIDI1_UART);
  LL_USART_ClearFlag_NE(MIDI1_UART);
  LL_USART_ClearFlag_ORE(MIDI1_UART);
  LL_USART_ClearFlag_IDLE(MIDI1_UART);

Looks like it's getting an ORE.

The fix: Move to interrupt-driven receiving.

I have moved to interrupt-driven receiving using LL drivers.
This does not seem to have fixed the problem. I can still get
plenty of overrun errors on MIDI (which is slow, ~3kBps)!

Let me try making the interrupt priority higher.
That seems to have done the trick (setting interrupt
priority higher), but it interferes with refilling the
I2S audio buffer and we get crackling.
Doubling the I2S buffer from 128 to 256 samples seems to
have helped quite a bit.

## Break over Serial Console

Interestingly, if I send a BREAK using Putty over the serial
console at 460,800 bps, it resets the Microcontroller!
