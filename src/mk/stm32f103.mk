#
# stm32f207.mk 
#
# Copyright(C) 2013 Robinson Mittmann. All Rights Reserved.
# 
# This file is part of the YARD-ICE.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3.0 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You can receive a copy of the GNU Lesser General Public License from 
# http://www.gnu.org/

MKDIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
BASEDIR := $(MKDIR)/..
TOOLSDIR = $(realpath $(BASEDIR)/../tools)
LDDIR = $(realpath $(BASEDIR)/ld)

export MKDIR LDDIR TOOLSDIR

ifndef MACH 
MACH = stm32f103
endif

ARCH = cm3
CPU = cortex-m3
#CDEFS += STM32F103 "STM32F_HCLKCM3_SYSTICK_CLK_HZ=8000000" 
CDEFS += STM32F103 "HCLK_HZ=72000000" 
OPTIONS	= -mcpu=$(CPU) -mthumb -mthumb-interwork 
CROSS_COMPILE = arm-none-eabi-

LDSCRIPT = $(MACH).ld  

include $(MKDIR)/prog.mk
