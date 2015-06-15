# Introduction #

ThinkOS is a Real Time Operating System designed for the ARM Cortex-M core. It takes advantage of some unique features of this processor to enable for very low task switch latency.

# API #

The ThinkOS API can be separated according to the type of object the operation is performed ...

  * **[Core](thinkOsApiCore.md)** - initialization configuration and debug
  * **[Threads](thinkOsApiThread.md)** - functions related to threads
  * **[Mutexes](thinkOsApiMutex.md)**
  * **[Conditional Variables](thinkOsApiCondVar.md)**
  * **[Semaphores](thinkOsApiSemaphores.md)**
  * **[Events](thinkOsApiEvents.md)**
  * **[Interrupts](thinkOsApiInterrupts.md)**
  * **[Time](thinkOsApiTime.md)**

## Kernel Objects ##

One feature that distinguish the **ThinkOS** from most of other Operating Systems is the way the control structures for the several type of objects are held by kernel. The usual approach for holding this information is to allocate a structure specific for a certain object and pass a pointer to the kernel. The kernel then stores this in a list or array. ThinkOS in other hand holds the information on a _structure of arrays_, each array refers to one field of an object. This may look very obscure but the idea here is to protect the memory and speed-up the scheduler.

## Threads ##

The ThinkOS is a preemptive time-sharing kernel.
Each thread have it's own stack and a timer. This timer is used to block the thread's execution for a certain period of time as in **thinkos\_sleep()**.

# Scheduler #

ThinkOS scheduler was designed to reduce the context switch time to a minimum. The most important piece on the Kernel is the Scheduler....

# Time Sharing #

All threads with priority higher than 0 go into a round-robin time sharing scheduling policy, when this feature is enabled. Their execution time will be inversely proportional to their priority.

# Interrupt Model #

There are two types of interrupt handlers with ThinkOS:

  * Implicit ISR - Thread interrupt handlers that executes as normal threads. This is a simple call to **thinkos\_irq\_wait(_IRQ_)** inside a regular task or function. The thread will block until this interrupt is raised. This mechanism provides a convenient and powerful tool to design drivers. Devices that benefit most of this type of ISR are simplex I/O channels like I2C or SPI, hardware accelerators like cryptographic ...

  * Native ISR - Normal interrupt handlers that shares a common stack. These handlers are subject to to some constraints regarding API calls. It is recommended to use this type of interrupt handler when low latency is a must. But bare in mind that if the handler is too complex the actual latency due to loading registers and housekeeping may be equivalent to a context switch in which case it will be equivalent of a threaded handler in terms of performance. Another case will be a high priority interrupt you want to make sure it will won't  be preempted...

# Configuration #

The file **include/thinkos\_sys.h** contains the definition of the structures used by the OS. In this file there is a series of macros used to set default values for the configuration options.

It is not recommended to change the values directly in _thinkos\_sys.h_, create a _config.h_ file instead, and define the macro CONFIG\_H at the compiler's: -DCONFIG\_H.

Note: if you are using the ThinkOS as part of the YARD-ICE there is a _config.h_ file located at the source root.

## Preformance ##

The main bottleneck for performance is usually the scheduler. The ThinkOS running in a 120MHz Cortex-M3 (STM32F270) have a measured latency time of 0.5 microseconds.

There are some tests and calculations that need to be done to determine the latencies of other elements of the system...