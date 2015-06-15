# Threads #

```
static inline void thinkos_yield(void);
```

```
int thinkos_thread_create(int (* task)(void *), void * arg, 
    void * stack_ptr, unsigned int stack_size, unsigned int opt);
```

```
#define THINKOS_OPT_PRIORITY(VAL) ((VAL) & 0xff)
#define THINKOS_OPT_ID(VAL) (((VAL) & 0x07f) << 8)
#define THINKOS_OPT_PAUSED (1 << 16)
```

```
int thinkos_thread_self(void);
```

```
int thinkos_cancel(unsigned int thread_id, int code);
```

```
int thinkos_join(unsigned int thread_id);
```

```
int thinkos_pause(unsigned int thread_id);
```

```
int thinkos_resume(unsigned int thread_id);
```

```
void thinkos_sleep(unsigned int ms);
```