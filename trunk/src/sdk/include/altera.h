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
 * @file altera.h
 * @brief YARD-ICE
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */

#ifndef __ALTERA_H__
#define __ALTERA_H__

#include <stdint.h>
#include <sys/delay.h>
#include <arch/at91x40.h>

struct altera_conf_io_map {
	/* DCLK */
	uint32_t dclk;
	/* nCONFIG */
	uint32_t config;
	/* DATA0 */
	uint32_t data;
	/* nSTATUS */
	uint32_t status;
	/* CONF_DONE */
	uint32_t conf_done;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
	Hardware setup
*/
int altera_io_init(const struct altera_conf_io_map * io);

int altera_configure(const struct altera_conf_io_map * io, const uint8_t * buf, int len);

int altera_nstatus(const struct altera_conf_io_map * io);

int altera_conf_done(const struct altera_conf_io_map * io);

#ifdef __cplusplus
}
#endif

#endif /* __ALTERA_H__ */

