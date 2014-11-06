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
 * @file yard-ice.c
 * @brief YARD-ICE application main
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#ifdef CONFIG_H
#include "config.h"
#endif

#include <sys/stm32f.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/serial.h>
#include <sys/delay.h>
#include <sys/dcclog.h>
#include <tcpip/tcp.h>
#include <netinet/in.h>

#include <thinkos.h>
#define __THINKOS_SYS__
#include <thinkos_sys.h>

#include <tcpip/ethif.h>
#include <tcpip/route.h>
#include <tcpip/loopif.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void tcpip_init(void);
void env_init(void);
void stdio_init(void);

/*
---------------------------------------------------------------------------
 Display TCP/IP stack statistics
---------------------------------------------------------------------------
*/
#if 0
void show_tcpip_stat(void)
{
	struct proto_stat stat;

	printf("PROTO |   RX OK |  RX ERR | RX DROP |");
	printf("   TX OK |  TX ERR | TX DROP |\n");
	printf("------+---------+---------+---------+");
	printf("---------+---------+---------+\n");
	etharp_proto_getstat(&stat, 0);
	printf("ARP   | %7d | %7d | %7d |", stat.rx_ok, stat.rx_err, stat.rx_drop);
	printf(" %7d | %7d | %7d |\n", stat.tx_ok, stat.tx_err, stat.tx_drop);
	ip_proto_getstat(&stat, 0);
	printf("IP    | %7d | %7d | %7d |", stat.rx_ok, stat.rx_err, stat.rx_drop);
	printf(" %7d | %7d | %7d |\n", stat.tx_ok, stat.tx_err, stat.tx_drop);
	icmp_proto_getstat(&stat, 0);
	printf("ICMP  | %7d | %7d | %7d |", stat.rx_ok, stat.rx_err, stat.rx_drop);
	printf(" %7d | %7d | %7d |\n", stat.tx_ok, stat.tx_err, stat.tx_drop);
	tcp_proto_getstat(&stat, 0);
	printf("TCP   | %7d | %7d | %7d |", stat.rx_ok, stat.rx_err, stat.rx_drop);
	printf(" %7d | %7d | %7d |\n", stat.tx_ok, stat.tx_err, stat.tx_drop);
	udp_proto_getstat(&stat, 0);
	printf("UDP   | %7d | %7d | %7d |", stat.rx_ok, stat.rx_err, stat.rx_drop);
	printf(" %7d | %7d | %7d |\n", stat.tx_ok, stat.tx_err, stat.tx_drop);
	printf("\n");

}
#endif

extern const char zarathustra_0[];
extern const char zarathustra_1[];
extern const char zarathustra_2[];
extern const char zarathustra_3[];
extern const char zarathustra_4[];
extern const char zarathustra_5[];
extern const char zarathustra_6[];
extern const char zarathustra_7[];
extern const char zarathustra_8[];
extern const char zarathustra_9[];
extern const char zarathustra_10[];

const char const * zarathustra[] = {
	zarathustra_0,
	zarathustra_1,
	zarathustra_2,
	zarathustra_3,
	zarathustra_4,
	zarathustra_5,
	zarathustra_6,
	zarathustra_7,
	zarathustra_8,
	zarathustra_9,
	zarathustra_10,
	NULL
};

int tcp_server_task(void * arg)
{
	struct tcp_pcb * svc;
	struct tcp_pcb * tp;
	uint32_t start;
	uint32_t end;
	unsigned int ms;
	char * s;
	int i;
	int n;
	int size;
	int again;
	int port = 100;

	svc = tcp_alloc();

	tcp_bind(svc, INADDR_ANY, htons(port));

	if (tcp_listen(svc, 1) != 0) {
		printf("Can't register the TCP listner!\n");
		return -1;
	}

//	printf(" - %s(%d): starting...\n", __FUNCTION__, id);

	printf("Listening on port %d.\n", port);

	for (;;) {
		printf("Wating for connection.\n");
		if ((tp = tcp_accept(svc)) == NULL) {
			printf("tcp_accept() failed!\n");
			break;
		}

		printf("Connection accepted.\n");

		again = 100;

		size = 0;
//		clock_gettime(CLOCK_REALTIME, &start);
		start = __thinkos_ticks();

		do {
			i = 0;
			again--;
			while ((s = (char *)zarathustra[i++]) != NULL) {
				n = strlen(s);
				if ((n = tcp_send(tp, s, n, 0)) < 0) {
					again = 0;
					break;
				}
				size += n;
			}
		} while (again);

		end = __thinkos_ticks();

		tcp_close(tp);

		ms = (int32_t)(end - start);

		printf("EOT: size=%d tm=%d(ms) speed=%d(KiB/s)\n", size, ms, size / ms);
//		show_tcpip_stat();
	}

	return 0;
}


int network_config(void)
{
	struct ifnet * ifn;
	in_addr_t ip_addr;
	in_addr_t netmask = INADDR_ANY;
	in_addr_t gw_addr = INADDR_ANY;
	char s[64];
	char * env;
	int dhcp;

	DCC_LOG(LOG_TRACE, "tcpip_init().");
	tcpip_init();

	if ((env = getenv("IPCFG")) == NULL) {
		/* default configuration */
		strcpy(s, "192.168.0.80 255.255.255.0 192.168.0.1 0");
		/* set the default configuration */
		setenv("IPCFG", s, 1);
	} else {
		strcpy(s, env);
	}

	if (!inet_aton(strtok(s, " ,"), (struct in_addr *)&ip_addr)) {
		DCC_LOG(LOG_WARNING, "inet_aton() failed.");
		return -1;
	}

	if (inet_aton(strtok(NULL, " ,"), (struct in_addr *)&netmask)) {
		if (inet_aton(strtok(NULL, " ,"), (struct in_addr *)&gw_addr)) {
			dhcp = strtoul(strtok(NULL, ""), NULL, 0);
			(void)dhcp;
		}
	}

	/* initialize the Ethernet interface */
	/* configure the ip address */
	ifn = ethif_init(ip_addr, netmask);
//	ifn = loopif_init(ip_addr, netmask);

	ifn_getname(ifn, s);
	ifn_ipv4_get(ifn, &ip_addr, &netmask);

	if (gw_addr != INADDR_ANY) {
		/* add the default route (gateway) to ethif */
		route_add(INADDR_ANY, INADDR_ANY, gw_addr, ifn);
	}

	return 0;
}

uint32_t server_stack[256];

int main(int argc, char ** argv)
{
	DCC_LOG_INIT();
	DCC_LOG_CONNECT();

	cm3_udelay_calibrate();

	stdio_init();

	env_init();

	printf("\n");
	printf("---------------------------------------------------------\n");
	printf(" ThinkOS (TCP Test)\n");
	printf("---------------------------------------------------------\n");
	printf("\n");

	thinkos_init(THINKOS_OPT_PRIORITY(0) | THINKOS_OPT_ID(0));

	network_config();

	thinkos_thread_create((void *)tcp_server_task, (void *)NULL,
						  server_stack, sizeof(server_stack), 
						  THINKOS_OPT_PRIORITY(4) | THINKOS_OPT_ID(8));

	for (;;) {
		thinkos_sleep(1000);
	}

	return 0;
}


