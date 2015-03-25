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
 * @file console.c
 * @brief YARD-ICE UART console
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#include <sys/stm32f.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arch/cortex-m3.h>
#include <sys/serial.h>
#include <sys/delay.h>

#include <thinkos.h>
#define __THINKOS_IRQ__
#include <thinkos_irq.h>

#include <sys/dcclog.h>

#define UART_TX_FIFO_BUF_LEN 128
#define UART_RX_FIFO_BUF_LEN 128

#define UART_IRQ_PRIORITY IRQ_PRIORITY_REGULAR

struct uart_fifo {
	volatile uint32_t head;
	volatile uint32_t tail;
	uint32_t mask;
	uint32_t len;
	uint8_t buf[];
};

static inline void uart_fifo_init(struct uart_fifo * fifo, int len)
{
	fifo->head = 0;
	fifo->tail = 0;
	fifo->len = len;
	fifo->mask = len - 1;
}

static inline uint8_t uart_fifo_get(struct uart_fifo * fifo)
{
	return fifo->buf[fifo->tail++ & fifo->mask];
}

static inline void uart_fifo_put(struct uart_fifo * fifo, int c)
{
	fifo->buf[fifo->head++ & fifo->mask] = c;
}

static inline bool uart_fifo_is_empty(struct uart_fifo * fifo)
{
	return (fifo->tail == fifo->head) ? true : false;
}

static inline bool uart_fifo_is_full(struct uart_fifo * fifo)
{
	return ((fifo->head - fifo->tail) == fifo->len) ? true : false;
}

static inline bool uart_fifo_is_half_full(struct uart_fifo * fifo)
{
	return ((fifo->head - fifo->tail) > (fifo->len / 2)) ? true : false;
}

struct serial_dev {
	int32_t tx_flag;
	int32_t rx_flag;
	struct uart_fifo tx_fifo;
	uint8_t tx_buf[UART_TX_FIFO_BUF_LEN];
	struct uart_fifo rx_fifo;
	uint8_t rx_buf[UART_RX_FIFO_BUF_LEN];
	uint32_t * txie;
	struct stm32_usart * uart;
};

int serial_read(struct serial_dev * dev, char * buf, 
				unsigned int len, unsigned int msec)
{

	char * cp = (char *)buf;
	int n = 0;
	int c;

	DCC_LOG(LOG_INFO, "read");

	while (uart_fifo_is_empty(&dev->rx_fifo)) {
		DCC_LOG(LOG_INFO, "wait...");
		thinkos_flag_take(dev->rx_flag);
		DCC_LOG(LOG_INFO, "wakeup.");
	}

	do {
		if (n == len) {
			break;
		}
		c = uart_fifo_get(&dev->rx_fifo);
		cp[n++] = c;
	} while (!uart_fifo_is_empty(&dev->rx_fifo));

	DCC_LOG2(LOG_INFO, "[%d] n=%d", thinkos_thread_self(), n);

	return n;
}

static void uart_putc(struct serial_dev * dev, int c)
{
	while (uart_fifo_is_full(&dev->tx_fifo)) {
		/* enable TX interrupt */
		DCC_LOG(LOG_TRACE, "wait...");
		thinkos_flag_take(dev->tx_flag);
		DCC_LOG(LOG_TRACE, "wakeup");
	}

	uart_fifo_put(&dev->tx_fifo, c);
	*dev->txie = 1; 
}

int serial_write(struct serial_dev * dev, const void * buf, 
				 unsigned int len)
{
	char * cp = (char *)buf;
	int c;
	int n;

	DCC_LOG1(LOG_INFO, "len=%d", len);

	for (n = 0; n < len; n++) {
		c = cp[n];
		uart_putc(dev, c);
	}

	DCC_LOG1(LOG_INFO, "cnt=%d", n);

	return n;
}


int serial_config_get(struct serial_dev * dev, struct serial_config * cfg)
{
//	struct stm32f_usart * uart = dev->uart;

	return 0;
}

int serial_config_set(struct serial_dev * dev, 
					  const struct serial_config * cfg)
{
	struct stm32_usart * uart = dev->uart;
	uint32_t flags;

	DCC_LOG(LOG_TRACE, "...");

	stm32_usart_baudrate_set(uart, cfg->baudrate);

	flags = CFG_TO_FLAGS(cfg);

	stm32_usart_mode_set(uart, flags);

	return 0;
}


struct serial_dev serial_dev;

void stm32f_usart6_isr(void)
{
	struct serial_dev * dev = &serial_dev;
	struct stm32_usart * uart = dev->uart;
	uint32_t sr;
	int c;
	
	sr = uart->sr & uart->cr1;

	if (sr & USART_RXNE) {
		DCC_LOG(LOG_INFO, "RXNE");
		c = uart->dr;
		if (!uart_fifo_is_full(&dev->rx_fifo)) { 
			uart_fifo_put(&dev->rx_fifo, c);
		} else {
			DCC_LOG(LOG_INFO, "RX fifo full!");
		}
		
		if (uart_fifo_is_half_full(&dev->rx_fifo)) 
			__thinkos_flag_give(dev->rx_flag);
	}	

	if (sr & USART_IDLE) {
		DCC_LOG(LOG_INFO, "IDLE");
		c = uart->dr;
		(void)c;
		__thinkos_flag_give(dev->rx_flag);
	}

	if (sr & USART_TXE) {
		DCC_LOG(LOG_INFO, "TXE");
		if (uart_fifo_is_empty(&dev->tx_fifo)) {
			/* disable TXE interrupts */
			*dev->txie = 0; 
			__thinkos_flag_give(dev->tx_flag);
		} else {
			uart->dr = uart_fifo_get(&dev->tx_fifo);
		}
	}

}

#define UART_TX STM32_GPIOC, 6
#define UART_RX STM32_GPIOC, 7

static void io_init(void)
{
	stm32_gpio_clock_en(STM32_GPIOC);

	/* UART TX */
	stm32_gpio_mode(UART_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
	stm32_gpio_af(UART_TX, GPIO_AF8);
	/* UART RX */
	stm32_gpio_mode(UART_RX, ALT_FUNC, PULL_UP);
	stm32_gpio_af(UART_RX, GPIO_AF8);
}

struct serial_dev * serial_open(void)
{
	struct stm32_usart * uart = STM32_USART6;
	struct serial_dev * dev = &serial_dev;

	DCC_LOG(LOG_INFO, "...");
	dev->rx_flag = thinkos_flag_alloc(); 
	dev->tx_flag = thinkos_flag_alloc(); 
	uart_fifo_init(&dev->tx_fifo, UART_TX_FIFO_BUF_LEN);
	uart_fifo_init(&dev->rx_fifo, UART_RX_FIFO_BUF_LEN);

	dev->txie = CM3_BITBAND_DEV(&uart->cr1, 7);
	dev->uart = uart;

	io_init();

	stm32_usart_init(uart);
	stm32_usart_baudrate_set(uart, 115200);
	stm32_usart_mode_set(uart, SERIAL_8N1);
	stm32_usart_enable(uart);

	cm3_irq_pri_set(STM32_IRQ_USART6, UART_IRQ_PRIORITY);
	cm3_irq_enable(STM32_IRQ_USART6);

	/* enable RX interrupt */
	uart->cr1 |= USART_RXNEIE | USART_IDLEIE;

	return dev;
}
