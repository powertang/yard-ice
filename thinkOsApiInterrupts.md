void thinkos\_irq\_wait(int irq);

```
#define IRQ_PRIORITY_HIGHEST   0x00
#define IRQ_PRIORITY_VERY_HIGH 0x20
#define IRQ_PRIORITY_HIGH      0x40
#define IRQ_PRIORITY_REGULAR   0x80
#define IRQ_PRIORITY_LOW       0xe0
```

```
void __thinkos_critical_enter(void);
```

```
void __thinkos_critical_exikt(void);
```

```
int __thinkos_ev_alloc(void);
```

```
void __thinkos_ev_free(int ev);
```

```
void __thinkos_ev_wait(int ev);
```

```
void __thinkos_ev_raise(int ev);
```

```
void __thinkos_irq_pri_set(unsigned int irq, unsigned int priority);
```

```
void __thinkos_irq_enable(unsigned int irq);
```