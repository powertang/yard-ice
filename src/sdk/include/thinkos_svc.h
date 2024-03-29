/* 
 * thikos_svc.h
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You can receive a copy of the GNU Lesser General Public License from 
 * http://www.gnu.org/
 */


#ifndef __THINKOS_SVC_H__
#define __THINKOS_SVC_H__

/* -------------------------------------------------------------------------- 
 * Service numbers
 * --------------------------------------------------------------------------*/

#define THINKOS_THREAD_SELF      0
#define THINKOS_THREAD_CREATE    1
   
#define THINKOS_CLOCK            2
#define THINKOS_ALARM            3
#define THINKOS_SLEEP            4

#define THINKOS_MUTEX_LOCK       5
#define THINKOS_MUTEX_TRYLOCK    6
#define THINKOS_MUTEX_TIMEDLOCK  7
#define THINKOS_MUTEX_UNLOCK     8

#define THINKOS_SEM_INIT         9
#define THINKOS_SEM_WAIT        10
#define THINKOS_SEM_TRYWAIT     11
#define THINKOS_SEM_TIMEDWAIT   12
#define THINKOS_SEM_POST        13

#define THINKOS_COND_WAIT       14
#define THINKOS_COND_TIMEDWAIT  15
#define THINKOS_COND_SIGNAL     16
#define THINKOS_COND_BROADCAST  17

#define THINKOS_FLAG_CLR        18
#define THINKOS_FLAG_SET        19
#define THINKOS_FLAG_WAIT       20
#define THINKOS_FLAG_TIMEDWAIT  21
#define THINKOS_FLAG_RELEASE    22
#define THINKOS_FLAG_TAKE       23
#define THINKOS_FLAG_TIMEDTAKE  24
#define THINKOS_FLAG_GIVE       25

#define THINKOS_EVENT_WAIT      26
#define THINKOS_EVENT_TIMEDWAIT 27
#define THINKOS_EVENT_RAISE     28
#define THINKOS_EVENT_MASK      29
#define THINKOS_EVENT_UNMASK    30

#define THINKOS_IRQ_WAIT        31

#define THINKOS_MUTEX_ALLOC     32
#define THINKOS_MUTEX_FREE      33

#define THINKOS_SEM_ALLOC       34
#define THINKOS_SEM_FREE        35

#define THINKOS_COND_ALLOC      36
#define THINKOS_COND_FREE       37

#define THINKOS_FLAG_ALLOC      38
#define THINKOS_FLAG_FREE       39

#define THINKOS_EVENT_ALLOC     40
#define THINKOS_EVENT_FREE      41

#define THINKOS_YIELD           42
#define THINKOS_PAUSE           43
#define THINKOS_RESUME          44
#define THINKOS_CANCEL          45
#define THINKOS_JOIN            46

#define THINKOS_RT_SNAPSHOT     47
#define THINKOS_EXIT            48

#ifndef __ASSEMBLER__

/* -------------------------------------------------------------------------- 
 * C service call macros 
 * --------------------------------------------------------------------------*/

#define __SVC_CALL(N) ( { register int ret asm("r0"); \
	asm volatile ("svc " #N "\n" : "=r"(ret) : : ); \
	ret; } )

#define __SVC_CALL1(N, A1) ( { register int ret asm("r0"); \
	register int r0 asm("r0") = (int)A1; \
	asm volatile ("svc " #N "\n" : "=r"(ret) : "0"(r0) : ); \
	ret; } )

#define __SVC_CALL2(N, A1, A2) ( { register int ret asm("r0"); \
	register int r0 asm("r0") = (int)A1; \
	register int r1 asm("r1") = (int)A2; \
	asm volatile ("svc " #N "\n" : "=r"(ret) : \
				  "0"(r0), "r"(r1) : ); \
	ret; } )

#define __SVC_CALL3(N, A1, A2, A3) ( { register int ret asm("r0"); \
	register int r0 asm("r0") = (int)A1; \
	register int r1 asm("r1") = (int)A2; \
	register int r2 asm("r2") = (int)A3; \
	asm volatile ("svc " #N "\n" : "=r"(ret) : \
				  "0"(r0), "r"(r1), "r"(r2) : ); \
	ret; } )

#define __SVC_CALL4(N, A1, A2, A3, A4) ( { register int ret asm("r0"); \
	register int r0 asm("r0") = (int)A1; \
	register int r1 asm("r1") = (int)A2; \
	register int r2 asm("r2") = (int)A3; \
	register int r3 asm("r3") = (int)A4; \
	asm volatile ("svc " #N "\n" : "=r"(ret) : \
				  "0"(r0), "r"(r1), "r"(r2), "r"(r3) : ); \
	ret; } )

#define __SVC_CALL5(N, A1, A2, A3, A4, A5) ( { register int ret asm("r0"); \
	register int r0 asm("r0") = (int)A1; \
	register int r1 asm("r1") = (int)A2; \
	register int r2 asm("r2") = (int)A3; \
	register int r3 asm("r3") = (int)A4; \
	register int r12 asm("r12") = (int)A5; \
	asm volatile ("svc " #N "\n" : "=r"(ret) : \
				  "0"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r12) : ); \
	ret; } )


#define THINKOS_SVC(N) __SVC_CALL(N)

#define THINKOS_SVC1(N, A1) __SVC_CALL1(N, (A1))

#define THINKOS_SVC2(N, A1, A2) __SVC_CALL2(N, (A1), (A2))

#define THINKOS_SVC3(N, A1, A2, A3) __SVC_CALL3(N, (A1), (A2), (A3))

#define THINKOS_SVC4(N, A1, A2, A3, A4) __SVC_CALL4(N, (A1), (A2), (A3), (A4))

#define THINKOS_SVC5(N, A1, A2, A3, A4, A5) __SVC_CALL5(N, (A1), (A2), \
														(A3), (A4), (A5))

#ifdef __cplusplus
extern "C" {
#endif

static inline int const __attribute__((always_inline)) 
	thinkos_thread_self(void) {
	return THINKOS_SVC(THINKOS_THREAD_SELF);
}

static inline int __attribute__((always_inline)) 
	thinkos_thread_create(int (* task)(void *), 
						  void * arg, void * stack_ptr,
						  unsigned int opt) {
	return THINKOS_SVC5(THINKOS_THREAD_CREATE, task, arg, 
						stack_ptr, opt, 0);
}

static inline int __attribute__((always_inline)) 
	thinkos_thread_create_inf(int (* task)(void *), void * arg, 
							  const struct thinkos_thread_inf * inf) {
	return THINKOS_SVC5(THINKOS_THREAD_CREATE, task, arg, 
						inf->stack_ptr, inf->opt, inf);
}

static inline int 
__attribute__((always_inline)) thinkos_cancel(unsigned int id, int code) {
	return THINKOS_SVC2(THINKOS_CANCEL, id, code);
}

static inline int 
__attribute__((always_inline)) thinkos_exit(int code) {
	return THINKOS_SVC1(THINKOS_EXIT, code);
}

static inline int 
__attribute__((always_inline)) thinkos_join(unsigned int id) {
	return THINKOS_SVC1(THINKOS_JOIN, id);
}

static inline int 
__attribute__((always_inline)) thinkos_pause(unsigned int id) {
	return THINKOS_SVC1(THINKOS_PAUSE, id);
}

static inline int 
__attribute__((always_inline)) thinkos_resume(unsigned int id) {
	return THINKOS_SVC1(THINKOS_RESUME, id);
}

static inline int
__attribute__((always_inline)) thinkos_sleep(unsigned int ms) {
	return THINKOS_SVC1(THINKOS_SLEEP, ms);
}

static inline int
__attribute__((always_inline)) thinkos_alarm(uint32_t clk) {
	return THINKOS_SVC1(THINKOS_ALARM, clk);
}

static inline uint32_t __attribute__((always_inline)) thinkos_clock(void) {
	return THINKOS_SVC(THINKOS_CLOCK);
}

static inline int __attribute__((always_inline)) thinkos_mutex_alloc(void) {
	return THINKOS_SVC(THINKOS_MUTEX_ALLOC);
}

static inline int __attribute__((always_inline)) thinkos_mutex_free(int mutex) {
	return THINKOS_SVC1(THINKOS_MUTEX_FREE, mutex);
}

static inline int __attribute__((always_inline)) thinkos_mutex_lock(int mutex) {
	return THINKOS_SVC1(THINKOS_MUTEX_LOCK, mutex);
}

static inline int __attribute__((always_inline)) thinkos_mutex_trylock(int mutex) {
	return THINKOS_SVC1(THINKOS_MUTEX_TRYLOCK, mutex);
}

static inline int __attribute__((always_inline)) 
thinkos_mutex_timedlock(int mutex, unsigned int ms) {
	return THINKOS_SVC2(THINKOS_MUTEX_TIMEDLOCK, mutex, ms);
}

static inline int __attribute__((always_inline)) 
thinkos_mutex_unlock(int mutex) {
	return THINKOS_SVC1(THINKOS_MUTEX_UNLOCK, mutex);
}


static inline int __attribute__((always_inline)) thinkos_cond_alloc(void) {
	return THINKOS_SVC(THINKOS_COND_ALLOC);
}

static inline int __attribute__((always_inline)) thinkos_cond_free(int cond) {
	return THINKOS_SVC1(THINKOS_COND_FREE, cond);
}

static inline int __attribute__((always_inline)) 
thinkos_cond_wait(int cond, int mutex) {
	int ret;
	if ((ret = THINKOS_SVC2(THINKOS_COND_WAIT, 
							cond, mutex)) >= THINKOS_ETIMEDOUT )
		ret = THINKOS_SVC1(THINKOS_MUTEX_LOCK, mutex);
	return ret;
}

static inline int __attribute__((always_inline)) 
thinkos_cond_timedwait(int cond, int mutex, unsigned int ms) {
	int ret;
	if ((ret = THINKOS_SVC3(THINKOS_COND_TIMEDWAIT, 
							cond, mutex, ms)) >= THINKOS_ETIMEDOUT) 
		ret = THINKOS_SVC1(THINKOS_MUTEX_LOCK, mutex);
	return ret;
}

static inline int __attribute__((always_inline)) 
thinkos_cond_signal(int cond) {
	return THINKOS_SVC1(THINKOS_COND_SIGNAL, cond);
}

static inline int __attribute__((always_inline)) 
thinkos_cond_broadcast(int cond) {
	return THINKOS_SVC1(THINKOS_COND_BROADCAST, cond);
}

static inline int __attribute__((always_inline)) 
thinkos_sem_alloc(unsigned int value) {
	return THINKOS_SVC1(THINKOS_SEM_ALLOC, value);
}

static inline int __attribute__((always_inline)) thinkos_sem_free(int sem) {
	return THINKOS_SVC1(THINKOS_SEM_FREE, sem);
}

static inline int __attribute__((always_inline)) 
	thinkos_sem_init(int sem, unsigned int value) {
	return THINKOS_SVC2(THINKOS_SEM_INIT, sem, value);
}

static inline int __attribute__((always_inline)) thinkos_sem_wait(int sem) {
	return THINKOS_SVC1(THINKOS_SEM_WAIT, sem);
}

static inline int __attribute__((always_inline)) thinkos_sem_trywait(int sem) {
	return THINKOS_SVC1(THINKOS_SEM_TRYWAIT, sem);
}

static inline int __attribute__((always_inline))
thinkos_sem_timedwait(int sem, unsigned int ms) {
	return THINKOS_SVC2(THINKOS_SEM_TIMEDWAIT, sem, ms);
}

static inline int __attribute__((always_inline)) thinkos_sem_post(int sem) {
	return THINKOS_SVC1(THINKOS_SEM_POST, sem);
}

/* ---------------------------------------------------------------------------
   Event sets
  ----------------------------------------------------------------------------*/

static inline int __attribute__((always_inline)) thinkos_ev_alloc(void) {
	return THINKOS_SVC(THINKOS_EVENT_ALLOC);
}

static inline int __attribute__((always_inline)) thinkos_ev_free(int set) {
	return THINKOS_SVC1(THINKOS_EVENT_FREE, set);
}

static inline int __attribute__((always_inline)) thinkos_ev_wait(int set) {
	return THINKOS_SVC1(THINKOS_EVENT_WAIT, set);
}

static inline int __attribute__((always_inline)) thinkos_ev_timedwait(
	int set, unsigned int ms) {
	return THINKOS_SVC2(THINKOS_EVENT_TIMEDWAIT, set, ms);
}

static inline int __attribute__((always_inline)) thinkos_ev_raise(
	int set, int ev) {
	return THINKOS_SVC2(THINKOS_EVENT_RAISE, set, ev);
}

static inline int __attribute__((always_inline)) thinkos_ev_mask(
	int set, uint32_t msk) {
	return THINKOS_SVC2(THINKOS_EVENT_MASK, set, msk);
}

static inline int __attribute__((always_inline)) thinkos_ev_unmask(
	int set, uint32_t msk) {
	return THINKOS_SVC2(THINKOS_EVENT_UNMASK, set, msk);
}

/* ---------------------------------------------------------------------------
   Flags
  ----------------------------------------------------------------------------*/

static inline int __attribute__((always_inline)) thinkos_flag_alloc(void) {
	return THINKOS_SVC(THINKOS_FLAG_ALLOC);
}

static inline int __attribute__((always_inline)) thinkos_flag_free(int flag) {
	return THINKOS_SVC1(THINKOS_FLAG_FREE, flag);
}

static inline int __attribute__((always_inline)) thinkos_flag_set(int flag) {
	return THINKOS_SVC1(THINKOS_FLAG_SET, flag);
}

static inline int __attribute__((always_inline)) thinkos_flag_clr(int flag) {
	return THINKOS_SVC1(THINKOS_FLAG_CLR, flag);
}

static inline int __attribute__((always_inline)) thinkos_flag_wait(int flag) {
	return THINKOS_SVC1(THINKOS_FLAG_WAIT, flag);
}

static inline int __attribute__((always_inline)) 
	thinkos_flag_timedwait(int flag, unsigned int ms) {
	return THINKOS_SVC2(THINKOS_FLAG_TIMEDWAIT, flag, ms);
}

static inline int __attribute__((always_inline)) thinkos_flag_give(int flag) {
	return THINKOS_SVC1(THINKOS_FLAG_GIVE, flag);
}

static inline int __attribute__((always_inline)) thinkos_flag_take(int flag) {
	return THINKOS_SVC1(THINKOS_FLAG_TAKE, flag);
}

static inline int __attribute__((always_inline)) 
	thinkos_flag_timedtake(int flag, unsigned int ms) {
	return THINKOS_SVC2(THINKOS_FLAG_TIMEDTAKE, flag, ms);
}

static inline int __attribute__((always_inline)) 
	thinkos_flag_signal(int flag) {
	return THINKOS_SVC1(THINKOS_FLAG_GIVE, flag);
}

static inline int __attribute__((always_inline)) 
	thinkos_flag_release( int flag, int sig) {
	return THINKOS_SVC2(THINKOS_FLAG_RELEASE, flag, sig);
}

static inline int __attribute__((always_inline)) thinkos_irq_wait(int irq) {
	return THINKOS_SVC1(THINKOS_IRQ_WAIT, irq);
}

static inline int __attribute__((always_inline)) thinkos_rt_snapshot(void * rt) {
	return THINKOS_SVC1(THINKOS_RT_SNAPSHOT, rt);
}

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLER__ */

#endif /* __THINKOS_SVC_H__ */

