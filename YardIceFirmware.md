# Introduction #

The software of the **YARD-ICE** is separated into 4 major groups:
  * **SDK Libraries** - Software Development Kit Libraries
  * **YARD-ICE Libraries** - Libraries specific to the application
  * **YARD-ICE Application**
  * **YARD-ICE Targets** - Database of processors, SoC and platforms

One important thing to notice is that the YARD-ICE firmware do not need any external or third-party library. All what it is need to compile the firmware is provided with the source-code.

## SDK Libraries ##

These are a set of supporting libraries located inside the **src/sdk** directory. Each library can be used independently or compiled as a part of the YARD-ICE firmware.

  * **libaltera** - provides functions for loading program (rbf) files into the attached FPGA. It uses an SPI interface.
  * **libbitvec** - bit vector manipulation functions.
  * **libc** - the YARD libc is a small library targeted to embedded systems.
  * **libcm3** - Cortex-M3 specific functions
  * **libcrc** - Cyclic Redundancy Check functions.
  * **libdrv** - drivers for devices like Ethernet, Serial
  * **libhexdump** - provides functions to display binary data as hexadecimal plus ASCII values, like the hexdump command.
  * **libice-comm** - library used for real-time trace. This tool will send the trace to a remote host if a tool like the YARD-ICE is connected to the JTAG port.
  * **libopcodes** - this library provides disassembler capabilities to the tool.
  * **libstm32f** - hardware access functions for the STM32F family of SoCs.
  * **libtcpip** -  efficient and small footprint TCP/IP stack, designed for embedded systems.
  * **[libthinkos](libThinkOs.md)** - the ThinkOS real time operating system. This small RTOS was designed and optimized for ARM Cortex-M cores. It provides a very low context switching latency and a useful "_interrupt as thread_" mechanism.
  * **libutil** - set of utilities like a calibrated delay.

## YARD-ICE Libraries ##

...

## YARD-ICE Targets ##

...

## YARD-ICE Application ##

# Compiling #

The source code is located in the **src** directory. To compile make sure you have the **GCC-ARM toolchain** (arm-none-eabi-**) installed into your host computer.**

Move inside the **src** directory and type **make**.

```
yard-ice/src$ make
Creating: /home/bob/devel/yard-ice/src/release/version.h
- LIBS START ------------------
make[1]: Entering directory `/home/bob/devel/yard-ice/src/sdk/libcm3'

...

LD: /home/bob/devel/yard-ice/src/release/yard-ice.elf
BIN: /home/bob/devel/yard-ice/src/release/yard-ice.bin
SYM: /home/bob/devel/yard-ice/src/release/yard-ice.sym
LST: /home/bob/devel/yard-ice/src/release/yard-ice.lst
```

The loadable firmware will be located into the **release** directory and is called: **yard-ice.bin**.

More detailed instructions can be fund at: [Yard-ICE Compile How-To](YardIceCompileHowTo.md).

## Firmware Installation ##

If you have a Windows box you can follow this instructions: [STM32F Firmware Upgrade on Windows](YardIceWindowsUsbFirmwareUpdate.md).