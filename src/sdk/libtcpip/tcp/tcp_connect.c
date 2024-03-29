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
 * @file tcp_connect.c
 * @brief
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#define __USE_SYS_TCP__
#include <sys/tcp.h>

#include <tcpip/tcp.h>

int tcp_connect(struct tcp_pcb * __tp, in_addr_t __addr, uint16_t __port) 
{
	struct route * rt;
	in_addr_t laddr;
	int mss;

	if (__tp == NULL) {
		DCC_LOG(LOG_WARNING, "NULL pointer");
		/* FIXME: not a socket? The semantic here is not exactly the same
		   as the sockets API. */
		return -ENOTSOCK;
	}

	if (pcb_find((struct pcb *)__tp, &__tcp__.closed) < 0) {
		DCC_LOG1(LOG_ERROR, "<%04x> pcb_find()", (int)__tp);
		return -ENOTSOCK;
	}

	tcpip_net_lock();

	if ((rt = __route_lookup(__tp->t_faddr)) == NULL) {
		DCC_LOG(LOG_WARNING, "no route to host");			
		if (__tp->t_laddr == INADDR_ANY) {
			DCC_LOG(LOG_WARNING, "no local address");			
			tcpip_net_unlock();
			return -1;
		}
		mss = tcp_defmss;
		laddr = __tp->t_laddr; 
	} else {
		/* default mss to the network interface mtu. */
		mss = rt->rt_ifn->if_mtu - (sizeof(struct tcphdr) + 
									sizeof(struct iphdr));
		if (__tp->t_laddr == INADDR_ANY) {
			/* set the local address to the network interface
			   address */
			laddr = rt->rt_ifn->if_ipv4_addr;
		} else
			laddr = __tp->t_laddr; 
	}

	/* Check to see if this address is not in use already. */
	if (pcb_lookup(__addr, __port, laddr, __tp->t_lport, 
				   &__tcp__.listen) != NULL) {
		DCC_LOG3(LOG_WARNING, "<%04x> %I:%d in use", (int)__tp,
				 __addr, ntohs(__port));
		return -EADDRINUSE;
	}

	__tp->t_faddr = __addr;
	__tp->t_fport = __port;

	if (mss > tcp_maxmss)
		mss = tcp_maxmss;

	if ((__tp->t_cond = __os_cond_alloc()) < 0) {
		DCC_LOG(LOG_WARNING, "__os_cond_alloc()");
		tcpip_net_unlock();
		return -1;
	}

	/* Set up the new PCB. */
/*	tp->t_lport = th->th_dport;
	tp->t_laddr = iph->daddr;
	tp->t_fport = th->th_sport; 
	tp->t_faddr = iph->saddr; */
	__tp->t_laddr = laddr; 
	__tp->t_maxseg = mss;

	/*
	 * TODO: max queue size...
	 */
	mbuf_queue_init(&__tp->rcv_q);
	mbuf_queue_init(&__tp->snd_q);

	__tp->rcv_nxt = 0;

	/* set the initial sequence number */
	__tp->snd_seq = (++__tcp__.iss << 20);
	DCC_LOG2(LOG_TRACE, "<%05x> snd_seq = (%d)", (int)__tp, __tp->snd_seq);

	__tp->snd_off = 0;
	__tp->snd_max = 0;

	/* set the send window to 1 (ACK) */
	__tp->snd_wnd = 1;

	/* set the connection-establishment timer to 75 seconds  */
	__tp->t_conn_tmr = tcp_conn_est_tmo;

	/* Move from the closed pcb list to the active list */
	pcb_move((struct pcb *)__tp, &__tcp__.closed, &__tcp__.active);

	__tp->t_state = TCPS_SYN_SENT;

	tcp_output_sched(__tp);

	DCC_LOG1(LOG_TRACE, "<%05x> [SYN_SENT]", (int)__tp);

	for (;;) {
		if ((__tp->t_state == TCPS_CLOSED)) {
			DCC_LOG1(LOG_WARNING, "<%05x> refused", (int)__tp);
			/* release the conditional variable */
			__os_cond_free(__tp->t_cond);
			/* XXX: discard the data */
			mbuf_queue_free(&__tp->snd_q);
			mbuf_queue_free(&__tp->rcv_q);

			tcpip_net_unlock();
			return -1;
		}

		if ((__tp->t_state == TCPS_TIME_WAIT) ||
			(__tp->t_state == TCPS_CLOSING) || 
			(__tp->t_state == TCPS_LAST_ACK) ||
			(__tp->t_state == TCPS_CLOSE_WAIT)) {
			DCC_LOG1(LOG_ERROR, "<%05x> unexpected state", (int)__tp);
			tcpip_net_unlock();
			return -1;
		}

		if (__tp->t_state == TCPS_ESTABLISHED) {
			DCC_LOG1(LOG_INFO, "<%05x> connected", (int)__tp);
			break;
		}

		__os_cond_wait(__tp->t_cond, net_mutex); 
	}

	tcpip_net_unlock();

	return 0;
}

