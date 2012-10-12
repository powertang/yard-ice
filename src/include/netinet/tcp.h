/* $Id: tcp.h,v 2.1 2008/05/23 03:31:03 bob Exp $ 
 *
 * File:	netinet/tcp.h
 * Module:	
 * Project:	AT91X40DK	
 * Author:	Robinson Mittmann (bob@cnxtech.com)
 * Target:	arm7tdmi
 * Comment:	generic ipv4 routing scheme	
 * Copyright(c) 2003,2004 CNX Technologies. All Rights Reserved.
 *
 */

#ifndef __NETINET_TCP_H__
#define __NETINET_TCP_H__

#include <stdint.h>

typedef uint32_t tcp_seq;

/* connection count per rfc1644 */
typedef uint32_t tcp_cc;               

struct tcphdr {
	uint16_t th_sport;               /* source port */
	uint16_t th_dport;               /* destination port */
	tcp_seq th_seq;                 /* sequence number */
	tcp_seq th_ack;                 /* acknowledgement number */
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t th_x2:4,                /* (unused) */
	        th_off:4;               /* data offset */
#elif  BYTE_ORDER == BIG_ENDIAN
	uint8_t th_off:4,               /* data offset */
	        th_x2:4;                /* (unused) */
#else
#error "Adjust your <bits/endian.h> defines"
#endif
	uint8_t th_flags;
	uint16_t th_win;                 /* window */
	uint16_t th_sum;                 /* checksum */
	uint16_t th_urp;                 /* urgent pointer */
	uint8_t th_opt[0];              /* options */	
};


/* length of header in bytes (no options) */
#define TCP_HEADER_LEN sizeof(struct tcphdr)
/* max length of header in bytes */
#define TCP_MAXHLEN (0x0f << 2)
/* max space left for options */
#define TCP_MAXOLEN (TCP_MAXHLEN -  sizeof(struct tcphdr))

#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG)

#define TCPOPT_END              0
#define TCPOPT_NOP              1
#define TCPOPT_MSS              2
#define TCPOLEN_MSS             4
#define TCPOPT_WINDOW           3
#define TCPOLEN_WINDOW          3
#define TCPOPT_SACK_PERMITTED   4
#define TCPOLEN_SACK_PERMITTED  2
#define TCPOPT_SACK             5
#define TCPOPT_TIMESTAMP        8
#define TCPOLEN_TIMESTAMP       10

/*
 * User-settable options (used with setsockopt).
 */
#define TCP_NODELAY     0x01    /* don't delay send to coalesce packets */
#define TCP_MAXSEG      0x02    /* set maximum segment size */
#define TCP_NOPUSH      0x04    /* don't push last block of write */
#define TCP_NOOPT       0x08    /* don't use TCP options */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NETINET_TCP_H__ */

