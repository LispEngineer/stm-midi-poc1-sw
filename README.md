# Doug's STM32 MIDI Synth Proof of Concept 1 - Software

* Author: [Douglas P. Fields, Jr.](mailto:symbolics@lisp.engineer)
* Copyright 2024, Douglas P. Fields Jr.
* License: [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0.txt)
* [Repo Self-Link](https://github.com/LispEngineer/stm-midi-poc1-sw)

Software for my 
[STM32 MIDI Synthesizer PoC 1](https://github.com/LispEngineer/stm-midi-poc1).

Software:
* Windows 11
* STM32CubeIDE
* STM32 HAL libraries

Hardware:
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
  * Opto-isolator: [6N137S-TA1](TODO)
* Power: USB-C
* Programming: SWD (with 5-pin header)

Pinout:
* See [`pinout-with-alternates.csv`](pinout-with-alternates.csv)


# Software

The software is adapted from my project
[Nucleo UART](https://github.com/LispEngineer/nucleo-uart), which has
this functionality:

* Decode incoming MIDI
* Create a single triangle wave tone based on last note on
  received
* Output the tone to the Audio DAC used in this board


# TODO

* TODO Transfer the memory map over for all the 
  different memory areas
  * And the BSS/Data section initializers

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


# Hardware Notes

## Serial console

* Only the Ground pin needs to be connected
  * The 3.3V pin does *not* need a connection
  
## USART6

Connected this to `ubld.it` MIDI breakout:
* RX -> Rx
* TX -> Tx
* Power/Ground

MIDI in works fine.


# EVT1 Hardware Problems Noted

Problems noted exist on both assembled boards.
Note one board is missing J5 so easy to differentiate.

## Tests remaining

* MIDI Out
* I2C
* SPI
* GPIO
* The other serial ports
  * Test MIDI with a `ubld.it` MIDI breakout
* RTC
* Master clock output
* Wakeup

## P0

* MIDI In doesn't seem to be working on either port
  * TODO: Try both TRS-A or TRS-B
  * Did I make an error in Rx & Tx?
* Enabling/disabling audio mute while no sound is playing
  makes an audible clicking sound on headphone out
  * This is also audible on the line out
  * It does not occur when I2S audio out is stopped

## P1

* Console is USART2
  * Nucleo boards use UART3 (oops!)
  * (But note the console UART works)

## Fixed

* FIXED: Enabling/disabling I2S turns on/off the Green LED (!)
  using the q/w keys
  * I2S sound works (if you use the audio mute signal to enable output)
  * Turns out the code was flashing this LED when filling I2S buffer
