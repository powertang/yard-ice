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

/* GPIO pin description */ 
struct stm32f_io {
	struct stm32f_gpio * gpio;
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

void led_on(int id)
{
	stm32f_gpio_set(led_io[id].gpio, led_io[id].pin);
}

void led_off(int id)
{
	stm32f_gpio_clr(led_io[id].gpio, led_io[id].pin);
}

void leds_all_off(void)
{
	int i;

	for (i = 0; i < sizeof(led_io) / sizeof(struct stm32f_io); ++i)
		stm32f_gpio_clr(led_io[i].gpio, led_io[i].pin);
}

void leds_all_on(void)
{
	int i;

	for (i = 0; i < sizeof(led_io) / sizeof(struct stm32f_io); ++i)
		stm32f_gpio_set(led_io[i].gpio, led_io[i].pin);
}

void leds_init(void)
{
	int i;

	for (i = 0; i < sizeof(led_io) / sizeof(struct stm32f_io); ++i) {
		stm32f_gpio_mode(led_io[i].gpio, led_io[i].pin,
						 OUTPUT, PUSH_PULL | SPEED_LOW);

		stm32f_gpio_clr(led_io[i].gpio, led_io[i].pin);
	}
}


/* ----------------------------------------------------------------------
 * I/O 
 * ----------------------------------------------------------------------
 */

#define PUSH_BTN STM32F_GPIOC, 9

int push_btn_stat(void)
{
	stm32f_gpio_stat(PUSH_BTN) ? 0 : 1;
}

void io_init(void)
{
	/* Enable IO clocks */
	stm32f_gpio_clock_en(STM32F_GPIOA);
	stm32f_gpio_clock_en(STM32F_GPIOB);
	stm32f_gpio_clock_en(STM32F_GPIOC);

	stm32f_gpio_mode(PUSH_BTN, INPUT, PULL_UP);

	stm32f_gpio_mode(STM32F_GPIOB, 10, INPUT, 0);
	stm32f_gpio_mode(STM32F_GPIOB, 11, INPUT, 0);

#if 0
	/* System configuration controller clock enable */
	rcc->apb2enr |= RCC_SYSCFGEN;

	/* Select PC9 for EXTI9 */ 
	syscfg->exticr3 = SYSCFG_EXTI9_PC;
	/* Unmask interrupt */
	exti->imr |= (1 << 9);
	/* Select falling edge trigger */
	exti->ftsr |= (1 << 9);

//	cm3_irq_enable(STM32F_IRQ_EXTI9_5);
#endif
}


