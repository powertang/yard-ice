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

const struct {
	uint8_t wq[0]; /* placeholder */
	uint8_t wq_ready; /* ready threads queue */
#if THINKOS_ENABLE_TIMESHARE
	uint8_t wq_tmshare; /* Threads waiting for time share cycle */
#endif
#if THINKOS_ENABLE_JOIN
	uint8_t wq_canceled; /* canceled threads wait queue */
#endif
#if THINKOS_ENABLE_PAUSE
	uint8_t wq_paused;
#endif
#if THINKOS_ENABLE_CLOCK
	uint8_t wq_clock;
#endif
#if THINKOS_MUTEX_MAX > 0
	uint8_t wq_mutex[THINKOS_MUTEX_MAX];
#endif
#if THINKOS_COND_MAX > 0
	uint8_t wq_cond[THINKOS_COND_MAX];
#endif
#if THINKOS_SEMAPHORE_MAX > 0
	uint8_t wq_sem[THINKOS_SEMAPHORE_MAX];
#endif
#if THINKOS_EVENT_MAX > 0
	uint8_t wq_event[THINKOS_EVENT_MAX];
#endif
#if THINKOS_FLAG_MAX > 0
	uint8_t wq_flag[THINKOS_FLAG_MAX];
#endif
#if THINKOS_ENABLE_JOIN
	uint8_t wq_join[THINKOS_THREADS_MAX];
#endif
} thinkos_obj_type_lut = {
	.wq_ready = THINKOS_OBJ_READY,
#if THINKOS_ENABLE_TIMESHARE
	.wq_tmshare = THINKOS_OBJ_TMSHARE,
#endif
#if THINKOS_ENABLE_JOIN
	.wq_canceled = THINKOS_OBJ_CANCELED,
#endif
#if THINKOS_ENABLE_PAUSE
	.wq_paused = THINKOS_OBJ_PAUSED,
#endif
#if THINKOS_ENABLE_CLOCK
	.wq_clock = THINKOS_OBJ_CLOCK,
#endif
#if THINKOS_MUTEX_MAX > 0
	.wq_mutex = { [0 ... (THINKOS_MUTEX_MAX - 1)] = THINKOS_OBJ_MUTEX },
#endif 
#if THINKOS_COND_MAX > 0
	.wq_cond = { [0 ... (THINKOS_COND_MAX - 1)] = THINKOS_OBJ_COND },
#endif
#if THINKOS_SEMAPHORE_MAX > 0
	.wq_sem = { [0 ... (THINKOS_SEMAPHORE_MAX - 1)] = THINKOS_OBJ_SEMAPHORE },
#endif
#if THINKOS_EVENT_MAX > 0
	.wq_event = { [0 ... (THINKOS_EVENT_MAX - 1)] = THINKOS_OBJ_EVENT },
#endif
#if THINKOS_FLAG_MAX > 0
	.wq_flag = { [0 ... (THINKOS_FLAG_MAX - 1)] = THINKOS_OBJ_FLAG },
#endif
#if THINKOS_ENABLE_JOIN
	.wq_join = { [0 ... (THINKOS_THREADS_MAX - 1)] = THINKOS_OBJ_JOIN }
#endif
};

uint32_t * const thinkos_obj_alloc_lut[] = {
	[THINKOS_OBJ_READY] = NULL,
	[THINKOS_OBJ_TMSHARE] = NULL,
	[THINKOS_OBJ_CANCELED] = NULL,
	[THINKOS_OBJ_PAUSED] = NULL,
	[THINKOS_OBJ_CLOCK] = NULL,
#if THINKOS_ENABLE_MUTEX_ALLOC
	[THINKOS_OBJ_MUTEX] = NULL,
#else
#endif
#if THINKOS_ENABLE_COND_ALLOC
	[THINKOS_OBJ_COND] = thinkos_rt.cond_alloc,
#else
	[THINKOS_OBJ_COND] = NULL
#endif
#if THINKOS_ENABLE_SEM_ALLOC
	[THINKOS_OBJ_SEMAPHORE] = thinkos_rt.sem_alloc,
#else
	[THINKOS_OBJ_SEMAPHORE] = NULL,
#endif
#if THINKOS_ENABLE_EVENT_ALLOC
	[THINKOS_OBJ_EVENT] = thinkos_rt.ev_alloc,
#else
	[THINKOS_OBJ_EVENT] = NULL,
#endif
#if THINKOS_ENABLE_FLAG_ALLOC
	[THINKOS_OBJ_FLAG] = thinkos_rt.flag_alloc,
#else
	[THINKOS_OBJ_FLAG] = NULL,
#endif
#if THINKOS_ENABLE_THREAD_ALLOC
	[THINKOS_OBJ_JOIN] = thinkos_rt.th_alloc,
#else
	[THINKOS_OBJ_JOIN] = NULL,
#endif
	[THINKOS_OBJ_INVALID] = NULL
};

const uint16_t thinkos_wq_base_lut[] = {
	[THINKOS_OBJ_READY] = THINKOS_WQ_READY,
#if THINKOS_ENABLE_TIMESHARE
	[THINKOS_OBJ_TMSHARE] = THINKOS_WQ_TMSHARE,
#endif
#if THINKOS_ENABLE_JOIN
	[THINKOS_OBJ_CANCELED] = THINKOS_WQ_CANCELED,
#endif
#if THINKOS_ENABLE_PAUSE
	[THINKOS_OBJ_PAUSED] = THINKOS_WQ_PAUSED,
#endif
#if THINKOS_ENABLE_CLOCK
	[THINKOS_OBJ_CLOCK] = THINKOS_WQ_CLOCK,
#endif
#if THINKOS_MUTEX_MAX > 0
	[THINKOS_OBJ_MUTEX] = THINKOS_MUTEX_BASE,
#endif
#if THINKOS_COND_MAX > 0
	[THINKOS_OBJ_COND] = THINKOS_COND_BASE,
#endif
#if THINKOS_SEMAPHORE_MAX > 0
	[THINKOS_OBJ_SEMAPHORE] = THINKOS_SEM_BASE,
#endif
#if THINKOS_EVENT_MAX > 0
	[THINKOS_OBJ_EVENT] = THINKOS_EVENT_BASE,
#endif
#if THINKOS_FLAG_MAX > 0
	[THINKOS_OBJ_FLAG] = THINKOS_FLAG_BASE,
#endif
#if THINKOS_ENABLE_JOIN
	[THINKOS_OBJ_JOIN] = THINKOS_JOIN_BASE,
#endif
	[THINKOS_OBJ_INVALID] = 0 
};

