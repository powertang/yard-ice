/* 
 * File:	 dac-test.c
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arch/cortex-m3.h>
#include <sys/delay.h>

#define __THINKOS_SYS__
#include <thinkos_sys.h>

#include <sys/dcclog.h>

#if THINKOS_ENABLE_EXCEPTIONS

void __show_ctrl(uint32_t ctrl)
{
	fprintf(stderr, "[%s ", (ctrl & (1 << 25)) ? "PSP" : "MSP");
	fprintf(stderr, "%s ", (ctrl & (1 << 24)) ? "USER" : "PRIV");
	fprintf(stderr, "PM=%c ", ((ctrl >> 0) & 0x01) + '0');
	fprintf(stderr, "FM=%c ", ((ctrl >> 16) & 0x01) + '0');
	fprintf(stderr, "BPRI=%02x] ", (ctrl >> 8) & 0xff);
}

static void __show_xpsr(uint32_t psr)
{
	fprintf(stderr, "[N=%c ", ((psr >> 31) & 0x01) + '0');
	fprintf(stderr, "Z=%c ", ((psr >> 30) & 0x01) + '0');
	fprintf(stderr, "C=%c ", ((psr >> 29) & 0x01) + '0');
	fprintf(stderr, "V=%c ", ((psr >> 28) & 0x01) + '0');
	fprintf(stderr, "Q=%c ", ((psr >> 27) & 0x01) + '0');
	fprintf(stderr, "ICI/IT=%02x ", ((psr >> 19) & 0xc0) | ((psr >> 10) & 0x3f));
	fprintf(stderr, "XCP=%02x]", psr & 0xff);
}


void thinkos_context_show(const struct thinkos_context * ctx, 
						  uint32_t sp, uint32_t msp, uint32_t psp)
{
	__show_xpsr(ctx->xpsr);

	fprintf(stderr, "\n");

	fprintf(stderr, "   r0=%08x", ctx->r0);
	fprintf(stderr, "   r4=%08x", ctx->r4);
	fprintf(stderr, "   r8=%08x", ctx->r8);
	fprintf(stderr, "  r12=%08x", ctx->r12);
	fprintf(stderr, " xpsr=%08x\n", ctx->xpsr);

	fprintf(stderr, "   r1=%08x", ctx->r0);
	fprintf(stderr, "   r5=%08x", ctx->r5);
	fprintf(stderr, "   r9=%08x", ctx->r9);
	fprintf(stderr, "   sp=%08x", sp);
	fprintf(stderr, "  msp=%08x\n", msp);

	fprintf(stderr, "   r2=%08x", ctx->r2);
	fprintf(stderr, "   r6=%08x", ctx->r6);
	fprintf(stderr, "  r10=%08x", ctx->r10);
	fprintf(stderr, "   lr=%08x",  ctx->lr);
	fprintf(stderr, "  psp=%08x\n", psp);

	fprintf(stderr, "   r3=%08x",  ctx->r3);
	fprintf(stderr, "   r7=%08x",  ctx->r7);
	fprintf(stderr, "  r11=%08x",  ctx->r11);
	fprintf(stderr, "   pc=%08x\n",  ctx->pc);
}

static inline struct thinkos_context * __attribute__((always_inline)) __get_context(void) {
	register struct thinkos_context * ctx;
	asm volatile ("push {r4-r11}\n"
				  "mov  %0, sp\n" : "=r" (ctx));
	return ctx;
}

static inline uint32_t __attribute__((always_inline)) __get_stack(void) {
	register uint32_t sp;
	asm volatile ("tst lr, #4\n" 
				  "ite eq\n" 
				  "mrseq r0, MSP\n" 
				  "mrsne r0, PSP\n" 
				  : "=r" (sp));
	return sp;
}

static void __dump_bfsr(void)
{
	struct cm3_scb * scb = CM3_SCB;
	uint32_t bfsr;

	bfsr = SCB_CFSR_BFSR_GET(scb->cfsr);

	fprintf(stderr, "BFSR=0X%08x", bfsr);

	if (bfsr & BFSR_BFARVALID)  
		fprintf(stderr, " BFARVALID");
	if (bfsr & BFSR_LSPERR)
		fprintf(stderr, " LSPERR");
	if (bfsr & BFSR_STKERR)  
		fprintf(stderr, " STKERR");
	if (bfsr & BFSR_UNSTKERR)  
		fprintf(stderr, " INVPC");
	if (bfsr & BFSR_IMPRECISERR)  
		fprintf(stderr, " IMPRECISERR");
	if (bfsr & BFSR_PRECISERR)
		fprintf(stderr, " PRECISERR");
	if (bfsr & BFSR_IBUSERR)  
		fprintf(stderr, " IBUSERR");


	if (bfsr & BFSR_BFARVALID)  {
		fprintf(stderr, "\n * ADDR = 0x%08x", (int)scb->bfar);
	}
}

static void __dump_ufsr(void)
{
	struct cm3_scb * scb = CM3_SCB;
	uint32_t ufsr;

	ufsr = SCB_CFSR_UFSR_GET(scb->cfsr);

	fprintf(stderr, "UFSR=0x%08x", ufsr);

	if (ufsr & UFSR_DIVBYZERO)  
		fprintf(stderr, " DIVBYZERO");
	if (ufsr & UFSR_UNALIGNED)  
		fprintf(stderr, " UNALIGNED");
	if (ufsr & UFSR_NOCP)  
		fprintf(stderr, " NOCP");
	if (ufsr & UFSR_INVPC)  
		fprintf(stderr, " INVPC");
	if (ufsr & UFSR_INVSTATE)  
		fprintf(stderr, " INVSTATE");
	if (ufsr & UFSR_UNDEFINSTR)  
		fprintf(stderr, " UNDEFINSTR");
}

void __attribute__((naked, noreturn)) cm3_bus_fault_isr(void)
{
	cm3_faultmask_set(1);

	fprintf(stderr, "---\n");
	fprintf(stderr, "Bus fault:");

	DCC_LOG(LOG_ERROR, "Bus fault!");

	__dump_bfsr();

	fprintf(stderr, "\n");

	for(;;);
}

void __attribute__((naked, noreturn)) cm3_usage_fault_isr(void)
{
	cm3_faultmask_set(1);

	fprintf(stderr, "---\n");
	fprintf(stderr, "Usage fault:");

	DCC_LOG(LOG_ERROR, "Usage fault!");

	__dump_ufsr();

	fprintf(stderr, "\n");

	for(;;);
}

void thinkos_exception_dsr(struct thinkos_context * ctx);

void __attribute__((naked, noreturn)) cm3_hard_fault_isr(void)
{
	struct thinkos_context * ctx;
	struct cm3_scb * scb = CM3_SCB;
	uint32_t hfsr;
	uint32_t sp;
	uint32_t msp;
	uint32_t psp;
	uint32_t lr;

	/* save the context */
	ctx = __get_context();

	lr = cm3_lr_get();
	msp = cm3_msp_get();
	psp = cm3_psp_get();

	if (lr & (1 << 4))
		sp = psp;
	else
		sp = msp;

	cm3_faultmask_set(1);

	fprintf(stderr, "---\n");
	fprintf(stderr, "Hard fault:");

	hfsr = scb->hfsr;

	if (hfsr & SCB_HFSR_DEBUGEVT)  
		fprintf(stderr, " DEBUGEVT");
	if (hfsr & SCB_HFSR_FORCED)  
		fprintf(stderr, " FORCED");
	if (hfsr & SCB_HFSR_VECTTBL)  
		fprintf(stderr, " VECTTBL");

	fprintf(stderr, "\n");

	if (hfsr & SCB_HFSR_FORCED) {
		__dump_bfsr();
		fprintf(stderr, "\n");
		__dump_ufsr();
		fprintf(stderr, "\n");
	}

	thinkos_context_show(ctx, sp, msp, psp);

	DCC_LOG(LOG_ERROR, "Hard fault!");
	DCC_LOG1(LOG_ERROR, "HFSR=0x%08x", scb->hfsr);
	DCC_LOG1(LOG_ERROR, "CFSR=0x%08x", scb->cfsr);
	DCC_LOG1(LOG_ERROR, "BFAR=0x%08x", scb->bfar);

	thinkos_exception_dsr(ctx);

	for(;;);
}

void thinkos_default_exception_dsr(struct thinkos_context * ctx)
{
}

void thinkos_exception_dsr(struct thinkos_context *) 
	__attribute__((weak, alias("thinkos_default_exception_dsr")));

const char const thinkos_except_nm[] = "EXCEPT";

#endif /* THINKOS_ENABLE_EXCEPT */
