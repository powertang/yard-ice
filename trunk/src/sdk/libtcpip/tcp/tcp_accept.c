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
 * @file tcp_accept.c
 * @brief
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#define __USE_SYS_TCP__
#include <sys/tcp.h>

struct tcp_pcb * tcp_accept(const struct tcp_pcb * __mux)
{
	struct tcp_listen_pcb * mux = (struct tcp_listen_pcb *)__mux;
	struct tcp_pcb * tp;

	tcpip_net_lock();
	
	while (mux->t_head == mux->t_tail) {

		DCC_LOG1(LOG_TRACE, "<%04x> waiting...", (int)mux);

		__os_cond_wait(__mux->t_cond, net_mutex);
	};

	tp = (struct tcp_pcb *)mux->t_backlog[mux->t_head++];

	/* wrap */
	if (mux->t_head == mux->t_max)
		mux->t_head = 0;

	if (tp == NULL) {
		DCC_LOG(LOG_PANIC, "NULL pointer");
	} else {
		DCC_LOG4(LOG_TRACE, "<%04x> --> <%04x> %I:%d", (int)mux, 
				 (int)tp, tp->t_faddr, ntohs(tp->t_fport));
	}

	tcpip_net_unlock();

	return tp;
}
