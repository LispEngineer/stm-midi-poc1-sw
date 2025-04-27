# Doug's STM32 MIDI Synth Proof of Concept 1 - Software

* Author: [Douglas P. Fields, Jr.](mailto:symbolics@lisp.engineer)
* Copyright 2024, Douglas P. Fields Jr.
* License: [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0.txt)
* Last updated: 2025-03-16
* [Repo Self-Link](https://github.com/LispEngineer/stm-midi-poc1-sw)
* Portions used under other licenses
  * SPI Display - MIT License
  * STM Software / Generated Code - Delegated to LICENSE file, hence
    Apache 2.0
  * Memory allocator - Public Domain (memmgr.c/h)
  * [FreeRTOS](https://freertos.org/Documentation/02-Kernel/01-About-the-FreeRTOS-kernel/04-Licensing) - 
    MIT License
    
Software for my 
[STM32 MIDI Synthesizer PoC 1](https://github.com/LispEngineer/stm-midi-poc1).


# Credits

* [SPI Display Software](https://github.com/afiskon/stm32-ili9341) 
  by Aleksander Alekseev under [MIT License](https://opensource.org/license/mit)
  * As enhanced by [my project](https://github.com/LispEngineer/stm-f7-spi-display)
* [FreeRTOS](https://freertos.org/) under MIT License currently maintained by
  Amazon / AWS
* [Memory Allocator](https://github.com/eliben/code-for-blog/tree/main/2008/memmgr)
  by [Eli Bendersky](mailto:eliben@gmail.com) under the Public Domain


# Software

* Windows 11
* STM32CubeIDE
* STM32 HAL and LL libraries
* FreeRTOS
* Segger
  * Ozone
  * SystemView
* git

# Hardware

* STM32F722RET6
  * [STM32F7x2 Overview](https://www.st.com/en/microcontrollers-microprocessors/stm32f7x2.html)
  * [STM32F722RE Page](https://www.st.com/en/microcontrollers-microprocessors/stm32f722re.html)
  * [STM32F722RET6 Part](https://estore.st.com/en/stm32f722ret6-cpn.html)
  * [STM32F722 Data Sheet](https://www.st.com/resource/en/datasheet/stm32f722ic.pdf)
    * DS11853 Rev 9 July 2022 as of this writing
  * Package: LQFP-64
  * Core: ARM Cortex-M7 (revision r1p1)
* Audio DAC with headphone amplifier
  * DAC: [PCM5102A](https://www.ti.com/product/PCM5102A)
    * (Future: Check out the [TAD5242](https://www.ti.com/product/TAD5242)
  * Amplifier: [PAM8908JER](https://www.diodes.com/part/view/PAM8908/)
* MIDI in/out
  * Opto-isolator: 
    * EVT1: 6N137S-TA1
    * EVT2: [TLP2362](https://toshiba.semicon-storage.com/us/semiconductor/product/isolators-solid-state-relays/detail.TLP2362.html)
      * much smaller chip
* Power: USB-C
* Programming: SWD (with 5-pin header, no SWO)
  * ST-Link v3: SWD & UART (USART2 console)
    * [STLINK-V3SET](https://www.st.com/en/development-tools/stlink-v3set.html)
  * Segger J-Link (Plus v10, EDU Mini)
    * EDU model does not have VCOM (console)
    * VCOM seems to work only at 115,200 bps
      * TODO: Figure out why

Key STM32F722 Manuals:
* Data sheet - DS11853 Rev 9
* Errata - ES0360 Rev 5
* Reference manual - RM0431 Rev 3
* Programming manual - PM0253 Rev 5
* System architecture & performance - AN4667 DocID027643 Rev 4
* L1 Cache - AN4839 Rev 2
* STM32F7 HAL and LL - UM1905 Rev 5
* STM32CubeMX - UM1718
* r1p1 "[suffers from erratum 1259864](https://forums.freertos.org/t/arm-cortex-m7-fault-exception-and-stack-corruption/17700/10)"
  * "you can’t safely use write-through caching"
* [Cortex-M7 Manual Revisions](https://developer.arm.com/documentation/ddi0489/f/revisions)
* [STM32F722 Errata](https://www.st.com/resource/en/errata_sheet/es0360-stm32f72xxx-and-stm32f73xxx-device-limitations-stmicroelectronics.pdf)
  * See section 2.1.1 for Erratum 1259864
* [Arm Errata for Cortex-M7](https://documentation-service.arm.com/static/665dff778ad83c4754308908?token=)
  * Category A = No Workaround
  * Fixed in version `r1p2`


External hardware:
* USB Console: Inland FT232 TTL UART to USB board (when not using ST-Link or J-Link VCOM)
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
* Synth:
  * Triangle-wave only
  * Velocity sensitive
  * N note polyphony (set by `#define SYNTH_POLYPHONY`)
  * Mixing is by clipping
  * 32,000 Hz, 16-bit resolution
* Output the mixed synth tone to the Audio DAC used in this board
* Display on the PJRC ILI9341 SPI display
* Display stuff on the serial UART at 460,800 8-N-1
* Send MIDI
* Mute and change the gain of the headphone amplifier

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
* Figure out how to tell if we're sending left or right channel
  on I2S and when we're sending that channel.
  * Then prove it by sending different sounds to the left and the
    right
* Move to an RTOS
  * FreeRTOS is built into STM32 tooling
    * I also got it running stand-alone on the STM32F722 chip!
  * Azure RTOS (formerly ThreadX) 
    * [Tutorial](https://wiki.st.com/stm32mcu/wiki/Introduction_to_Azure_RTOS_with_STM32)
    * [STM32Cube integration](https://www.st.com/content/st_com/en/campaigns/x-cube-azrtos-azure-rtos-stm32.html)
    * [STM32F7 integration](https://github.com/STMicroelectronics/x-cube-azrtos-f7)
  * Zephyr
* Use Segger RTT instead of console UART?
  * `C:\Program Files\SEGGER\JLink_V826\Samples\RTT` has the code
    `SEGGER_RTT_V826.zip`

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
* GPIO
* The other serial ports
  * Test MIDI with a `ubld.it` MIDI breakout
  * USART6 RX has been tested
* RTC
* Master clock output (removed in EVT2)
* Wakeup
* USB

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
```
  LL_USART_ClearFlag_LBD(MIDI1_UART);
  LL_USART_ClearFlag_PE(MIDI1_UART);
  LL_USART_ClearFlag_NE(MIDI1_UART);
  LL_USART_ClearFlag_ORE(MIDI1_UART);
  LL_USART_ClearFlag_IDLE(MIDI1_UART);
```  

Looks like it's getting an ORE (overrun exception).

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

# EVT2 Hardware Problems Noted

None serious so far!

Things to check:
* Line out

Things to test:
* I2C
* GPIO
* RTC
* Wakeup
* USB
* The other serial ports (than MIDI 1/2 and console)

Some thoughts:
* Muting & unmuting makes an audible click.
  * Maybe change the DAC mute and the headphone amplifier mute to
    be on different signals and see which one is the problem?
* MIDI in/out are working properly


# FreeRTOS

I am transplanting in FreeRTOS and then will migrate all the
current functionality to work nicely in a FreeRTOS manner.

* [Template Project](https://github.com/LispEngineer/midi-freertos-jlink)

Note that I use the Segger J-Link because of the availability of the
Ozone debugger and SystemView RTOS tool, free for non-commercial use. 
I have a new EDU Mini and a second-hand Plus v10.

Steps:

1. Copy files:
  * The `ThirdParty` directory from the Template
  * The `FreeRTOSConfig.h` include file in `Core/Inc`
2. `ThirdParty` Properties -> C/C++ Build -> Exclude from Build -> *unchecked*
3. Set up Include paths
  * In the IDE, Project -> Properties
  * C/C++ Build -> Settings -> Tool Settings -> MCU/MPU GCC Compiler -> Include Paths
  * Add these: `Workspace` -> `ThirdParty/`
    * `FreeRTOS-Kernel/include`
    * `FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1`
    * `SystemView`
    * `SystemView/Config`
    
Check: The project should build cleanly now. (Not much of a check.)

4. Update the `.ioc` file in several sections
  * Remove handlers that are provided by `port.c`
    * They are: `SVC_Handler`, `PendSV_Handler`, and `SysTick_Handler`
    * Open System Core -> NVIC
    * Open Code Generation under Configuration
    * In the `Generate IRQ handler` column, uncheck these rows:
      * Pendable request for system service
      * Time base: System tick timer
      * System service call via SWI instruction
  * Update the STM32 HAL timebase
    * System Core -> SYS
    * Ensure the "Mode" pane can be seen; maybe pull down the "Configuration" pane
    * Change `Timebase Source` from `SysTick` to `TIM6`
  * Update NVIC priority/sub-priority sizes from "3 bits for pre-emption priority,
    1 bits for subpriority"
    * (Not yet sure why we do this, it is how the example FreeRTOS was set up)
    * System Core -> NVIC
    * Configuration pane, NVIC tab
    * Priority Group = "4 bits for pre-emption priority 0 bits for subpriority"
      * (This was already set)
    * Update priorities: (all sub-priorities become 0)
      * These DMA interrupts should go to 2:
        * DMA1, streams 1,3,5,6
        * DMA2, streams 1,2
      * 3: 
        * DMA1 stream4
        * SPI2 global interrupt (maybe move this to 4)
      * 4: UART4 global interrupt (not used)
      * 5: UART5 global interrupt (not used)
      * 15:
        * System tick timer
        * Time base: TIM6 global interrupt, DAC1 and DAC2 underrun error interrupts
      * TODO: Revisit all these priorities later
  * Save the `.ioc` and regenerate the code

Check: The project still builds, and it should work the same way as always.
(Increment the version number to check the new code is in use.)

## FreeRTOS Status

1. The simple blinky works
2. UART I/O using a dumb conversion of my old SuperLoop code
   to FreeRTOS works

## FreeRTOS TO DO

### UART rewrite

1. Make received data only get read when an interrupt
   shows it has been read. However, still use DMA to
   copy that so if we don't read it quickly it will be
   able to see all data.
   
2. Protect all the serial writes so that if we are writing
   from multiple tasks, only one can be writing at a time.
   Or, alternatively, the tasks can (accidentally/intentionally)
   interleave their writes but the writer won't get messed up.
   
3. Make written data only poll when the DMA transmit is empty.
   So, basically, pause a task until the DMA transmit is done.
   Then pause it until there is something to write.
   
See [FreeRTOS example](https://github.com/FreeRTOS/iot-reference-stm32u5/blob/main/Common/cli/cli_uart_drv.c#L150).
   

# USB

Taking a side track to USB, as I will want to support USB
MIDI in the future. For now, let's explore USB Device mode.

TODO: First note: I have a physical pull-up resistor on
the USB D+ line (1.5k). Apparently, this is not needed;
[see Gemini's thoughts](https://g.co/gemini/share/8994b606c020).
Importantly, see [AN4879](https://www.st.com/resource/en/application_note/an4879-introduction-to-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.pdf)
which says that all models of the STM32F7 have an enbedded
pull-up resistor. (Rev 10, Jan 2025 current as of this writing.)
EVT#3 will need to have this removed.

The first implementation will be a USB Keyboard that just
sends a letter when User1 or User2 buttons are pressed.

## Configuration in IOC file

`USB_OTG_FS` under `Connectivity`:
* Mode: `Device_Only`
* Activate_VBUS: `Disable`
* Activate_SOF: not checked

Middleware, `USB_DEVICE`:
* Class for FS IP: `Human Interface Device Class (HID)`
* Device Descriptor -> Device Descriptor FS -> PRODUCT_STRING:
  DPF MIDI Synth HID
* Use default VID & PID (1155, 22315)

After changing the `.ioc` file, remember to generate code.
Clean and build the code.

