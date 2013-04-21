/* 
 * Copyright(C) 2012 Robinson Mittmann. All Rights Reserved.
 * 
 * This file is part of the YARD-ICE.
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

/** 
 * @file at91sam7x256.c
 * @brief YARD-ICE
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#include "target/at91sam7.h"
#include "armice.h"

const struct target_arch at91sam7x256_arch = {
	.name = "AT91SAM7X256",
	.model = "generic",
	.vendor = "BORESTE",
	.cpu = &at91sam7_cpu,
	.sym = void_sym
};

const struct target_info at91sam7x256 = {
	.name = "at91sam7x256",
	.arch = &at91sam7x256_arch,
	.mem = (struct ice_mem_entry *)at91sam7s256_mem,

	.ice_drv = &armice_drv,
	.ice_cfg = (void *)&at91sam7_cfg,

	.jtag_clk_slow = 100000,
	.jtag_clk_def = 8000000,
	.jtag_clk_max = 16000000,

	/* The target has a TRST connection */
	.has_trst = NO,
	/* The target has a nRST connection */
	.has_nrst = YES,
	/* Target supports adaptive clock */
	.has_rtck = NO,
	/* The preferred clock method is adaptive (RTCK) */
	.prefer_rtck = NO,
	/* Start with slow clock */
	.clk_slow_on_connect = YES,
	/* Set default clock after init  */
	.clk_def_on_init = YES,
	/* auto probe scan path */
	.jtag_probe = YES,
	/* hardware reset before ICE configure */
	.reset_on_config = YES,

	/* The preferred reset method is software */
	.reset_mode = RST_SOFT,

	.start_addr = 0x00000000,

	.on_init = (target_script_t)at91sam7_on_init,
	.on_halt = (target_script_t)at91sam7_on_halt,
	.on_run = NULL,
	.reset_script = (target_script_t)at91sam7_reset,
	.probe = (target_script_t)at9sam7x256_probe,
	.test = (target_test_t)at9sam7_test
};

