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
#include <sys/shell.h>
#include <sys/tty.h>

#include <sys/dcclog.h>

#include <thinkos.h>
#define __THINKOS_SYS__
#include <thinkos_sys.h>

#include "board.h"

#include <bacnet/bacnet-ptp.h>
#include "dcc.h"
#include "npdu.h"
#include "handlers.h"
#include "lattice.h"

#define VERSION_NUM "0.1"
#define VERSION_DATE "Mar, 2015"

const char * version_str = "BACnet Demo " \
							VERSION_NUM " - " VERSION_DATE;
const char * copyright_str = "(c) Copyright 2015 - Bob Mittmann";

const char * shell_prompt(void)
{
	return "[WEBSRV]$ ";
}

void shell_greeting(FILE * f) 
{
	fprintf(f, "\n%s", version_str);
	fprintf(f, "\n%s\n\n", copyright_str);
}

extern const struct shell_cmd shell_cmd_tab[];
extern const uint8_t ice40lp384_bin[];
extern const unsigned int sizeof_ice40lp384_bin;

void io_init(void)
{
	stm32_clk_enable(STM32_RCC, STM32_CLK_GPIOA);
	stm32_clk_enable(STM32_RCC, STM32_CLK_GPIOB);
	stm32_clk_enable(STM32_RCC, STM32_CLK_GPIOC);
	stm32_clk_enable(STM32_RCC, STM32_CLK_GPIOD);
	stm32_clk_enable(STM32_RCC, STM32_CLK_GPIOE);

	/* USART5 TX */
	stm32_gpio_mode(UART5_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
	stm32_gpio_af(UART5_TX, GPIO_AF8);
	/* USART5 RX */
	stm32_gpio_mode(UART5_RX, ALT_FUNC, PULL_UP);
	stm32_gpio_af(UART5_RX, GPIO_AF8);

	/* USART6_TX */
	stm32_gpio_mode(UART6_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
	stm32_gpio_af(UART6_TX, GPIO_AF7);
	/* USART6_RX */
	stm32_gpio_mode(UART6_RX, ALT_FUNC, PULL_UP);
	stm32_gpio_af(UART6_RX, GPIO_AF7);

	/* IO init */
	stm32_gpio_mode(RS485_RX, ALT_FUNC, PULL_UP);
	stm32_gpio_af(RS485_RX, GPIO_AF7);

	stm32_gpio_mode(RS485_TX, ALT_FUNC, PUSH_PULL | SPEED_MED);
	stm32_gpio_af(RS485_TX, GPIO_AF7);

#ifdef RS485_CK
	stm32_gpio_mode(RS485_CK, ALT_FUNC, PUSH_PULL | SPEED_MED);
	stm32_gpio_af(RS485_CK, GPIO_AF7);
#endif

#ifdef RS485_TRIG
	stm32_gpio_mode(RS485_TRIG, INPUT, PULL_UP);
#endif

#ifdef RS485_TXEN
	stm32_gpio_mode(RS485_TXEN, OUTPUT, PUSH_PULL | SPEED_MED);
	stm32_gpio_set(RS485_TXEN);
#endif

#ifdef RS485_LOOP
	stm32_gpio_mode(RS485_LOOP, OUTPUT, PUSH_PULL | SPEED_MED);
	stm32_gpio_set(RS485_LOOP);
#endif

#ifdef RS485_MODE
	stm32_gpio_mode(RS485_MODE, OUTPUT, PUSH_PULL | SPEED_LOW);
	stm32_gpio_set(RS485_MODE);
#endif

	lattice_ice40_configure(ice40lp384_bin, sizeof_ice40lp384_bin);
}

struct bacnet_ptp_lnk * ptp_lnk;

int datalink_send_pdu(BACNET_ADDRESS * dest, BACNET_NPDU_DATA * npdu_data,
					  uint8_t * pdu, unsigned pdu_len)
{
	DCC_LOG(LOG_TRACE, "...");
	return bacnet_ptp_send(ptp_lnk, pdu, pdu_len);
}

uint16_t datalink_receive(BACNET_ADDRESS * src, uint8_t * pdu,
						  uint16_t max_pdu, unsigned timeout)
{
	return 0;
}

void datalink_cleanup(void)
{
	DCC_LOG(LOG_TRACE, "...");
	return;
}

void datalink_get_broadcast_address(BACNET_ADDRESS * dest)
{
	DCC_LOG(LOG_TRACE, "...");
	return;
}

void datalink_get_my_address(BACNET_ADDRESS * my_address)
{
	DCC_LOG(LOG_TRACE, "...");
	return; 
}

void datalink_set_interface(char *ifname)
{
	DCC_LOG(LOG_TRACE, "...");
	return;
}

void datalink_set(char *datalink_string)
{
	DCC_LOG(LOG_TRACE, "...");
	return;
}

FILE * serial_tty_open(struct serial_dev * serdev)
{
	struct tty_dev * tty;
	FILE * f_raw;

	f_raw = serial_fopen(serdev);
	tty = tty_attach(f_raw);
	return tty_fopen(tty);
}

int main(int argc, char ** argv)
{
	struct serial_dev * ser5;
	struct serial_dev * ser1;
	FILE * term1;
	FILE * term5;


	uint8_t buf[512];
	int len;

	DCC_LOG_INIT();
	DCC_LOG_CONNECT();

	cm3_udelay_calibrate();

	DCC_LOG(LOG_TRACE, "1. stm32f_nvram_env_init().");
	stm32f_nvram_env_init();

	DCC_LOG(LOG_TRACE, "2. thinkos_init().");
	thinkos_init(THINKOS_OPT_PRIORITY(0) | THINKOS_OPT_ID(0));

	DCC_LOG(LOG_TRACE, "3. io_init()");
	io_init();

	DCC_LOG(LOG_TRACE, "4. console serial init");
	ser5 = stm32f_uart5_serial_init(57600, SERIAL_8N1);
	ser1 = stm32f_uart1_serial_init(460800, SERIAL_8N1);

	term1 = serial_tty_open(ser1);
	term5 = serial_tty_open(ser5);

	DCC_LOG(LOG_TRACE, "4. stdio_init().");
	stderr = term1;
	stdout = term1;
	stdin = term1;

	printf("\n");
	printf("---------------------------------------------------------\n");
	printf(" BACnet Test\n");
	printf("---------------------------------------------------------\n");
	printf("\n");

	DCC_LOG(LOG_TRACE, "5. starting console shell...");

	for (;;) {
		DCC_LOG(LOG_WARNING, "Console shell!");
	
		shell(term5, shell_prompt, shell_greeting, shell_cmd_tab);

		/* BACnet protocol... */
		DCC_LOG(LOG_WARNING, "BACnet Data Link Connection!");
		ptp_lnk = bacnet_ptp_inbound(ser5);

		while ((len = bacnet_ptp_recv(ptp_lnk, buf)) >= 0) {
			DCC_LOG(LOG_TRACE, "BACnet PDU received...");
        	npdu_handler(NULL, buf, len);
		}

	}

	return 0;
}



