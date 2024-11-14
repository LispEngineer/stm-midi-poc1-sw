# Doug's STM32 MIDI Synth Proof of Concept 1 - Software

* Author: [Douglas P. Fields, Jr.](mailto:symbolics@lisp.engineer)
* Copyright 2024, Douglas P. Fields Jr.
* License: [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0.txt)
* Last updated: 2024-11-10
* [Repo Self-Link](https://github.com/LispEngineer/stm-midi-poc1-sw)
* Portions used under other licenses
  * SPI Display - MIT License
  * STM Software / Generated Code - Delegated to LICENSE file, hence
    Apache 2.0

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

External hardware:
* USB Console: Inland FT232 TTL UART to USB board
  * [Similar to this one](https://www.ebay.com/itm/142909573493)
* SPI Display: [PJRC ILI9341](https://www.pjrc.com/store/display_ili9341_touch.html)



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



# Functionality

The software is adapted from my project
[Nucleo UART](https://github.com/LispEngineer/nucleo-uart), which has
this functionality:

* Decode incoming MIDI
* Create a single triangle wave tone based on last note on
  received
* Output the tone to the Audio DAC used in this board
* Display on the PJRC ILI9341 SPI display


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

## Debugging Notes
