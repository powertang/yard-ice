# Introduction #

The source code is located in the **src** directory. All you need to compile is a working GCC-ARM Toolchain.

## GCC-ARM Toolchain ##

To compile make sure you have the **GCC-ARM toolchain** installed into your host computer. The toolchain comprises of a set of "tools" to compile your firmware including:
  * arm-none-eabi-gcc - Compiler
  * arm-none-eabi-ld - Linker
  * arm-none-eabi-as - Assembler
  * arm-none-eabi-ar - Archiver
  * arm-none-eabi-objcopy - Binary files copy tool

You can compile it by yourself:

> [ARM-GCC Tolchain How-To](http://bobmittmann.blogspot.ca/2012/02/ubuntu-1004-lts-and-arm-gnu-toolchain.html)

or download a pre-compiled package for your system.

(Note: someone could write a how-to get pre-compiled packages).

Make sure you have the **arm-none-eabi-gcc** in your PATH:
```
$ arm-none-eabi-gcc
arm-none-eabi-gcc: fatal error: no input files
compilation terminated.
```

# The Source Code #

You can get the latest revision source code from the repository:
```
svn checkout http://yard-ice.googlecode.com/svn/trunk yard-ice
```

# Compiling #

Move inside the **src** directory and type **make**.

```
$ cd yard-ice/src
yard-ice/src$ make
Creating: /home/bob/devel/yard-ice/src/release/version.h
- LIBS START ------------------
make[1]: Entering directory `/home/bob/devel/yard-ice/src/sdk/libcm3'
CC 1: /home/bob/devel/yard-ice/src/release/libcm3/cm3-udelay.o
AR: /home/bob/devel/yard-ice/src/release/libcm3/libcm3.a

...

AR: /home/bob/devel/yard-ice/src/release/libopcodes/libopcodes.a
arm-none-eabi-ar: creating /home/bob/devel/yard-ice/src/release/libopcodes/libopcodes.a
LST: /home/bob/devel/yard-ice/src/release/libopcodes/libopcodes.lst
make[1]: Leaving directory `/home/bob/devel/yard-ice/src/sdk/libopcodes'
- LIBS END --------------------
CC 1: /home/bob/devel/yard-ice/src/release/yard-ice.o
LD: /home/bob/devel/yard-ice/src/release/yard-ice.elf
BIN: /home/bob/devel/yard-ice/src/release/yard-ice.bin
SYM: /home/bob/devel/yard-ice/src/release/yard-ice.sym
LST: /home/bob/devel/yard-ice/src/release/yard-ice.lst
yard-ice/src$
```

The files will be compiled, by default, into a directory called **release**:
```
yard-ice/src$ ls release
libaltera  libcm3  libhexdump   libstm32f   libutil       yard-ice.elf  yard-ice.o
libbitvec  libcrc  libice-comm  libtcpip    version.h     yard-ice.lst  yard-ice.sym
libc       libdrv  libopcodes   libthinkos  yard-ice.bin  yard-ice.map
```

The loadable firmware is called: **yard-ice.bin**.

## Make Targets and Options ##

A list of available make targets and options can be obtained by typing **make help** at the **src** directory.

```
yard-ice/src$ make help
Targets:

  all        - Build YARD-ICE
  clean      - Remove most generated files
  jtagload   - Build YARD-ICE and try to load it into target

  make V=0|1 [targets] 0 => quiet build (default), 1 => verbose build
  make O=dir [targets] Locate all output files in "dir"
  make D=0|1 [targets] 0 => release (default), 1 => debug
```