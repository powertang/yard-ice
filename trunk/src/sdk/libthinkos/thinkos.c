/* 
 * thikos.c
 *
 * Copyright(C) 2012 Robinson Mittmann. All Rights Reserved.
 * 
 * This file is part of the ThinkOS library.
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

#define __THINKOS_SYS__
#include <thinkos_sys.h>
#include <thinkos.h>
#include <sys/delay.h>

#if THINKOS_ENABLE_JOIN
void thinkos_join_svc(int32_t * arg)
{
	int self = thinkos_rt.active;
	unsigned int th = arg[0];
	unsigned int wq;

#if THINKOS_ENABLE_ARG_CHECK
	if (th >= THINKOS_THREADS_MAX) {
		DCC_LOG1(LOG_ERROR, "object %d is not a thread!", th);
		arg[0] = THINKOS_EINVAL;
		return;
	}
#if THINKOS_ENABLE_THREAD_ALLOC
	if (__bit_mem_rd(&thinkos_rt.th_alloc, th) == 0) {
		DCC_LOG1(LOG_ERROR, "invalid thread %d!", th);
		arg[0] = THINKOS_EINVAL;
		return;
	}
#endif
#endif

#if THINKOS_ENABLE_CANCEL
	/* remove thread from the canceled wait queue */
	if (__bit_mem_rd(&thinkos_rt.wq_canceled, th)) {
		bmp_bit_clr(&thinkos_rt.wq_canceled, th);  
		bmp_bit_set(&thinkos_rt.wq_ready, th);  
	}
#endif /* THINKOS_ENABLE_CANCEL */

	/* insert the current thread (self) into the joining thread wait queue */
	wq = __wq_idx(&thinkos_rt.wq_join[th]);
	__thinkos_wq_insert(wq, self);

	/* wait for event */
	__thinkos_wait();

	/* set the return to ERROR as a default value. The
	   exit function of the joining thread will set this to the 
	   appropriate return code */
	arg[0] = -1;
}
#endif

#if THINKOS_ENABLE_CANCEL
void thinkos_cancel_svc(int32_t * arg)
{
	unsigned int th = arg[0];
	int code = arg[1];
	unsigned int wq;
	int stat;

#if THINKOS_ENABLE_ARG_CHECK
	if (th >= THINKOS_THREADS_MAX) {
		DCC_LOG1(LOG_ERROR, "invalid thread %d!", th);
		arg[0] = THINKOS_EINVAL;
		return;
	}
#if THINKOS_ENABLE_THREAD_ALLOC
	if (__bit_mem_rd(&thinkos_rt.th_alloc, th) == 0) {
		arg[0] = THINKOS_EINVAL;
		return;
	}
#endif
#endif

#if (THINKOS_ENABLE_THREAD_STAT == 0)
#error "thinkos_cancel() depends on THINKOS_ENABLE_THREAD_STAT"	
#endif
	stat = thinkos_rt.th_stat[th];
	/* remove from other wait queue including wq_ready */
	bmp_bit_clr(&thinkos_rt.wq_lst[stat >> 1], th);  

#if THINKOS_ENABLE_JOIN
	/* insert into the canceled wait queue and wait for a join call */ 
	wq = __wq_idx(&thinkos_rt.wq_canceled);
#else /* THINKOS_ENABLE_JOIN */
	/* if join is not enabled insert into the ready queue */
	wq = __wq_idx(&thinkos_rt.wq_ready);
#endif /* THINKOS_ENABLE_JOIN */

	__thinkos_wq_insert(wq, th);

#if THINKOS_ENABLE_TIMESHARE
	/* possibly remove from the time share wait queue */
	bmp_bit_clr(&thinkos_rt.wq_tmshare, th);  
#endif

#if THINKOS_ENABLE_CLOCK
	/* possibly remove from the time wait queue */
	bmp_bit_clr(&thinkos_rt.wq_clock, th);  
#endif

	DCC_LOG3(LOG_TRACE, "<%d> cancel %d, with code %d!", 
			 thinkos_rt.active, th, code); 

	thinkos_rt.ctx[th]->pc = (uint32_t)thinkos_thread_exit;
	thinkos_rt.ctx[th]->r0 = code;
	arg[0] = 0;
}
#endif


#if THINKOS_ENABLE_EXIT
void thinkos_exit_svc(int32_t * arg)
{
	int self = thinkos_rt.active;
	int code = arg[0];
	unsigned int wq;

#if THINKOS_ENABLE_JOIN
	/* insert into the canceled wait queue and wait for a join call */ 
	wq = __wq_idx(&thinkos_rt.wq_canceled);
#else /* THINKOS_ENABLE_JOIN */
	/* if join is not enabled insert into the ready queue */
	wq = __wq_idx(&thinkos_rt.wq_ready);
#endif /* THINKOS_ENABLE_JOIN */

	__thinkos_wq_insert(wq, self);

	DCC_LOG2(LOG_TRACE, "<%d> exit with code %d!", self, code); 

	/* adjust PC */
	arg[6] = (uint32_t)thinkos_thread_exit;
	/* set the return code at R0 */
	arg[0] = code;

	__thinkos_defer_sched();
}
#endif


#if THINKOS_ENABLE_PAUSE
void thinkos_resume_svc(int32_t * arg)
{
	unsigned int th = arg[0];
	unsigned int wq;
	int stat;

#if THINKOS_ENABLE_ARG_CHECK
	if (th >= THINKOS_THREADS_MAX) {
		DCC_LOG1(LOG_ERROR, "invalid thread %d!", th);
		arg[0] = THINKOS_EINVAL;
		return;
	}
#if THINKOS_ENABLE_THREAD_ALLOC
	if (__bit_mem_rd(&thinkos_rt.th_alloc, th) == 0) {
		arg[0] = THINKOS_EINVAL;
		return;
	}
#endif
#endif

	DCC_LOG1(LOG_TRACE, "thread=%d", th);

	arg[0] = 0;

	if (__bit_mem_rd(&thinkos_rt.wq_paused, th) == 0) {
		/* not paused */
		return;
	}

#if (THINKOS_ENABLE_THREAD_STAT == 0)
#error "thinkos_resume() depends on THINKOS_ENABLE_THREAD_STAT"	
#endif
	/* remove from the paused queue */
	bmp_bit_clr(&thinkos_rt.wq_paused, th);  
	/* reinsert the thread into a waiting queue, including ready  */
	stat = thinkos_rt.th_stat[th];
	wq = stat >> 1;
	DCC_LOG2(LOG_TRACE, "stat=0x%02x wq=%d", stat, wq);
	__bit_mem_wr(&thinkos_rt.wq_lst[wq], th, 1);

#if THINKOS_ENABLE_CLOCK
	/* reenable the clock according to the thread status */
	__bit_mem_wr(&thinkos_rt.wq_clock, th, 1);
#endif
	__thinkos_defer_sched();
}

void thinkos_pause_svc(int32_t * arg)
{
	unsigned int th = arg[0];
	unsigned int wq;
	int stat;

#if THINKOS_ENABLE_ARG_CHECK
	if (th >= THINKOS_THREADS_MAX) {
		DCC_LOG1(LOG_ERROR, "invalid thread %d!", th);
		arg[0] = THINKOS_EINVAL;
		return;
	}
#if THINKOS_ENABLE_THREAD_ALLOC
	if (__bit_mem_rd(&thinkos_rt.th_alloc, th) == 0) {
		arg[0] = THINKOS_EINVAL;
		return;
	}
#endif
#endif

#if (THINKOS_ENABLE_THREAD_STAT == 0)
#error "thinkos_pause() depends on THINKOS_ENABLE_THREAD_STAT"	
#endif

	DCC_LOG1(LOG_TRACE, "thread=%d", th);

	arg[0] = 0;

	if (__bit_mem_rd(&thinkos_rt.wq_paused, th) != 0) {
		/* paused */
		return;
	}

	/* insert into the paused queue */
	__bit_mem_wr(&thinkos_rt.wq_paused, th, 1);

	/* remove the thread from a waiting queue, including ready  */
	stat = thinkos_rt.th_stat[th];
	wq = stat >> 1;
	DCC_LOG2(LOG_TRACE, "stat=0x%02x wq=%d", stat, wq);
	__bit_mem_wr(&thinkos_rt.wq_lst[wq], th, 0);

#if THINKOS_ENABLE_TIMESHARE
	/* possibly remove from the time share wait queue */
	__bit_mem_wr(&thinkos_rt.wq_tmshare, th, 0);
#endif

#if THINKOS_ENABLE_CLOCK
	/* disable the clock */
	__bit_mem_wr(&thinkos_rt.wq_clock, th, 0);
#endif

	__thinkos_defer_sched();
}
#endif /* THINKOS_ENABLE_PAUSE */

/* initialize a thread context */
void thinkos_thread_create_svc(int32_t * arg)
{
	struct thinkos_thread_init * init = (struct thinkos_thread_init *)arg;
	struct thinkos_context * ctx;
	uint32_t sp;
	int th;

#if THINKOS_ENABLE_THREAD_ALLOC
	if (init->opt.id >= THINKOS_THREADS_MAX) {
		th = thinkos_alloc_hi(&thinkos_rt.th_alloc, THINKOS_THREADS_MAX);
		DCC_LOG2(LOG_INFO, "thinkos_alloc_hi() %d -> %d.", init->opt.id, th);
		DCC_LOG1(LOG_INFO, "thinkos_rt.th_alloc=0x%08x", thinkos_rt.th_alloc);
	} else {
		/* Look for the next available slot */
		th = thinkos_alloc_lo(&thinkos_rt.th_alloc, init->opt.id);
		DCC_LOG2(LOG_INFO, "thinkos_alloc_lo() %d -> %d.", init->opt.id, th);
		if (th < 0) {
			th = thinkos_alloc_hi(&thinkos_rt.th_alloc, init->opt.id);
			DCC_LOG2(LOG_INFO, "thinkos_alloc_hi() %d -> %d.", 
					init->opt.id, th);
		}
	}

	if (th < 0) {
		arg[0] = THINKOS_EINVAL;
		return;
	}
#else
	th = init->opt.id;
	if (th >= THINKOS_THREADS_MAX)
		th = THINKOS_THREADS_MAX - 1;
#endif

	sp = (uint32_t)init->stack_ptr + init->opt.stack_size;

	if (init->opt.stack_size < sizeof(struct thinkos_context)) {
		DCC_LOG1(LOG_ERROR, "stack too small. size=%d", init->opt.stack_size);
		arg[0] = th;
		return;
	}

	DCC_LOG2(LOG_INFO, "stack ptr=%08x size=%d", 
			 init->stack_ptr, init->opt.stack_size);

	sp &= 0xfffffff8; /* 64bits alignemnt */
	sp -= sizeof(struct thinkos_context);

	DCC_LOG1(LOG_INFO, "sp=%08x", sp);

	/* initialize stack */
	{
		uint32_t * ptr;
		for (ptr = init->stack_ptr; ptr < (uint32_t *)sp; ++ptr)
			*ptr = 0xdeadbeef;
	}

	ctx = (struct thinkos_context *)sp;

//	ctx->ret = CM3_EXC_RET_THREAD_PSP;
	ctx->r0 = (uint32_t)init->arg;
	ctx->r1 = 0;
	ctx->r2 = 0;
	ctx->r3 = 0;
	ctx->r4 = 0;
	ctx->r5 = 0;
	ctx->r6 = 0;
	ctx->r7 = 0;
	ctx->r8 = 0;
	ctx->r9 = 0;
	ctx->r10 = 0;
	ctx->r11 = 0;
	ctx->r12 = 0;
	ctx->lr = (uint32_t)thinkos_thread_exit;
	ctx->pc = (uint32_t)init->task;
	ctx->xpsr = 0x01000000;

	thinkos_rt.ctx[th] = ctx;

#if THINKOS_ENABLE_THREAD_INFO
	thinkos_rt.th_inf[th] = init->inf;
#endif

#if THINKOS_ENABLE_TIMESHARE
	thinkos_rt.sched_pri[th] = init->opt.priority;
	if (thinkos_rt.sched_pri[th] > THINKOS_SCHED_LIMIT_MAX)
		thinkos_rt.sched_pri[th] = THINKOS_SCHED_LIMIT_MAX;

	/* update schedule limit */
	if (thinkos_rt.sched_limit < thinkos_rt.sched_pri[th]) {
		thinkos_rt.sched_limit = thinkos_rt.sched_pri[th];
	}
	thinkos_rt.sched_val[th] = thinkos_rt.sched_limit / 2;
#endif

#if THINKOS_ENABLE_PAUSE
	if (init->opt.f_paused) 
		/* insert into the paused list */
		bmp_bit_set(&thinkos_rt.wq_paused, th);  
	else
#endif
	{
		/* insert into the ready list */
		bmp_bit_set(&thinkos_rt.wq_ready, th);  
		__thinkos_defer_sched();
	}

#if THINKOS_ENABLE_TIMESHARE
	DCC_LOG5(LOG_TRACE, "<%d> pri=%d/%d task=%08x sp=%08x", 
			 th, thinkos_rt.sched_pri[th], 
			 thinkos_rt.sched_limit, init->task, ctx);
#endif

	arg[0] = th;
}

void thinkos_sleep_svc(int32_t * arg)
{
	uint32_t ms = (uint32_t)arg[0];
#if THINKOS_ENABLE_CLOCK
	int self = thinkos_rt.active;

	/* set the clock */
	thinkos_rt.clock[self] = thinkos_rt.ticks + (ms / 1);
	/* insert into the clock wait queue */
	bmp_bit_set(&thinkos_rt.wq_clock, self);  

#if THINKOS_ENABLE_THREAD_STAT
	/* mark the thread clock enable bit */
	thinkos_rt.th_stat[self] = (THINKOS_WQ_CLOCK << 1) + 1;
#endif

	DCC_LOG2(LOG_MSG, "<%d> waiting %d milliseconds...", self, ms);

	/* wait for event */
	__thinkos_wait();
#else
	DCC_LOG1(LOG_TRACE, "busy wait: %d milliseconds...", ms);
	udelay(1000 * ms);
#endif
	
}

