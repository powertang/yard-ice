/* 
 * thikos_svc.c
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
#include <thinkos_svc.h>

void thinkos_thread_create_svc(int32_t * arg);

void thinkos_pause_svc(int32_t * arg);

void thinkos_resume_svc(int32_t * arg);

void thinkos_join_svc(int32_t * arg);

void thinkos_cancel_svc(int32_t * arg);

void thinkos_exit_svc(int32_t * arg);

void thinkos_sleep_svc(int32_t * arg);

void thinkos_alarm_svc(int32_t * arg);

void thinkos_mutex_alloc_svc(int32_t * arg);

void thinkos_mutex_free_svc(int32_t * arg);

void thinkos_mutex_lock_svc(int32_t * arg);

void thinkos_mutex_trylock_svc(int32_t * arg);

void thinkos_mutex_timedlock_svc(int32_t * arg);

void thinkos_mutex_unlock_svc(int32_t * arg);


void thinkos_cond_alloc_svc(int32_t * arg);

void thinkos_cond_free_svc(int32_t * arg);

void thinkos_cond_wait_svc(int32_t * arg);

void thinkos_cond_timedwait_svc(int32_t * arg);

void thinkos_cond_signal_svc(int32_t * arg);

void thinkos_cond_broadcast_svc(int32_t * arg);


void thinkos_sem_alloc_svc(int32_t * arg);

void thinkos_sem_free_svc(int32_t * arg);

void thinkos_sem_init_svc(int32_t * arg);

void thinkos_sem_wait_svc(int32_t * arg);

void thinkos_sem_trywait_svc(int32_t * arg);

void thinkos_sem_timedwait_svc(int32_t * arg);

void thinkos_sem_post_svc(int32_t * arg);


void thinkos_ev_alloc_svc(int32_t * arg);

void thinkos_ev_free_svc(int32_t * arg);

void thinkos_ev_wait_svc(int32_t * arg);

void thinkos_ev_timedwait_svc(int32_t * arg);

void thinkos_ev_raise_svc(int32_t * arg);

void thinkos_ev_mask_svc(int32_t * arg);

void thinkos_ev_unmask_svc(int32_t * arg);


void thinkos_flag_alloc_svc(int32_t * arg);

void thinkos_flag_free_svc(int32_t * arg);

void thinkos_flag_val_svc(int32_t * arg);

void thinkos_flag_set_svc(int32_t * arg);

void thinkos_flag_clr_svc(int32_t * arg);

void thinkos_flag_wait_svc(int32_t * arg);

void thinkos_flag_timedwait_svc(int32_t * arg);

void thinkos_flag_give_svc(int32_t * arg);

void thinkos_flag_take_svc(int32_t * arg);

void thinkos_flag_timedtake_svc(int32_t * arg);

void thinkos_flag_release_svc(int32_t * arg);

void thinkos_irq_wait_svc(int32_t * arg);

void thinkos_rt_snapshot_svc(int32_t * arg);

static inline uint32_t __attribute__((always_inline)) cm3_svc_stackframe(void) {
	register uint32_t sp;
	asm volatile ("tst lr, #4\n" 
				  "ite eq\n" 
				  "mrseq %0, MSP\n" 
				  "mrsne %0, PSP\n" 
				  : "=r" (sp));
	return sp;
}

void cm3_svc_isr(void)
{
	int32_t * arg;
	uint8_t * pc;
	int svc;

	/* get a pointer to the caller's stack */
#if THINKOS_ENABLE_IRQ_SVC_CALL
	arg = (int32_t * )cm3_svc_stackframe();
#else
	arg = (int32_t * )cm3_psp_get();
#endif

	/* get PC value */
	pc = (uint8_t *)arg[6];
	/* get the immediate data from instruction */
	svc = pc[-2];

	DCC_LOG1(LOG_MSG, "pc=0x%08x", pc);

	switch (svc) {
	case THINKOS_THREAD_SELF:
		arg[0] = thinkos_rt.active;
		break;

	case THINKOS_THREAD_CREATE:
		thinkos_thread_create_svc(arg);
		break;

	case THINKOS_YIELD:
		__thinkos_defer_sched();
		break;

#if THINKOS_ENABLE_PAUSE
	case THINKOS_PAUSE:
		thinkos_pause_svc(arg);
		break;

	case THINKOS_RESUME:
		thinkos_resume_svc(arg);
		break;
#endif

#if THINKOS_ENABLE_CANCEL
	case THINKOS_CANCEL:
		thinkos_cancel_svc(arg);
		break;
#endif

#if THINKOS_ENABLE_JOIN
	case THINKOS_JOIN:
		thinkos_join_svc(arg);
		break;
#endif

#if THINKOS_ENABLE_SLEEP
	case THINKOS_SLEEP:
		thinkos_sleep_svc(arg);
		break;
#endif

#if THINKOS_ENABLE_ALARM
	case THINKOS_ALARM:
		thinkos_alarm_svc(arg);
		break;
#endif
		
#if THINKOS_ENABLE_CLOCK
	case THINKOS_CLOCK:
		arg[0] = thinkos_rt.ticks;
		break;
#endif

#if THINKOS_MUTEX_MAX > 0
#if THINKOS_ENABLE_MUTEX_ALLOC
	case THINKOS_MUTEX_ALLOC:
		thinkos_mutex_alloc_svc(arg);
		break;

	case THINKOS_MUTEX_FREE:
		thinkos_mutex_free_svc(arg);
		break;
#endif /* THINKOS_MUTEX_ALLOC */

	case THINKOS_MUTEX_LOCK:
		thinkos_mutex_lock_svc(arg);
		break;

	case THINKOS_MUTEX_TRYLOCK:
		thinkos_mutex_trylock_svc(arg);
		break;

#if THINKOS_ENABLE_TIMED_CALLS
	case THINKOS_MUTEX_TIMEDLOCK:
		thinkos_mutex_timedlock_svc(arg);
		break;
#endif

	case THINKOS_MUTEX_UNLOCK:
		thinkos_mutex_unlock_svc(arg);
		break;
#endif /* THINKOS_MUTEX_MAX > 0 */


#if THINKOS_COND_MAX > 0
#if THINKOS_ENABLE_COND_ALLOC
	case THINKOS_COND_ALLOC:
		thinkos_cond_alloc_svc(arg);
		break;

	case THINKOS_COND_FREE:
		thinkos_cond_free_svc(arg);
		break;
#endif /* THINKOS_COND_ALLOC */

	case THINKOS_COND_WAIT:
		thinkos_cond_wait_svc(arg);
		break;

#if THINKOS_ENABLE_TIMED_CALLS
	case THINKOS_COND_TIMEDWAIT:
		thinkos_cond_timedwait_svc(arg);
		break;
#endif

	case THINKOS_COND_SIGNAL:
		thinkos_cond_signal_svc(arg);
		break;

	case THINKOS_COND_BROADCAST:
		thinkos_cond_broadcast_svc(arg);
		break;
#endif /* THINKOS_COND_MAX > 0 */


#if THINKOS_SEMAPHORE_MAX > 0
#if THINKOS_ENABLE_SEM_ALLOC
	case THINKOS_SEM_ALLOC:
		thinkos_sem_alloc_svc(arg);
		break;

	case THINKOS_SEM_FREE:
		thinkos_sem_free_svc(arg);
		break;
#endif /* THINKOS_ENABLE_SEM_ALLOC */

	case THINKOS_SEM_INIT:
		thinkos_sem_init_svc(arg);
		break;

	case THINKOS_SEM_WAIT:
		thinkos_sem_wait_svc(arg);
		break;

	case THINKOS_SEM_TRYWAIT:
		thinkos_sem_trywait_svc(arg);
		break;

#if THINKOS_ENABLE_TIMED_CALLS
	case THINKOS_SEM_TIMEDWAIT:
		thinkos_sem_timedwait_svc(arg);
		break;
#endif

	case THINKOS_SEM_POST:
		thinkos_sem_post_svc(arg);
		break;
#endif /* THINKOS_SEMAPHORE_MAX > 0 */


#if (THINKOS_EVENT_MAX > 0)
#if THINKOS_ENABLE_EVENT_ALLOC
	case THINKOS_EVENT_ALLOC:
		thinkos_ev_alloc_svc(arg);
		break;

	case THINKOS_EVENT_FREE:
		thinkos_ev_free_svc(arg);
		break;
#endif

	case THINKOS_EVENT_WAIT:
		thinkos_ev_wait_svc(arg);
		break;

#if THINKOS_ENABLE_TIMED_CALLS
	case THINKOS_EVENT_TIMEDWAIT:
		thinkos_ev_timedwait_svc(arg);
		break;
#endif

	case THINKOS_EVENT_RAISE:
		thinkos_ev_raise_svc(arg);
		break;

	case THINKOS_EVENT_MASK:
		thinkos_ev_mask_svc(arg);
		break;

	case THINKOS_EVENT_UNMASK:
		thinkos_ev_unmask_svc(arg);
		break;

#endif /* (THINKOS_EVENT_MAX > 0) */


#if (THINKOS_FLAG_MAX > 0)
#if THINKOS_ENABLE_FLAG_ALLOC
	case THINKOS_FLAG_ALLOC:
		thinkos_flag_alloc_svc(arg);
		break;

	case THINKOS_FLAG_FREE:
		thinkos_flag_free_svc(arg);
		break;
#endif

	case THINKOS_FLAG_CLR:
		thinkos_flag_clr_svc(arg);
		break;

	case THINKOS_FLAG_SET:
		thinkos_flag_set_svc(arg);
		break;

	case THINKOS_FLAG_WAIT:
		thinkos_flag_wait_svc(arg);
		break;

#if THINKOS_ENABLE_TIMED_CALLS
	case THINKOS_FLAG_TIMEDWAIT:
		thinkos_flag_timedwait_svc(arg);
		break;
#endif

#if THINKOS_ENABLE_FLAG_LOCK
	case THINKOS_FLAG_RELEASE:
		thinkos_flag_release_svc(arg);
		break;
#endif

	case THINKOS_FLAG_TAKE:
		thinkos_flag_take_svc(arg);
		break;

#if THINKOS_ENABLE_TIMED_CALLS
	case THINKOS_FLAG_TIMEDTAKE:
		thinkos_flag_timedtake_svc(arg);
		break;
#endif

	case THINKOS_FLAG_GIVE:
		thinkos_flag_give_svc(arg);
		break;
#endif /* (THINKOS_FLAG_MAX > 0) */

#if THINKOS_IRQ_MAX > 0
	case THINKOS_IRQ_WAIT:
		thinkos_irq_wait_svc(arg);
		break;
#endif /* THINKOS_IRQ_MAX > 0 */

#if THINKOS_ENABLE_EXIT
	case THINKOS_EXIT:
		thinkos_exit_svc(arg);
		break;
#endif

#if THINKOS_ENABLE_RT_DEBUG
	case THINKOS_RT_SNAPSHOT:
		thinkos_rt_snapshot_svc(arg);
		break;
#endif

	default:
		DCC_LOG1(LOG_WARNING, "Invalid System call: %d", svc);
		arg[0] = THINKOS_ENOSYS;
		break;
	}
}

/* FIXME: this is a hack to force linking this file. 
 The linker then will override the weak alias for the cm3_svc_isr() */
const char thinkos_svc_nm[] = "SVC";

