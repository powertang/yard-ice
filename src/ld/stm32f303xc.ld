/* 
 * stm32f303xc.ld
 *
 * Copyright(C) 2013 Robinson Mittmann. All Rights Reserved.
 * 
 * This file is part of the ThinkOS library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You can receive a copy of the GNU Lesser General Public License from 
 * http://www.gnu.org/
 */

/* */

OUTPUT_FORMAT ("elf32-littlearm")
ENTRY(_reset)
SEARCH_DIR(.)

MEMORY
{
	vectors(r) : ORIGIN = 0x00000000, LENGTH = 1K
	flash (rx) : ORIGIN = 0x08000000, LENGTH = 256K 
	sram0 (rw) : ORIGIN = 0x20000000, LENGTH = 32K
	sram1 (rw) : ORIGIN = 0x10000000, LENGTH = 8K
	dcclog (r) : ORIGIN = 0x40000000, LENGTH = 1024K
}

INCLUDE stm32f3xx.ld

