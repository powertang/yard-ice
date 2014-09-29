/* 
 * Copyright(C) 2014 Robinson Mittmann. All Rights Reserved.
 * 
 * This file is part of the MicroJs
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

/** 
 * @file microjs-i.h
 * @brief Syntax-directed translation compiler
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */



#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "slcdev.h"
#include "slcdev-lib.h"
#include <sys/dcclog.h>

int32_t __rand(void * env, int32_t argv[], int argc) 
{
	argv[0] = rand();

	return 1;
};

int32_t __isqrt(void * env, int32_t argv[], int argc)
{
	uint32_t x = argv[0];
	uint32_t rem = 0;
	uint32_t root = 0;
	int i;

	for (i = 0; i < 16; ++i) {
		root <<= 1;
		rem = ((rem << 2) + (x >> 30));
		x <<= 2;
		root++;
		if (root <= rem) {
			rem -= root;
			root++;
		} else
			root--;
	}

	argv[0] = root >> 1;

	return 1;
}	

int32_t __memrd(void * env, int32_t argv[], int argc)
{
	uint32_t addr = argv[0];

	if (addr >= 256)
		return -1;

	argv[0] = 0;

	return 1;
}	

int32_t __ilog2(void * env, int32_t argv[], int argc)
{
	const uint8_t log2_debruijn_index[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9 };
	int32_t x = argv[0];

	x |= x >> 1; 
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x = (x >> 1) + 1;
	x = (x * 0x077cb531UL) >> 27;
	argv[0] = log2_debruijn_index[x];
	return 1;
}	

extern const char __zeros[];
extern const char __blanks[];

extern int uint2dec(char * s, uint32_t val);
extern int uint2hex(char * s, uint32_t val);

#define BUF_LEN 12
#define PERCENT 0x01
#define WIDTH   0x02
#define ZERO    0x04
#define LEFT    0x08
#define SIGN    0x10
#define LONG    0x20

static int32_t __vprintf(void * env, int32_t argv[], int argc, const char * fmt)
{
	char buf[BUF_LEN];
	int flags;
	int cnt;
	int c;
	int w;
	int n;
	char * cp;
	union {
		void * ptr;
		uint32_t n;
		int i;
	} val;
	int i = 0;

	#define _va_arg(AP, TYPE) ((i < argc) ? argv[i++] : 0)

	n = 0;
	w = 0;
	cnt = 0;
	cp = (char *)fmt;
	for (flags = 0; (c = *fmt++); ) {
		if (flags == 0) {
			if (c == '%') {
				w = 0;
				flags = PERCENT;
				if (n) {
					fwrite(cp, n, 1, stdout);
					cp = (char *)fmt;
					cnt += n;;
					n = 0;
				}
				continue;
			}
			n++;
			continue;
		}

		if ((c >= '0') && (c <= '9')) {
			if (!(flags & WIDTH)) {
				flags |= WIDTH;
				if (c == '0') {
					flags |= ZERO;
					continue;
				}
			}
			/* w = w * 10 + c - '0' */
			w = (((w << 2) + w) << 1) + (c - '0');
			continue;
		}

		if (c == '-') {
			flags |= LEFT;
			continue;
		}

		if (c == '*') {
			w = _va_arg(ap, int);
			continue;
		}

		if (c == 'c') {
			c = _va_arg(ap, int);
			goto print_char;
		}

		if (c == 'd') {
			cp = buf;
			{
				val.i = _va_arg(ap, int);
				if (val.i < 0) {
					buf[0] = '-';
					val.i = -val.i;
					flags |= SIGN;
					cp++;
					w--;
				}
				n = uint2dec(cp, val.n);
			}
			goto print_buf;
		}

		if (c == 'x') {
			cp = buf;
			val.n = _va_arg(ap, unsigned int);
			n = uint2hex(cp, val.n);
			goto print_buf;
		}

		if (c == 's') {
			cp = (char *)str(_va_arg(ap, char *));
			n = strlen(cp);
			goto print_buf;
		}

		if (c == 'u') {
			cp = buf;
			val.n = _va_arg(ap, unsigned int);
			n = uint2dec(cp, val.n);
			goto print_buf;
		}

		flags = 0;
		w = 0;

print_char:
		cp = buf;
		buf[0] = c;
		n = 1;

print_buf:
		if (!(flags & LEFT) && (w > n)) {
			if (flags & ZERO) {
				if (flags & SIGN) {
					flags &= ~SIGN;
					fwrite(buf, 1, 1, stdout);
				}
				fwrite(__zeros, w - n, 1, stdout);
			} else {
				fwrite(__blanks, w - n, 1, stdout);
			}
			cnt += w - n;
		}

		if (flags & SIGN) {
			fwrite(buf, 1, 1, stdout);
			cnt++;
		}

		fwrite(cp, n, 1, stdout);
		cnt += n;

		if ((flags & LEFT) && (w > n)) {
			fwrite(__blanks, w - n, 1, stdout);
			cnt += w - n;
		}

		flags = 0;
		w = 0;

		cp = (char *)fmt;
		n = 0;
	}

	if (n) {
		fwrite(cp, n, 1, stdout);
		cnt+= n;;
	}

	return 0;
}

int32_t __printf(void * env, int32_t argv[], int argc)
{
	const char * fmt = str(argv[0]);

	return __vprintf(env, &argv[1], argc - 1, fmt);
}

int32_t __print(void * env, int32_t argv[], int argc)
{
	int i;


	for (i = 0; i < argc; ++i) {
		if (i != 0)
			__vprintf(env, argv, 0, ", ");
		__vprintf(env, &argv[i], 1, "%d");
	}

	__vprintf(env, argv, 0, "\n");
	return 0;
}	


/* --------------------------------------------------------------------------
   Device models
   -------------------------------------------------------------------------- */

int32_t __model_name(void * env, int32_t argv[], int argc) 
{
	return 0; /* return the number of return values */
}

/* --------------------------------------------------------------------------
   Devices API
   -------------------------------------------------------------------------- */

/* Array index translator */
int32_t __sensor(void * env, int32_t argv[], int argc)
{
	/* just check for bounds */
	if ((uint32_t)argv[0] >= 160)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */
	return 1; /* return the number of return values */
}	

/* Array index translator */
int32_t __module(void * env, int32_t argv[], int argc)
{
	/* just check for bounds */
	if ((uint32_t)argv[0] >= 160)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */
	argv[0] += 160;
	return 1; /* return the number of return values */
}	

int32_t __dev_state(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	argv[0] = ss_dev_tab[idx].alm ? 1 : 0 |
		ss_dev_tab[idx].tbl ? 2 : 0;

	return 1; /* return the number of return values */
}

int32_t __dev_model(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	argv[0] = ss_dev_tab[idx].model;

	return 1; /* return the number of return values */
}

int32_t __dev_addr(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	argv[0] = ss_dev_tab[idx].addr;

	return 1; /* return the number of return values */
}

int32_t __dev_is_module(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	argv[0] = ss_dev_tab[idx].module;

	return 1; /* return the number of return values */
}

int32_t __dev_ap(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	argv[0] = ss_dev_tab[idx].apen;

	return 1; /* return the number of return values */
}

int32_t __dev_enabled(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val > 1)
			return -EXCEPT_INVALID_VALUE;
	
		ss_dev_tab[idx].enabled = 1;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].enabled;

	return 1; /* return the number of return values */
}

int32_t __dev_cfg(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	argv[0] = ss_dev_tab[idx].cfg;

	return 1; /* return the number of return values */
}

int32_t __dev_tst(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val > 1)
			return -EXCEPT_INVALID_VALUE;
	
		ss_dev_tab[idx].tst = 1;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].tst;
	return 1; /* return the number of return values */
}

int32_t __dev_tbias(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val >= 200)
			return -EXCEPT_INVALID_VALUE;
	
		ss_dev_tab[idx].tbias = (val * 128) / 100;
		return 0;
	}

	argv[0] = (ss_dev_tab[idx].tbias * 100) / 128;

	return 1; /* return the number of return values */
}

int32_t __dev_ilat(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val > 250)
			return -EXCEPT_INVALID_VALUE;
	
		ss_dev_tab[idx].ilat = 1;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].ilat;

	return 1; /* return the number of return values */
}

int32_t __dev_imode(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];
	unsigned int icfg;

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	icfg = ss_dev_tab[idx].icfg;

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val > 25)
			return -EXCEPT_INVALID_VALUE;

		icfg = (icfg & ~(0x1f)) + val;
		ss_dev_tab[idx].icfg = icfg;
		return 0;
	}

	argv[0] = icfg & 0x1f;

	return 1; /* return the number of return values */
}

int32_t __dev_irate(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];
	unsigned int icfg;

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	icfg = ss_dev_tab[idx].icfg;

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val > 3)
			return -EXCEPT_INVALID_VALUE;
	
		icfg = (icfg & ~(0x07 << 5)) + (val << 5);
		ss_dev_tab[idx].icfg = icfg;
		return 0;
	}

	argv[0] = icfg >> 5;

	return 1; /* return the number of return values */
}

int32_t __dev_ipre(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val > 250)
			return -EXCEPT_INVALID_VALUE;
	
		ss_dev_tab[idx].ipre = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].ipre;

	return 1; /* return the number of return values */
}


int32_t __dev_alarm(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val >= 16)
			return -EXCEPT_INVALID_ALARM_CODE;
	
		ss_dev_tab[idx].alm = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].alm;

	return 1; /* return the number of return values */
}

int32_t __dev_trouble(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val >= 16)
			return -EXCEPT_INVALID_TROUBLE_CODE;
	
		ss_dev_tab[idx].tbl = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].tbl;

	return 1; /* return the number of return values */
}

int32_t __dev_level(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];
	unsigned int lvl = argv[1];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (lvl >= 4)
		return -EXCEPT_INVALID_LEVEL_VARIABLE; 

	if (argc > 2) {
		unsigned int val = argv[2];
		if (val >= 256)
			return -EXCEPT_INVALID_LEVEL_VALUE;
		ss_dev_tab[idx].lvl[lvl] = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].lvl[lvl];

	return 1; /* return the number of return values */
}

int32_t __dev_out1(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];
		if (val > 1)
			return -EXCEPT_INVALID_VALUE;
		ss_dev_tab[idx].out1 = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].out1;
	return 1; /* return the number of return values */
}

int32_t __dev_out2(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];
		if (val > 1)
			return -EXCEPT_INVALID_VALUE;
		ss_dev_tab[idx].out2 = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].out2;
	return 1; /* return the number of return values */
}

int32_t __dev_out3(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];
		if (val > 1)
			return -EXCEPT_INVALID_VALUE;
		ss_dev_tab[idx].out3 = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].out3;
	return 1; /* return the number of return values */
}

int32_t __dev_out5(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];
		if (val > 1)
			return -EXCEPT_INVALID_VALUE;
		ss_dev_tab[idx].out5 = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].out5;
	return 1; /* return the number of return values */
}

int32_t __dev_pw1(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val >= 8000)
			return -EXCEPT_INVALID_PW_VALUE;
	
		ss_dev_tab[idx].pw1 = val;

		return 0;
	}

	argv[0] = ss_dev_tab[idx].pw1;

	return 1; /* return the number of return values */
}

int32_t __dev_pw2(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val >= 8000)
			return -EXCEPT_INVALID_PW_VALUE;
	
		ss_dev_tab[idx].pw2 = val;

		return 0;
	}

	argv[0] = ss_dev_tab[idx].pw2;

	return 1; /* return the number of return values */
}

int32_t __dev_pw3(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val >= 8000)
			return -EXCEPT_INVALID_PW_VALUE;
	
		ss_dev_tab[idx].pw3 = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].pw3;

	return 1; /* return the number of return values */
}

int32_t __dev_pw4(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[0];

		if (val >= 8000)
			return -EXCEPT_INVALID_PW_VALUE;
	
		ss_dev_tab[idx].pw4 = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].pw4;

	return 1; /* return the number of return values */
}

int32_t __dev_pw5(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[0];

		if (val >= 8000)
			return -EXCEPT_INVALID_PW_VALUE;
	
		ss_dev_tab[idx].pw5 = val;
		return 0;
	}

	argv[0] = ss_dev_tab[idx].pw5;

	return 1; /* return the number of return values */
}

/* Clear device's group list */
int32_t __dev_grp_clear(void * env, int32_t argv[], int argc)
{
	unsigned int idx = argv[0];
	int i;

	if (idx >= 320)
		return -EXCEPT_BAD_ADDR; /* Throw an exception */

	for (i = 0; i < 4; ++i)
		ss_dev_tab[idx].grp[i] = 0;

	return 0; /* return the number of return values */
}


/* --------------------------------------------------------------------------
   Groups API
   -------------------------------------------------------------------------- */

/* Remove all devices form group */
int32_t __grp_clear(void * env, int32_t argv[], int argc)
{
	unsigned int grp = argv[0];
	int j;

	if ((grp == 0) || (grp >= 256))
		return -EXCEPT_INVALID_GROUP; /* Throw an exception */

	/* remove all devices from group */
	for (j = 0; j < 320; ++j) {
		if (ss_dev_tab[j].grp[0] == grp);
			ss_dev_tab[j].grp[0] = 0;
		if (ss_dev_tab[j].grp[1] == grp);
			ss_dev_tab[j].grp[1] = 0;
		if (ss_dev_tab[j].grp[2] == grp);
			ss_dev_tab[j].grp[2] = 0;
		if (ss_dev_tab[j].grp[3] == grp);
			ss_dev_tab[j].grp[3] = 0;
	}

	return 0; /* return the number of return values */
}

/* Insert device into group */
int32_t __grp_insert(void * env, int32_t argv[], int argc)
{
	unsigned int grp = argv[0];
	unsigned int dev = argv[1];
	int i;

	if (dev >= 320)
		return -EXCEPT_INVALID_DEVICE; /* Throw an exception */

	if ((grp == 0) || (grp >= 256))
		return -EXCEPT_INVALID_GROUP; /* Throw an exception */

	for (i = 0; i < 4; ++i) {
		if (ss_dev_tab[dev].grp[i] == grp)
			return 0;
	}
		
	for (i = 0; i < 4; ++i) {
		if (ss_dev_tab[dev].grp[i] == 0) {
			ss_dev_tab[dev].grp[i] = grp;
			return 0;
		}
	}

	return -EXCEPT_TOO_MANY_GROUPS;
}

/* remove device form group */
int32_t __grp_remove(void * env, int32_t argv[], int argc)
{
	unsigned int grp = argv[0];
	unsigned int dev = argv[1];
	int i;

	if (dev >= 320)
		return -EXCEPT_INVALID_DEVICE; /* Throw an exception */

	if ((grp == 0) || (grp >= 256))
		return -EXCEPT_INVALID_GROUP; /* Throw an exception */

	for (i = 0; i < 4; ++i) {
		if (ss_dev_tab[dev].grp[i] == grp)
			ss_dev_tab[dev].grp[i] = 0;
	}

	return 0;
}

int32_t __grp_belong(void * env, int32_t argv[], int argc)
{
	unsigned int grp = argv[0];
	unsigned int dev = argv[1];
	int i;

	if (dev >= 320)
		return -EXCEPT_INVALID_DEVICE; /* Throw an exception */

	if ((grp == 0) || (grp >= 256))
		return -EXCEPT_INVALID_GROUP; /* Throw an exception */

	for (i = 0; i < 4; ++i) {
		if (ss_dev_tab[dev].grp[i] == grp) {
			argv[0] = 1;
			return 1;
		}
	}

	argv[0] = 0;
	return 1;
}

/* --------------------------------------------------------------------------
   LED
   -------------------------------------------------------------------------- */

int32_t js_led_on(void * env, int32_t argv[], int argc)
{
	unsigned int id = argv[0];

	if (id >= 6)
		return -EXCEPT_INVALID_LED; /* Throw an exception */

	if (argc > 1) {
		unsigned int val = argv[1];

		if (val > 1)
			return -EXCEPT_INVALID_VALUE;

		if (val)
			led_on(id);
		else
			led_off(id);

		return 0;
	}

	argv[0] = led_status(id);

	return 1; /* return the number of return values */
}

int32_t js_led_flash(void * env, int32_t argv[], int argc)
{
	unsigned int id = argv[0];
	unsigned int ms = argv[1];


	if (id >= 6)
		return -EXCEPT_INVALID_LED; /* Throw an exception */

	if (ms >= (256 * IO_POLL_PERIOD_MS))
		return -EXCEPT_INVALID_VALUE;

	led_flash(id, ms);

	return 0; /* return no values */
}

/* --------------------------------------------------------------------------
   Native (external) call table
   -------------------------------------------------------------------------- */

int32_t (* const microjs_extern[])(void *, int32_t [], int) = {
	[EXT_PRINTF] = __printf,
	[EXT_PRINT] = __print,
	[EXT_RAND] = __rand,
	[EXT_SQRT] = __isqrt,
	[EXT_LOG2] = __ilog2,
	[EXT_MEMRD] = __memrd,

	[EXT_MODEL_NAME] = __model_name,

	[EXT_SENSOR] = __sensor,
	[EXT_MODULE] = __module,

	[EXT_DEV_STATE] = __dev_state,
	[EXT_DEV_MODEL] = __dev_model,
	[EXT_DEV_ADDR] = __dev_addr,
	[EXT_DEV_IS_MODULE] = __dev_is_module,
	[EXT_DEV_AP] = __dev_ap,
	[EXT_DEV_EN] = __dev_enabled,
	[EXT_DEV_CFG] = __dev_cfg,
	[EXT_DEV_TST] = __dev_tst,
	[EXT_DEV_TBIAS] = __dev_tbias,
	[EXT_DEV_ILAT] = __dev_ilat,
	[EXT_DEV_IMODE] = __dev_imode,
	[EXT_DEV_IRATE] = __dev_irate,
	[EXT_DEV_IPRE] = __dev_ipre,
	[EXT_DEV_ALARM] = __dev_alarm,
	[EXT_DEV_TROUBLE] = __dev_trouble,
	[EXT_DEV_LEVEL] = __dev_level,
	[EXT_DEV_OUT1] = __dev_out1,
	[EXT_DEV_OUT2] = __dev_out2,
	[EXT_DEV_OUT3] = __dev_out3,
	[EXT_DEV_OUT5] = __dev_out5,
	[EXT_DEV_PW1] = __dev_pw1,
	[EXT_DEV_PW2] = __dev_pw2,
	[EXT_DEV_PW3] = __dev_pw3,
	[EXT_DEV_PW4] = __dev_pw4,
	[EXT_DEV_PW5] = __dev_pw5,
	[EXT_DEV_GRP_CLEAR] = __dev_grp_clear,

	[EXT_GRP_CLEAR] = __grp_clear,
	[EXT_GRP_INSERT] = __grp_insert,
	[EXT_GRP_REMOVE] = __grp_remove,
	[EXT_GRP_BELONG] = __grp_belong,

	[EXT_LED_ON] = js_led_on,
	[EXT_LED_FLASH] = js_led_flash,
};
