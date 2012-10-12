/*
 *
 * File:	tcpip/udp.h
 * Module:	
 * Project:	
 * Author:	Robinson Mittmann (bob@boreste.com, bobmittmann@gmail.com)
 * Target:	
 * Comment:	TCPIP
 * Copyright(c) 2003-2009 BORESTE (www.boreste.com). All Rights Reserved.
 *
 */

#ifndef __SYS_UDP_H__
#define __SYS_UDP_H__

#ifndef __USE_SYS_UDP__
#error "Never use <sys/udp.h> directly; include <tcpip/udp.h> instead."
#endif 

#ifdef CONFIG_H
#include "config.h"
#endif

#ifdef UDP_DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif
#include <sys/dcclog.h>

#include <sys/os.h>

#include <stdint.h>

#include <netinet/in.h>
#include <netinet/udp.h>

#include <sys/mbuf.h>
#include <sys/net.h>
#include <sys/pcb.h>

#include <tcpip/ip.h>
#include <tcpip/udp.h>
#include <tcpip/arp.h>

#ifndef UDP_PCB_TAB_LEN
#define UDP_PCB_TAB_LEN 2
#endif

#ifndef ENABLE_UDP_PROTO_STAT
#define ENABLE_UDP_PROTO_STAT 0
#endif

struct udp_dgram {
	in_addr_t addr;
	uint16_t port;
	uint16_t len;
	struct mbuf * q;
}; /* 12 bytes */

#define UDP_RECV_MAX 4

/*! \brief UDP protocol control block. */
struct udp_pcb {
	/*! foreign address */
	in_addr_t u_faddr;
	/*! local address */
	in_addr_t u_laddr;
	union {
		struct {
			/*! foreign port */
			uint16_t u_fport;
			/*! local port */
			uint16_t u_lport;
		};
		uint32_t u_ports;
	};
	/* 12 */
	/*! udp flags  */
	uint8_t u_flags : 7;
	uint8_t u_icmp_err : 1;
	int8_t u_rcv_cond;
	volatile uint8_t u_rcv_tail;
	volatile uint8_t u_rcv_head;
	/* 16 */
	struct udp_dgram u_rcv_buf[UDP_RECV_MAX];
	/* 4 * 12 = 48 + 16 = 64*/
};

struct udp_system {
#ifdef ENABLE_UDP_CACHE
	struct udp_pcb * cache;
#endif
	/*! pcb list */
	struct pcb_list list;
#if ENABLE_UDP_PROTO_STAT
	struct proto_stat stat;
#endif
};

#if ENABLE_UDP_PROTO_STAT
#define UDP_PROTO_STAT_ADD(STAT, VAL) __udp__.stat.STAT += (VAL)
#else
#define UDP_PROTO_STAT_ADD(STAT, VAL)
#endif

extern struct udp_system __udp__;

//extern uthread_mutex_t udp_input_mutex;

#ifdef __cplusplus
extern "C" {
#endif

/* Internals */

int udp_input(struct ifnet * __if, struct iphdr * __ip, 
			   struct udphdr * __udp, int __len);

void udp_output(struct udp_pcb * __up);

/*
 * This function is called from the ICMP input to notify an
 * UDP connection when the remote host sends a ICMP_PORT_UNREACH
 * message.
 */
int udp_port_unreach(in_addr_t __faddr, uint16_t __fport, 
					 in_addr_t __laddr, uint16_t __lport);

int udp_release(struct udp_pcb * __up);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_UDP_H__ */
