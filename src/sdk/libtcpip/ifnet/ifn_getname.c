/* 
 * Copyright(c) 2004-2012 BORESTE (www.boreste.com). All Rights Reserved.
 *
 * This file is part of the libtcpip.
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
 * @file ifn_getname.c
 * @brief Get Network Interface Name
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#define __USE_SYS_IFNET__
#include <sys/ifnet.h>

#include <stdio.h>

#ifdef IFNET_DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

const char ifn_type_name[][4] = {
	"xxx",
	"eth",
	"lo",
	"sl",
	"ppp",
	"atm"
	""
};

int ifn_getname(struct ifnet * __if, char * __s)
{
	return sprintf(__s, "%s%d", ifn_type_name[__if->if_id >> 4], 
				   __if->if_id & 0x0f);
}

