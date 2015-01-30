/* 
 * File:	 io.c
 * Author:   Robinson Mittmann (bobmittmann@gmail.com)
 * Target:
 * Comment:
 * Copyright(C) 2013 Bob Mittmann. All Rights Reserved.
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
#include <sys/delay.h>
#include <stdint.h>

#include <sys/dcclog.h>

#include "trace.h"
#include "board.h"
#include "io.h"

#include <thinkos.h>
#define __THINKOS_IRQ__
#include <thinkos_irq.h>

#define POLL_PERIOD_MS 32

#if 0
/* GPIO pin description */ 
struct stm32f_io {
	struct stm32_gpio * gpio;
	uint8_t pin;
};

/* ----------------------------------------------------------------------
 * LEDs 
 * ----------------------------------------------------------------------
 */

const struct stm32f_io led_io[] = {
	{ STM32F_GPIOC, 1 },
	{ STM32F_GPIOC, 14 },
	{ STM32F_GPIOC, 7 },
	{ STM32F_GPIOC, 8 }
};

#define UNLOCKED -1

struct {
	int lock;
	uint8_t tmr[sizeof(led_io) / sizeof(struct stm32f_io)];
} led_drv;

void led_on(int id)
{
	if ((led_drv.lock != UNLOCKED) && (led_drv.lock != thinkos_thread_self()))
		return;

	stm32_gpio_set(led_io[id].gpio, led_io[id].pin);
}

void led_off(int id)
{
	if ((led_drv.lock != UNLOCKED) && (led_drv.lock != thinkos_thread_self()))
		return;

	stm32_gpio_clr(led_io[id].gpio, led_io[id].pin);
}

void led_flash(int id, int ms)
{
	if ((led_drv.lock != UNLOCKED) && (led_drv.lock != thinkos_thread_self()))
		return;

	led_drv.tmr[id] = ms / POLL_PERIOD_MS;
	stm32_gpio_set(led_io[id].gpio, led_io[id].pin);
}

void leds_all_off(void)
{
	int i;

	if ((led_drv.lock != UNLOCKED) && (led_drv.lock != thinkos_thread_self()))
		return;

	for (i = 0; i < sizeof(led_io) / sizeof(struct stm32f_io); ++i)
		stm32_gpio_clr(led_io[i].gpio, led_io[i].pin);
}

void leds_all_on(void)
{
	int i;

	if ((led_drv.lock != UNLOCKED) && (led_drv.lock != thinkos_thread_self()))
		return;

	for (i = 0; i < sizeof(led_io) / sizeof(struct stm32f_io); ++i)
		stm32_gpio_set(led_io[i].gpio, led_io[i].pin);
}

void leds_all_flash(int ms)
{
	int i;

	if ((led_drv.lock != UNLOCKED) && (led_drv.lock != thinkos_thread_self()))
		return;

	for (i = 0; i < sizeof(led_io) / sizeof(struct stm32f_io); ++i) {
		led_drv.tmr[i] = ms / POLL_PERIOD_MS;
		stm32_gpio_set(led_io[i].gpio, led_io[i].pin);
	}
}

void leds_lock(void)
{
	if (led_drv.lock != UNLOCKED)
		return;

	led_drv.lock = thinkos_thread_self();
}

void leds_unlock(void)
{
	if ((led_drv.lock != UNLOCKED) && (led_drv.lock != thinkos_thread_self()))
		return;

	led_drv.lock = UNLOCKED;
}


void leds_init(void)
{
	int i;

	led_drv.lock = UNLOCKED;

	for (i = 0; i < sizeof(led_io) / sizeof(struct stm32f_io); ++i) {
		stm32_gpio_mode(led_io[i].gpio, led_io[i].pin,
						 OUTPUT, PUSH_PULL | SPEED_LOW);

		stm32_gpio_clr(led_io[i].gpio, led_io[i].pin);
	}
}

#endif

/* ----------------------------------------------------------------------
 * Events
 * ----------------------------------------------------------------------
 */

struct event_drv {
	volatile uint32_t bmp;
	uint32_t msk;
	int flag;
} event_drv;

void event_drv_init(void)
{
	event_drv.flag = thinkos_flag_alloc();
	event_drv.msk = 0;
	event_drv.bmp = 0;
}

void event_raise(int event)
{
	if (event) {
		DCC_LOG1(LOG_MSG, "event=%d", event);
		/* set the event bit */
		__bit_mem_wr((uint32_t *)&event_drv.bmp, event - 1, 1);  
		/* signal the flag */
		thinkos_flag_give_i(event_drv.flag);
	}
}

int event_wait(void)
{
	int idx;

	DCC_LOG1(LOG_INFO, "bmp=0x%08x", event_drv.bmp);

	/* get a thread from the ready bitmap */
	while ((idx = __clz(__rbit(event_drv.bmp))) == 32) {
		/* wait for the flag to be set */
		thinkos_flag_take(event_drv.flag);
	}

	DCC_LOG1(LOG_INFO, "2. idx=%d", idx);
	__bit_mem_wr((uint32_t *)&event_drv.bmp, idx, 0);  

	return idx + 1;
}

/* ----------------------------------------------------------------------
 * Push button
 * ----------------------------------------------------------------------
 */

/* Button FSM states */
enum {
	BTN_FSM_IDLE = 0,
	BTN_FSM_CLICK1_WAIT,
	BTN_FSM_PRE_WAIT1,
	BTN_FSM_CLICK2_WAIT,
	BTN_FSM_HOLD_TIME_WAIT,
	BTN_FSM_CLICK_N_HOLD_TIME_WAIT,
	BTN_FSM_HOLD_RELEASE_WAIT,
};

struct btn_drv {
	uint8_t st;
	volatile uint8_t tmr;
	uint8_t fsm;
	int8_t event;
	int flag;
} btn_drv;

/* Button internal events */
enum {
	BTN_PRESSED = 1,
	BTN_RELEASED,
	BTN_TIMEOUT
};

static void btn_drv_fsm(int irq_ev)
{
	int event = EVENT_NONE;

	switch (btn_drv.fsm) {
	case BTN_FSM_IDLE:
		if (irq_ev == BTN_PRESSED) {
			/* double click timer */
			btn_drv.tmr = 500 / POLL_PERIOD_MS;
			btn_drv.fsm = BTN_FSM_CLICK1_WAIT;
		}
		break;

	case BTN_FSM_CLICK1_WAIT:
		/* wait for button release after first press */
		if (irq_ev == BTN_RELEASED) {
			event = EVENT_BTN_CLICK;
			btn_drv.fsm = BTN_FSM_PRE_WAIT1;
		} else if (irq_ev == BTN_TIMEOUT) {
			/* hold timer */
			btn_drv.tmr = 500 / POLL_PERIOD_MS;;
			btn_drv.fsm = BTN_FSM_HOLD_TIME_WAIT;
		}
		break;

	case BTN_FSM_PRE_WAIT1:
		/* wait for button press after first press and release
		 inside the double click timer */
		if (irq_ev == BTN_PRESSED) {
			btn_drv.fsm = BTN_FSM_CLICK2_WAIT;
		} else if (irq_ev == BTN_TIMEOUT) {
			btn_drv.fsm = BTN_FSM_IDLE;
		}
		break;

	case BTN_FSM_CLICK2_WAIT:
		/* wait for button release after press/release/press
		 inside the double click timer */
		if (irq_ev == BTN_RELEASED) {
			event = EVENT_BTN_DBL_CLICK;
			btn_drv.tmr = 0;
			btn_drv.fsm = BTN_FSM_IDLE;
		} else if (irq_ev == BTN_TIMEOUT) {
			btn_drv.fsm = BTN_FSM_IDLE;
			/* hold and click timer */
			btn_drv.tmr = 500 / POLL_PERIOD_MS;;
			btn_drv.fsm = BTN_FSM_CLICK_N_HOLD_TIME_WAIT;
		}
		break;


	case BTN_FSM_HOLD_TIME_WAIT:
		/* wait for button release after press
		 inside the hold timer */
		if (irq_ev == BTN_RELEASED) {
			btn_drv.tmr = 0;
			btn_drv.fsm = BTN_FSM_IDLE;
		} else if (irq_ev == BTN_TIMEOUT) {
			event = EVENT_BTN_HOLD1;
			/* long hold timer */
			btn_drv.tmr = 3500 / POLL_PERIOD_MS;;
			btn_drv.fsm = BTN_FSM_HOLD_RELEASE_WAIT;
		}
		break;

	case BTN_FSM_CLICK_N_HOLD_TIME_WAIT:
		/* wait for button release after press/release/press
		 inside the hold timer */
		if (irq_ev == BTN_RELEASED) {
			btn_drv.tmr = 0;
			btn_drv.fsm = BTN_FSM_IDLE;
		} else if (irq_ev == BTN_TIMEOUT) {
			event = EVENT_BTN_CLICK_N_HOLD;
			/* long hold timer */
			btn_drv.tmr = 3500 / POLL_PERIOD_MS;;
			btn_drv.fsm = BTN_FSM_HOLD_RELEASE_WAIT;
		}
		break;

	case BTN_FSM_HOLD_RELEASE_WAIT:
		/* wait for button release after the hold timer expired */
		if (irq_ev == BTN_RELEASED) {
			btn_drv.tmr = 0;
			btn_drv.fsm = BTN_FSM_IDLE;
		} else if (irq_ev == BTN_TIMEOUT) {
			event = EVENT_BTN_HOLD2;
			btn_drv.fsm = BTN_FSM_IDLE;
		}
		break;
	};

	if (event) {
		event_raise(event);
	}
}


static void btn_drv_init(void)
{
	stm32_gpio_mode(PUSHBTN_IO, INPUT, PULL_UP);

	btn_drv.flag = thinkos_flag_alloc();
	btn_drv.st = stm32_gpio_stat(PUSHBTN_IO) ? 1 : 0;
	btn_drv.fsm = BTN_FSM_IDLE;
}

/* ----------------------------------------------------------------------
 * I/O 
 * ----------------------------------------------------------------------
 */

void stm32f_tim2_isr(void)
{
	struct stm32f_tim * tim = STM32F_TIM2;
	int st;
//	int i;

	/* Clear interrupt flags */
	tim->sr = 0;

#if 0
	/* process led timers */
	for (i = 0; i < sizeof(led_io) / sizeof(struct stm32f_io); ++i) {
		if (led_drv.tmr[i] == 0)
			continue;
		if (--led_drv.tmr[i] == 0) 
			stm32_gpio_clr(led_io[i].gpio, led_io[i].pin);
	}
#endif

	if ((btn_drv.tmr) && (--btn_drv.tmr == 0)) {
		/* process button timer */
		btn_drv_fsm(BTN_TIMEOUT);
	} else {
		/* process push button */
		st = stm32_gpio_stat(PUSHBTN_IO) ? 1 : 0;
		if (btn_drv.st != st) {
			btn_drv.st = st;
			btn_drv_fsm(st ? BTN_PRESSED : BTN_RELEASED);
		}
	}
}

void ui_init(void)
{
	struct stm32_rcc * rcc = STM32_RCC;
	struct stm32f_tim * tim = STM32F_TIM2;
	uint32_t div;
	uint32_t pre;
	uint32_t n;
	uint32_t freq = 1000 / POLL_PERIOD_MS;

	/* Initialize button driver */
	btn_drv_init();

	/* Initialize event driver */
	event_drv_init();

	/* get the total divisior */
	div = ((2 * stm32f_apb1_hz) + (freq / 2)) / freq;
	/* get the minimum pre scaler */
	pre = (div / 65536) + 1;
	/* get the reload register value */
	n = (div * 2 + pre) / (2 * pre);

	DCC_LOG3(LOG_TRACE, "freq=%dHz pre=%d n=%d", freq, pre, n);
	DCC_LOG1(LOG_TRACE, "real freq=%dHz", (2 * stm32f_apb1_hz) / pre / n);

	/* Timer clock enable */
	rcc->apb1enr |= RCC_TIM2EN;
	
	/* Timer configuration */
	tim->psc = pre - 1;
	tim->arr = n - 1;
	tim->cnt = 0;
	tim->egr = 0;
	tim->dier = TIM_UIE; /* Update interrupt enable */
	tim->ccmr1 = TIM_OC1M_PWM_MODE1;
	tim->ccr1 = tim->arr / 2;

	cm3_irq_pri_set(STM32F_IRQ_TIM2, IRQ_PRIORITY_LOW);
	/* Enable interrupt */
	cm3_irq_enable(STM32F_IRQ_TIM2);

	tim->cr1 = TIM_URS | TIM_CEN; /* Enable counter */

}

