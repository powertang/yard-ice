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
 * @file udp.c
 * @brief
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#define __USE_SYS_UDP__
#include <sys/udp.h>

#include <string.h>
#include <stdlib.h>

#ifndef UDP_DEFAULT_TTL
#define UDP_DEFAULT_TTL 127
#endif

const uint8_t udp_def_ttl = UDP_DEFAULT_TTL;

#ifndef UDP_DEFAULT_TOS
#define UDP_DEFAULT_TOS 0x80
#endif

const uint8_t udp_def_tos = UDP_DEFAULT_TOS;

struct udp_system __udp__;

struct udp_pcb * udp_alloc(void)
{
	struct udp_pcb * up;

	/* get a new memory PCB */
	if ((up = (struct udp_pcb *)pcb_alloc()) == NULL) {
		DCC_LOG(LOG_WARNING, "could not allocate a PCB");
		return NULL;
	}

	/* ensure the mem is clean */
	memset(up, 0, sizeof(struct udp_pcb));

	tcpip_net_lock();

	pcb_insert((struct pcb *)up, &__udp__.list);

	up->u_rcv_cond = __os_cond_alloc();

	tcpip_net_unlock();

	return up;
}

int udp_port_unreach(in_addr_t __faddr, uint16_t __fport, 
					 in_addr_t __laddr, uint16_t __lport)
{
	struct udp_pcb * up;

	up = (struct udp_pcb *)pcb_wildlookup(__faddr, __fport, 
										  __laddr, __lport, &__udp__.list);
	if (up == NULL) {
		DCC_LOG4(LOG_TRACE, "not found: %I:%d > %I:%d", 
				 __laddr, ntohs(__lport), __faddr, ntohs(__lport));
		return -1;
	}

	/* set the error flag */
	up->u_icmp_err = 1;

	/* unconnect the PCB */
	up->u_faddr = INADDR_ANY;
	up->u_fport = 0;

	/* notify the application */
	__os_cond_signal(up->u_rcv_cond);

	return 0;
}

void udp_init(void)
{
	DCC_LOG(LOG_TRACE, "initializing UDP subsystem."); 

	pcb_list_init(&__udp__.list);
}

