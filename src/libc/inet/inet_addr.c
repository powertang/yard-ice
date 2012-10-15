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
 * @file inet_addr.c
 * @brief YARD-ICE libc
 * @author Carlos Augusto Vieira e Vieira <carlos.vieira@boreste.com>
 */ 


#include <arpa/inet.h>

in_addr_t inet_addr(const char * cp)
{
	struct in_addr dummy;
	if (inet_aton(cp, &dummy) == 0)
		return INADDR_NONE;
	return dummy.s_addr;
}

