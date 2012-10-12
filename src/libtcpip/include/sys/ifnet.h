/* $Id: ifnet.h,v 2.7 2008/06/04 00:03:14 bob Exp $ 
 *
 * File:	sys/ifnet.h
 * Module:	
 * Project:	
 * Author:	Robinson Mittmann (bob@boreste.com, bobmittmann@gmail.com)
 * Target:	
 * Comment:	TCPIP
 * Copyright(c) 2009 BORESTE (www.boreste.com). All Rights Reserved.
 *
 */

#ifndef __SYS_IFNET_H__
#define __SYS_IFNET_H__

#ifndef __USE_SYS_IFNET__
#error "Never use <sys/ifnet.h> directly; include <tcpip/ifnet.h> instead."
#endif 

#ifdef CONFIG_H
#include "config.h"
#endif

#ifdef IFNET_DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif
#include <sys/dcclog.h>

#define __need_size_t
#include <stddef.h>

#define __USE_SYS_NET__
#include <sys/net.h>

#include <stdint.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#include <tcpip/route.h>

#ifndef ENABLE_NETIF_STAT
#define ENABLE_NETIF_STAT 0
#endif

struct ifnet;

struct ifnet_operations {
	/* interface type */
	uint8_t op_type;

	/* mac address length */
	uint8_t op_addrlen;

	/* initialize de interface */
	int (* op_init)(struct ifnet * __if);

	/* remove form ifnterface list */
	int (* op_cleanup)(struct ifnet * __if);

	/* request a memory region */
	void * (* op_mmap)(struct ifnet * __if, size_t __length);

	/* send a frame */
	int (* op_send)(struct ifnet * __if, const uint8_t * __dst,
		int __proto, const void * __buf, int __len);

	/* translate ipv4 address into hardware address */
	void * (* op_arplookup)(struct ifnet * __if, in_addr_t __ipaddr);	

	/* get the interface mac address if any */
	int (* op_getaddr)(struct ifnet * __if, uint8_t * __buf);	

	/* put the interface in a sleep state */
	int (* op_sleep)(struct ifnet * __if);

	/* wakeup the interface from sleep state */
	int (* op_wakeup)(struct ifnet * __if);

	/* return a string describing the interface. */
	int (* op_getinfo)(struct ifnet * __if, char * __s, int __len);

	/* request a memory region */
	int (* op_munmap)(struct ifnet * __if, void * __mem);
};

struct ifnet {
	/* low level io address */
	void * if_io;

	/* irq number */
	uint8_t if_irq_no;

	/* interface id */
	uint8_t if_id;

	/* flags (IFF_*) */
	uint16_t if_flags;

	/* maximum transmission unit */
	uint16_t if_mtu;

	/* interface link speed (bps) */
	uint32_t if_link_speed;

	/* IPV4 address */
	in_addr_t if_ipv4_addr;

	/* IPV4 network mask */
	in_addr_t if_ipv4_mask;

	/* low level device driver private structure */
	void * if_drv;

	/* interface operations */
	const struct ifnet_operations * if_op;
#if (ENABLE_NETIF_STAT)
	struct ifnet_stat stat;
#endif
};


#if ENABLE_NETIF_STAT
#define NETIF_STAT_ADD(NETIF, STAT, VAL) (NETIF)->stat.STAT += (VAL)
#else
#define NETIF_STAT_ADD(NETIF, STAT, VAL)
#endif


/* Network Interface Flags */
/* interface is up */
#define	IFF_UP              0x0001		
/* broadcast address valid */
#define IFF_BROADCAST       0x0002
/* is a loopback net */
#define IFF_LOOPBACK        0x0004
/* interface is has p-p link */
#define IFF_POINTTOPOINT    0x0008
/* no ARP protocol */
#define IFF_NOARP           0x0010
/* receive all packets */
/* supports multicast */
#define IFF_MULTICAST       0x0020
/* link is active */
#define IFF_LINK_UP         0x0080

/* Network Interface Type */
#define IFT_MASK            0xf0
#define IFT_OTHER           0x00
#define IFT_ETHER           0x10
#define IFT_LOOP            0x20
#define IFT_SLIP            0x30
#define IFT_PPP             0x40

extern inline int ifn_init(struct ifnet * __if) {
	return __if->if_op->op_init(__if);
}

extern inline int ifn_cleanup(struct ifnet * __if) {
	return __if->if_op->op_cleanup(__if);
}

extern inline void * ifn_mmap(struct ifnet * __if, int __length) {
	return __if->if_op->op_mmap(__if, __length);
}

extern inline int ifn_munmap(struct ifnet * __if, void * __mem) {
	return __if->if_op->op_munmap(__if, __mem);
}

extern inline int ifn_send(struct ifnet * __if, const uint8_t * __dst, 
							  int __proto, const void * __buf, int __len) {
	return __if->if_op->op_send(__if, __dst, __proto, __buf, __len);
}

extern inline int  ifn_sleep(struct ifnet * __if) {
	return __if->if_op->op_sleep(__if);
}

extern inline int ifn_wakeup(struct ifnet * __if) {
	return __if->if_op->op_wakeup(__if);
}

extern inline int ifn_getaddr(struct ifnet * __if, uint8_t * __buf) {
	return __if->if_op->op_getaddr(__if, __buf);
}

extern inline int ifn_getinfo(struct ifnet * __if, char * __s, int __len) {
	return __if->if_op->op_getinfo(__if, __s, __len);
}

extern inline int in_broadcast(in_addr_t __addr, struct ifnet * __if)
{
	return (__addr | __if->if_ipv4_mask) == INADDR_BROADCAST;
}

extern  const uint8_t ifnet_max;

extern  struct ifnet __ifnet__[];

extern const char ifn_type_name[][4];

#ifdef __cplusplus
extern "C" {
#endif

int ifn_sleep(struct ifnet * __if);

int ifn_wakeup(struct ifnet * __if);

int ifn_init(struct ifnet * __if);

void * ifn_mmap(struct ifnet * __if, int __length);

int ifn_send(struct ifnet * __if, const uint8_t * __dst, 
			 int __proto, const void * __buf, int __len);

int ifn_add(struct ifnet * __if);

int ifn_del(struct ifnet * __if);

int in_broadcast(in_addr_t __addr, struct ifnet * __if);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_IFNET_H__ */
