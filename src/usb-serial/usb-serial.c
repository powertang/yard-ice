/* 
 * File:	 usb-test.c
 * Author:   Robinson Mittmann (bobmittmann@gmail.com)
 * Target:
 * Comment:
 * Copyright(C) 2011 Bob Mittmann. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <sys/stm32f.h>
#include <arch/cortex-m3.h>
#include <sys/delay.h>
#include <sys/serial.h>
#include <sys/param.h>
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <thinkos.h>

#include <sys/dcclog.h>
#include <sys/usb-cdc.h>

struct serial_dev * serial_open(struct stm32f_usart * uart);

int serial_write(struct serial_dev * dev, const void * buf, 
				 unsigned int len);

int serial_read(struct serial_dev * dev, char * buf, 
				unsigned int len, unsigned int msec);

void led_lock(void);
void led_unlock(void);
void led1_flash(unsigned int cnt);
void led2_flash(unsigned int cnt);
void led1_on(void);
void led1_off(void);
void led2_on(void);
void led2_off(void);
void leds_init(void);

#ifndef USB_DEBUG
#ifdef DEBUG
#define USB_DEBUG 1
#else
#define USB_DEBUG 0
#endif
#endif

#if USB_DEBUG
volatile uint32_t dbg_msg_dump;

const char dbg_msg[] = "\r\n1.\r\n"
"When Zarathustra was thirty years old, he left his home and the lake of\r\n"
"his home, and went into the mountains.  There he enjoyed his spirit and\r\n"
"solitude, and for ten years did not weary of it.  But at last his heart\r\n"
"changed,--and rising one morning with the rosy dawn, he went before the\r\n"
"sun, and spake thus unto it:\r\n\r\n"
"Thou great star!  What would be thy happiness if thou hadst not those for\r\n"
"whom thou shinest!\r\n\r\n"
"For ten years hast thou climbed hither unto my cave:  thou wouldst have\r\n"
"wearied of thy light and of the journey, had it not been for me, mine\r\n"
"eagle, and my serpent.\r\n\r\n"
"But we awaited thee every morning, took from thee thine overflow\r\n"
"and blessed thee for it.\r\n\r\n"
"Lo!  I am weary of my wisdom, like the bee that hath gathered too much\r\n"
"honey; I need hands outstretched to take it.\r\n\r\n"
"I would fain bestow and distribute, until the wise have once more become\r\n"
"joyous in their folly, and the poor happy in their riches.\r\n\r\n"
"Therefore must I descend into the deep:  as thou doest in the evening,\r\n"
"when thou goest behind the sea, and givest light\r\n"
"also to the nether-world,\r\n"
"thou exuberant star!\r\n\r\n"
"Like thee must I GO DOWN, as men say, to whom I shall descend.\r\n\r\n"
"Bless me, then, thou tranquil eye, that canst behold even the greatest\r\n"
"happiness without envy!\r\n\r\n"
"Bless the cup that is about to overflow,\r\n"
"that the water may flow golden out\r\n"
"of it, and carry everywhere the reflection of thy bliss!\r\n\r\n"
"Lo!  This cup is again going to empty itself, and Zarathustra is again\r\n"
"going to be a man.\r\n\r\n"
"Thus began Zarathustra's down-going.\r\n\r\n\r\n";
#endif

struct vcom {
	struct serial_dev * serial;
	usb_cdc_class_t * cdc;
	struct serial_status ser_stat;
};

#define VCOM_BUF_SIZE 128

#define USB_FS_DP STM32F_GPIOA, 12
#define USB_FS_DM STM32F_GPIOA, 11

#define USB_FS_VBUS STM32F_GPIOB, 6 /* PB6 */

#define PUSHBTN_IO STM32F_GPIOB, 8
#define EXTRST0_IO STM32F_GPIOB, 0 
#define EXTRST1_IO STM32F_GPIOA, 5 /* PA5 (connected to USART2_CK/ USART3_TX) */

#define USART1_TX STM32F_GPIOA, 9
#define USART1_RX STM32F_GPIOA, 10

#define USART2_TX STM32F_GPIOA, 2
#define USART2_RX STM32F_GPIOA, 3

#define USART3_TX STM32F_GPIOB, 10
#define USART3_RX STM32F_GPIOB, 11

void io2_sel_pa5(void)
{
	stm32f_gpio_mode(USART3_TX, INPUT, 0);
	stm32f_gpio_mode(EXTRST1_IO, OUTPUT, OPEN_DRAIN | PULL_UP);
}

void io_sel_usart3(void)
{
	stm32f_gpio_mode(EXTRST1_IO, INPUT, 0);
	stm32f_gpio_mode(USART3_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
}

void io_sel_i2c(void)
{
}

void io_sel_rts(void)
{
}

/* USART1 and USART2 pins are connected together.
   Only one TX pin must be enable at any time */

/* Select USART2 TX */
void io_sel_usart2(void)
{
	stm32f_gpio_mode(USART1_TX, INPUT, 0);
	stm32f_gpio_mode(USART2_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
}

/* Select USART1 TX */
void io_sel_usart1(void)
{
	stm32f_gpio_mode(USART2_TX, INPUT, 0);
	stm32f_gpio_mode(USART1_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
}

void usb_vbus(bool on)
{
//	if (on)
//		stm32f_gpio_set(USB_FS_VBUS);
//	else
//		stm32f_gpio_clr(USB_FS_VBUS);

	if (on)
		stm32f_gpio_mode(USB_FS_VBUS, OUTPUT, PUSH_PULL | SPEED_LOW);
	else
		stm32f_gpio_mode(USB_FS_VBUS, INPUT, 0);
}

void io_init(void)
{
	struct stm32f_rcc * rcc = STM32F_RCC;

	stm32f_gpio_clock_en(STM32F_GPIOA);
	stm32f_gpio_clock_en(STM32F_GPIOB);

	/* Enable Alternate Functions IO clock */
	rcc->apb2enr |= RCC_AFIOEN;

	/* UART1 TX (disabled) */
	stm32f_gpio_mode(USART1_TX, INPUT, 0);
	/* UART1 RX */
	stm32f_gpio_mode(USART1_RX, INPUT, PULL_UP);

	/* Primary UART TX */
	stm32f_gpio_mode(USART2_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
	/* Primary UART RX */
	stm32f_gpio_mode(USART2_RX, INPUT, PULL_UP);

	/* Secondary UART TX */
	stm32f_gpio_mode(USART3_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
	/* Secondary UART RX */
	stm32f_gpio_mode(USART3_RX, INPUT, PULL_UP);

	/* Push button */
	stm32f_gpio_mode(PUSHBTN_IO, INPUT, PULL_UP);

	/* External Reset pins */
	stm32f_gpio_mode(EXTRST0_IO, OUTPUT, PUSH_PULL | SPEED_LOW);
	stm32f_gpio_set(EXTRST0_IO);

	stm32f_gpio_mode(EXTRST1_IO, INPUT, 0);
	stm32f_gpio_set(EXTRST1_IO);

//	stm32f_gpio_mode(USB_FS_VBUS, OUTPUT, PUSH_PULL | SPEED_LOW);
//	stm32f_gpio_clr(USB_FS_VBUS);

	stm32f_gpio_mode(USB_FS_VBUS, INPUT, 0);
	stm32f_gpio_set(USB_FS_VBUS);
}

void system_reset(void)
{
	DCC_LOG(LOG_TRACE, "...");
    CM3_SCB->aircr =  SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
	for(;;);
}

#define LOOP_TIME 50 
#define BUSY_TIME (5000 / LOOP_TIME)

static int push_btn_stat(void)
{
	return stm32f_gpio_stat(PUSHBTN_IO) ? 1 : 0;
}

enum {
	EVENT_NONE,
	EVENT_BTN_PRESSED,
	EVENT_BTN_RELEASED,
	EVENT_EXT_RST_TIMEOUT,
	EVENT_SYS_RST_TIMEOUT
};

enum {
	EXT_RST_OFF,
	EXT_RST_ON,
	EXT_RST_WAIT
};


int button_task(void)
{
	int btn_st[2];
	int sys_rst_tmr = 0;
	int ext_rst_tmr = 0;
	int ext_rst_st = EXT_RST_OFF;
	int event;
	int i;

	DCC_LOG1(LOG_TRACE, "[%d] started.", thinkos_thread_self());

	btn_st[0] = push_btn_stat();

	while (1) {
		thinkos_sleep(LOOP_TIME);

		btn_st[1] = push_btn_stat();
		if (btn_st[1] != btn_st[0]) {
			/* process push button */
			event = btn_st[1] ? EVENT_BTN_PRESSED : EVENT_BTN_RELEASED;
			btn_st[0] = btn_st[1];
		} else if (ext_rst_tmr) {
			/* process external reset timer */
			event = (--ext_rst_tmr == 0) ? EVENT_EXT_RST_TIMEOUT: EVENT_NONE;
		} else if (sys_rst_tmr) {
			/* process system reset timer */
			event = (--sys_rst_tmr == 0) ? EVENT_SYS_RST_TIMEOUT: EVENT_NONE;
		} else {
			event = EVENT_NONE;
		}

		switch (event) {

		case EVENT_BTN_PRESSED:
			DCC_LOG(LOG_TRACE, "BTN_PRESSED");
			if (ext_rst_st == EXT_RST_OFF) {
				/* start external reset timer */
				ext_rst_tmr = 500 / LOOP_TIME;
				/* start system reset timer */
				sys_rst_tmr = 5000 / LOOP_TIME;
				stm32f_gpio_set(EXTRST1_IO);
				stm32f_gpio_set(EXTRST0_IO);
				led_lock();
				led2_on();
				ext_rst_st = EXT_RST_ON;
			}
#if USB_DEBUG
			if (dbg_msg_dump == 0)
				dbg_msg_dump = 1;
			else
				dbg_msg_dump <<= 1;
#endif
			break;

		case EVENT_BTN_RELEASED:
			DCC_LOG(LOG_TRACE, "BTN_RELEASED");
//			if (ext_rst_st == EXT_RST_WAIT) {
//				ext_rst_st = EXT_RST_OFF;
//			}
			/* reset system reset timer */
			sys_rst_tmr = 0;
			break;

		case EVENT_EXT_RST_TIMEOUT:
			DCC_LOG(LOG_TRACE, "EXT_RST_TIMEOUT");
			stm32f_gpio_clr(EXTRST0_IO);
			stm32f_gpio_clr(EXTRST1_IO);
			led2_off();
			led_unlock();
			ext_rst_st = EXT_RST_OFF;
			break;

		case EVENT_SYS_RST_TIMEOUT:
			DCC_LOG(LOG_TRACE, "SYS_RST_TIMEOUT");
			led_lock();

			for (i = 0; i < 10; ++i) {
				led2_on();
				thinkos_sleep(100);
				led2_off();
				thinkos_sleep(200);
			}

			system_reset();
			break;
		}
	}
	return 0;
}

int usb_recv_task(struct vcom * vcom)
{
	struct serial_dev * serial = vcom->serial;
	usb_cdc_class_t * cdc = vcom->cdc;
	char buf[VCOM_BUF_SIZE];
	int len;

	DCC_LOG1(LOG_TRACE, "[%d] started.", thinkos_thread_self());

	for (;;) {
		len = usb_cdc_read(cdc, buf, VCOM_BUF_SIZE, 100);
		led1_flash(1);
		serial_write(serial, buf, len);
	}

	return 0;
}

int serial_recv_task(struct vcom * vcom)
{
	struct serial_dev * serial = vcom->serial;
	struct usb_cdc_class * cdc = vcom->cdc;
	char buf[VCOM_BUF_SIZE];
	int len;

	DCC_LOG1(LOG_TRACE, "[%d] started.", thinkos_thread_self());

	for (;;) {
		len = serial_read(serial, buf, VCOM_BUF_SIZE, 100);
		led2_flash(1);
		usb_cdc_write(cdc, buf, len);
	}

	return 0;
}

int serial_ctrl_task(struct vcom * vcom)
{
	struct serial_dev * serial = vcom->serial;
	struct usb_cdc_class * cdc = vcom->cdc;
	struct usb_cdc_state prev_state;
	struct usb_cdc_state state;

	DCC_LOG1(LOG_TRACE, "[%d] started.", thinkos_thread_self());

	memset(&prev_state, 0, sizeof(struct usb_cdc_state));

	while (1) {
		usb_cdc_state_get(cdc, &state);
		if ((state.cfg.baudrate != prev_state.cfg.baudrate) ||
			(state.cfg.databits != prev_state.cfg.databits) ||
			(state.cfg.parity != prev_state.cfg.parity) ||
			(state.cfg.stopbits != prev_state.cfg.stopbits)) {
			serial_config_set(serial, &state.cfg);
			prev_state.cfg = state.cfg;
		}

		if (state.ctrl.dtr != prev_state.ctrl.dtr) {
			vcom->ser_stat.dsr = state.ctrl.dtr;
			usb_cdc_status_set(cdc, &vcom->ser_stat);
			prev_state.ctrl = state.ctrl;
		}

		usb_cdc_ctl_wait(cdc, 0);
	}
	return 0;
}

uint32_t button_stack[32];
uint32_t serial_ctrl_stack[32];
uint32_t serial_recv_stack[(VCOM_BUF_SIZE / 4) + 64];
uint32_t usb_recv_stack[(VCOM_BUF_SIZE / 4) + 64];

int main(int argc, char ** argv)
{
	uint64_t esn;
	struct vcom vcom;
	unsigned int i;
	struct stm32f_usart * uart = STM32F_USART2;

	DCC_LOG_INIT();
	DCC_LOG_CONNECT();

	/* calibrate usecond delay loop */
	cm3_udelay_calibrate();

	DCC_LOG(LOG_TRACE, "1. io_init()");
	io_init();

	DCC_LOG(LOG_TRACE, "2. thinkos_init()");
	thinkos_init(THINKOS_OPT_PRIORITY(8) | THINKOS_OPT_ID(7));

	leds_init();

	stm32f_usart_init(uart);
	stm32f_usart_baudrate_set(uart, 9600);
	stm32f_usart_mode_set(uart, SERIAL_8N1);
	stm32f_usart_enable(uart);

	vcom.serial = serial_open(uart);

	esn = *((uint64_t *)STM32F_UID);
	DCC_LOG2(LOG_TRACE, "ESN=0x%08x%08x", esn >> 32, esn);

	vcom.cdc = usb_cdc_init(&stm32f_usb_fs_dev, esn);

	thinkos_thread_create((void *)button_task, (void *)NULL,
						  button_stack, sizeof(button_stack),
						  THINKOS_OPT_PRIORITY(8) | THINKOS_OPT_ID(5));

	thinkos_thread_create((void *)usb_recv_task, (void *)&vcom,
						  usb_recv_stack, sizeof(usb_recv_stack),
						  THINKOS_OPT_PRIORITY(1) | THINKOS_OPT_ID(1));

	thinkos_thread_create((void *)serial_recv_task, (void *)&vcom,
						  serial_recv_stack, sizeof(serial_recv_stack),
						  THINKOS_OPT_PRIORITY(1) | THINKOS_OPT_ID(2));

	thinkos_thread_create((void *)serial_ctrl_task, (void *)&vcom,
						  serial_ctrl_stack, sizeof(serial_ctrl_stack),
						  THINKOS_OPT_PRIORITY(4) | THINKOS_OPT_ID(3));

	usb_vbus(true);

	for (i = 0; ; ++i) {
		thinkos_sleep(5000);
		led2_flash(1);
		DCC_LOG1(LOG_TRACE, "%d tick.", i);
#if USB_DEBUG
		if (dbg_msg_dump) {
			int cnt = dbg_msg_dump;
			int j;
			for (j = 1; j <= cnt; ++j) {
				DCC_LOG1(LOG_TRACE, "dump: %d.", j);
				usb_cdc_write(vcom.cdc, dbg_msg, sizeof(dbg_msg));
			}
			dbg_msg_dump = 0;
		}
#endif

//		vcom.ser_stat.dsr = i & 1;
//		vcom.ser_stat.ri = i & 1;
//		vcom.ser_stat.dcd = i & 1;
//		vcom.ser_stat.brk = i & 1;

//		usb_cdc_status_set(vcom.cdc, &vcom.ser_stat);
	}

	return 0;
}

