# Introduction #

The hardware for the **YARD-ICE** is based on a ST STM32F207 SoC and an Altera Cyclone FPGA.

# Details #

## Goals ##

The hardware was designed with the following features in mind:
  * Simple enough to be hand soldered: there is no BGA or QFN packages on the board. All chips are at least 0603 sized. All transistors and regulators are SOT-23 type of package. There is no aluminum electrolytic capacitors.
  * Inexpensive to manufacture, except for 2 connectors all the remaining parts are SMT. Single side assembly.
  * Inexpensive PCB fabrication: Dual layer PCB. This as/is would not pass an EMC test. But the addition of two internal ground layers with the correct stack-up will, most likely, solve any EMC issues.
  * It provides a regulated 5V 1A output to supply your circuit under debug.
  * Have a dry contact (relay) port to control an external device. This is useful for automated or off site remote debugging sessions.
  * Have an embedded speaker for audible event notification. This is useful for get the operator's attention on a semi-automated testing/programming set up.
  * Ethernet connection
  * Wide range input voltage
  * Programmable JTAG I/O voltage
  * NON volatile memory to store session information. This is a debug tool for the debugger itself.
  * No need for an external tool to program the board. A short circuit in a jumper allows it to boot from the internal ROM and download the firmware trough the USB port.

## Missing Features ##

  * An large external memory or SD card slot for autonomous operation. The problem is that the board is already congested and there is no space in the enclosure for extra interfaces. The solution may be use the P2,P3 and P4 connectors to support a risen daughter card.

## Block Diagram ##

The top-level block diagram for the platform is depicted bellow:

![http://yard-ice.googlecode.com/files/yard-ice-top-level-block-diagram.png](http://yard-ice.googlecode.com/files/yard-ice-top-level-block-diagram.png)

FIXME: this diagram is incomplete. The original SVG can be found at the source code tree under the doc/svg folder.

## Release 0.1 ##

A set of prototype boards where produce with the help of **I-LAX Electronics Inc.** (www.i-lax.ca) in Canada.
There is an error in the USB connector which was fixed in the 0.2 release.

![http://yard-ice.googlecode.com/files/yard_ice_01_top.jpg](http://yard-ice.googlecode.com/files/yard_ice_01_top.jpg)

![http://yard-ice.googlecode.com/files/yard_ice_01_bottom.jpg](http://yard-ice.googlecode.com/files/yard_ice_01_bottom.jpg)

## Release 0.2 ##

The fabrication files for the release 0.2 can be downloaded from here:

http://yard-ice.googlecode.com/files/yard-ice-pcb-rev0.2.zip

There is no plans for fabrication at this point as I'm trying to figure out how to finance this. Any suggestions are welcome.