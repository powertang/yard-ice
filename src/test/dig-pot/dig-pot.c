/* 
 * File:	 dig-pot.c
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
#include <arch/cortex-m3.h>
#include <sys/delay.h>
#include <sys/serial.h>
#include <sys/param.h>
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <thinkos.h>

#include <sys/dcclog.h>
#include <sys/usb-cdc.h>

/* 
 * MCP402X digital potentiometer protocol 
 * 
 * Increment:
 *
 *  __  ______                              _________
 *  CS        |___________________________|
 *            :
 *            :--LCUR---:--THI--:-TLO-:
 *                      :       :     :  
 *   _     _______       _______       _______
 * U/D  __|       |_____|       |_____|       |_______
 *                      :             :
 *                      :             :
 *                      :              ______________
 *   W                   _____________|
 *      ________________|
 *
 * Decrement:
 *
 *  __  ______                            ___________
 *  CS        |__________________________|
 *            :
 *            :-LCUR-:--THI--:--TLO--:
 *                   :       :       :  
 *   _                _______         _________    
 * U/D  _____________|       |_______|         |________
 *                   :               :
 *                   :               :
 *      _____________                :
 *   W               |_______________
 *                                   |__________________
 *
 * T_LCUR(MIN) = 3uS
 * T_LO(MIN) = 500nS
 * T_HI(MI) = 500nS
 * 
 */


/***********************************************************
  Timer Configuration
 ***********************************************************/

#define DGPOT_STEPS 64
#define DGPOT_CHIPS 2

struct dgpot_drv {
	int8_t ev;
	int8_t mutex;
	uint8_t pos[DGPOT_CHIPS];
} dgpot;


#define DGPOT_NCS0 STM32F_GPIOB, 11
#define DGPOT_NCS1 STM32F_GPIOB, 12
#define DGPOT_UPDWN STM32F_GPIOB, 10

void stm32f_tim1_up_isr(void)
{
	struct stm32f_tim * tim = STM32F_TIM1;

	/* Clear interrupts */
	tim->sr= 0;
}

void stm32f_tim1_up_tim16_isr(void) 
	__attribute__ ((alias ("stm32f_tim1_up_isr")));

#define TIMER_CLK_FREQ 100000

static void dgpot_timer_init(void)
{
	struct stm32f_rcc * rcc = STM32F_RCC;
	struct stm32f_tim * tim = STM32F_TIM1;
	uint32_t div;
	uint32_t pre;
	uint32_t n;

	/* get the total divisior */
	div = ((2 * stm32f_apb1_hz) + (TIMER_CLK_FREQ / 2)) / TIMER_CLK_FREQ;
	/* get the minimum pre scaler */
	pre = (div / 65536) + 1;
	/* get the reload register value */
	n = (div + pre / 2) / pre;

	/* Timer clock enable */
	rcc->apb1enr |= RCC_TIM1EN;
	
	/* Timer configuration */
	tim->psc = pre - 1;
	tim->arr = n - 1;
	tim->cnt = 0;
	tim->egr = 0;
	tim->dier = TIM_UIE; /* Update interrupt enable */
	tim->ccmr1 = TIM_OC1M_PWM_MODE1;
	tim->ccr1 = tim2->arr - 2;
	tim->cr2 = 0;
	/* Clear interrupts */
	tim->sr= 0;
	/* Center mode, one pulse */
	tim->cr1 = TIM_CMS_SET(3) | TIM_OPM; 

	cm3_irq_enable(STM32F_IRQ_TIM1);

}



void dgpot_set(unsigned int cs, unsigned int pos)
{
	struct stm32f_rcc * rcc = STM32F_RCC;
	int diff;

	/* limit the range */
	if (pos > (DGPOT_STEPS - 1))
		pos = (DGPOT_STEPS - 1);

	thinkos_mutex_lock(dgpot.mutex);

	diff = pos - dgpot.pos[i];

	if (diff < 0) {
		/* decrement */
		diff = -diff;
		/* set the repetition counter register */
		tim->rcr = (diff * 2) - 2;
	} else if (diff > 0) {
		/* increment */
		/* set the repetition counter register */
		tim->rcr = (diff * 2) - 1;
	}

	/* Generate an update to load the crc */
	tim->cr1 = TIM_UG;
	/* Center mode, one pulse, enable counter */
	tim->cr1 = TIM_CMS_SET(3) | TIM_OPM | TIM_CEN; 

	/* wait for the end of adjustment ... */
	__thinkos_critical_enter();
	DCC_LOG(LOG_TRACE, "wait ...");
	__thinkos_ev_wait(dev->tx_lock_ev);
	DCC_LOG(LOG_TRACE, "wakeup");
	__thinkos_critical_exit();

	thinkos_mutex_unlock(dgpot.mutex);
}

void dgpot_init(void)
{
	int i;

	stm32f_gpio_mode(DGPOT_NCS0, OUTPUT, PUSH_PULL | SPEED_LOW);
	stm32f_gpio_mode(DGPOT_NCS1, OUTPUT, PUSH_PULL | SPEED_LOW);
	stm32f_gpio_mode(DGPOT_UPDWN, ALT_FUNC, PUSH_PULL | SPEED_LOW);

	stm32f_gpio_set(DGPOT_NCS0);
	stm32f_gpio_set(DGPOT_NCS1);

	dgpot_timer_init();

	dgpot.mutex = thinkos_mutex_alloc();
	dgpot.ev = thinkos_event_alloc();

	for (i = 0; i < DGPOT_CHIPS; ++i) {
		/* artificially set the potentiometer to the 
		   maximum level to allow zeroing */
		dgpot.pos[i] = DGPOT_STEPS - 1;
		dgpot_set(i, 0);
	}

}

#define USART1_TX STM32F_GPIOB, 6
#define USART1_RX STM32F_GPIOB, 7

void io_init(void)
{
	struct stm32f_rcc * rcc = STM32F_RCC;

	DCC_LOG(LOG_MSG, "Configuring GPIO pins...");

	stm32f_gpio_clock_en(STM32F_GPIOA);
	stm32f_gpio_clock_en(STM32F_GPIOB);
	stm32f_gpio_clock_en(STM32F_GPIOC);

	/* Enable Alternate Functions IO clock */
	rcc->apb2enr |= RCC_AFIOEN;

	/* USART1_TX */
	stm32f_gpio_mode(USART1_TX, ALT_FUNC, PUSH_PULL | SPEED_LOW);
	/* USART1_RX */
	stm32f_gpio_mode(USART1_RX, INPUT, PULL_UP);
}

int main(int argc, char ** argv)
{
	struct stm32f_usart * us = STM32F_USART1;
	int i = 0;

	DCC_LOG_INIT();
	DCC_LOG_CONNECT();

	/* calibrate usecond delay loop */
	cm3_udelay_calibrate();

	DCC_LOG(LOG_TRACE, "1. io_init()");
	io_init();

	DCC_LOG(LOG_TRACE, "2. thinkos_init()");
	thinkos_init(THINKOS_OPT_PRIORITY(0) | THINKOS_OPT_ID(32));

	stm32f_usart_init(us);
	stm32f_usart_baudrate_set(us, 115200);
	stm32f_usart_mode_set(us, SERIAL_8N1);
	stm32f_usart_enable(us);

	for (i = 0; ; i++) {
		DCC_LOG1(LOG_TRACE, "%d", i);
		stm32f_usart_putc(us, 'U');
		thinkos_sleep(3000);
	}

	return 0;
}

