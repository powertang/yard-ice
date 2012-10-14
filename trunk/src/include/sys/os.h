/* 
 * File:	sys/os.h
 * Module:	
 * Project:	
 * Author:	Robinson Mittmann (bob@boreste.com, bob@methafora.com.br)
 * Target:	
 * Comment:
 * Copyright(c) 2008 BORESTE (www.boreste.com). All Rights Reserved.
 *
 */

#ifndef __SYS_OS_H__
#define __SYS_OS_H__

#include <thinkos.h>
#include <errno.h>

#define ENABLE_THINKOS 1

#define __OS_PRIORITY_HIGHEST 0
#define __OS_PRIORITY_HIGH    1
#define __OS_PRIORITY_MEDIUM  2
#define __OS_PRIORITY_LOW     4
#define __OS_PRIORITY_LOWEST  8

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t __os_mutex_t;
typedef int8_t __os_cond_t;
typedef int8_t __os_sem_t;

static inline void __os_sleep(unsigned int msec) {
#if ENABLE_THINKOS
	thinkos_sleep(msec);
#else
	mdelay(msec);
#endif
}

static inline void exit(int status) {
#if ENABLE_THINKOS
	thinkos_cancel(thinkos_thread_self(), status);
	thinkos_yield();
#else
	for(;;);
#endif
}

/********************************************************************************* 
 * Mutexes
 */

static inline int __os_mutex_alloc(void) {
#if ENABLE_THINKOS
	return thinkos_mutex_alloc();
#else
	return 0;
#endif
}

static inline int __os_mutex_free(int mutex) {
#if ENABLE_THINKOS
	return thinkos_mutex_free(mutex);
#else
	return 0;
#endif
}

static inline int __os_mutex_lock(int mutex) {
#if ENABLE_THINKOS
	return thinkos_mutex_lock(mutex);
#else
	return 0;
#endif
}

static inline int __os_mutex_unlock(int mutex) {
#if ENABLE_THINKOS
	return thinkos_mutex_unlock(mutex);
#else
	return 0;
#endif
}

/********************************************************************************* 
 * Conditional variables 
 */

static inline int __os_cond_alloc(void) {
#if ENABLE_THINKOS
	return thinkos_cond_alloc();
#else
	return 0;
#endif
}

static inline int __os_cond_free(int cond) {
#if ENABLE_THINKOS
	return thinkos_cond_free(cond);
#else
	return 0;
#endif
}

static inline int __os_cond_wait(int cond, int mutex) {
#if ENABLE_THINKOS
	return thinkos_cond_wait(cond, mutex);
#else
	return 0;
#endif
}

static inline int __os_cond_timedwait(int cond, int mutex, unsigned int tmo) {
#if ENABLE_THINKOS
	return thinkos_cond_timedwait(cond, mutex, tmo);
#else
	return 0;
#endif
}

static inline int __os_cond_signal(int cond) {
#if ENABLE_THINKOS
	return thinkos_cond_signal(cond);
#else
	return 0;
#endif
}

/********************************************************************************* 
 * Semaphores
 */

static inline int __os_sem_alloc(int val) {
#if ENABLE_THINKOS
	return thinkos_sem_alloc(val);
#else
	return 0;
#endif
}

static inline int __os_sem_free(int sem) {
#if ENABLE_THINKOS
	return thinkos_sem_alloc(sem);
#else
	return 0;
#endif
}

static inline int __os_sem_post(int sem) {
#if ENABLE_THINKOS
	return thinkos_sem_post(sem);
#else
	return 0;
#endif
}

static inline int __os_sem_wait(int sem) {
#if ENABLE_THINKOS
	return thinkos_sem_wait(sem);
#else
	return 0;
#endif
}

static inline int __os_sem_trywait(int sem) {
#if ENABLE_THINKOS
	return thinkos_sem_trywait(sem);
#else
	return 0;
#endif
}


/********************************************************************************* 
 * Threads
 */

static inline int __os_thread_create(int (* task)(void *), void * arg, 
									  void * stack_ptr, unsigned int stack_size,
									  int priority) {
	return thinkos_thread_create(task, arg, stack_ptr, stack_size, 
								 THINKOS_OPT_PRIORITY(priority) |
								 THINKOS_OPT_ID(32));
}

static inline int __os_thread_self(void) {
#if ENABLE_THINKOS
	return thinkos_thread_self();
#else
	return 0;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* __SYS_OS_H__ */
