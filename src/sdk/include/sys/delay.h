/* 
 * Copyright(c) 2004-2012 BORESTE (www.boreste.com). All Rights Reserved.
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
 * @file sys/delay.h
 * @brief YARD-ICE libutil
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#ifndef __SYS_DELAY_H__
#define __SYS_DELAY_H__

#ifdef __cplusplus
extern "C" {
#endif

void udelay_calibrate(unsigned int ticks1ms, 
					  unsigned int (* get_ticks)(void));

void udelay(unsigned int usec);

void mdelay(unsigned int msec);

void delay(unsigned int sec);


#ifdef __cplusplus
}
#endif

#endif /* __SYS_DELAY_H__ */

