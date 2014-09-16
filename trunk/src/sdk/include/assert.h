#ifdef	__ASSERT_H__

#undef	__ASSERT_H__
#undef	assert

#endif /* __ASSERT_H__ */

#define	__ASSERT_H__

#if defined __cplusplus && __GNUC_PREREQ (2,95)
# define __ASSERT_VOID_CAST static_cast<void>
#else
# define __ASSERT_VOID_CAST (void)
#endif

#define __CONCAT(x,y)	x ## y
#define __STRING(x)	#x

#ifdef	NDEBUG

# define assert(expr)		(void)

#else /* Not NDEBUG.  */

/* This prints an "Assertion failed" message and aborts.  */
extern void __assert_fail (const char *__assertion, const char *__file, 
						   unsigned int __line, const char *__function) 
	__attribute__ ((__noreturn__));

# define assert(expr) \
  ((expr) \
   ? (void) \
   : __assert_fail (__STRING(expr), __FILE__, __LINE__, __func__))

#endif /* NDEBUG.  */
