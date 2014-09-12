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
 * @file microjs_str.c
 * @brief Strings
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */


#define __MICROJS_I__
#include "microjs-i.h"

#define __DEF_CONST_STRBUF__
#include "const_str.h"

#include <string.h>

#define CONST_NM (256 - CONST_STRINGS_MAX)

/* --------------------------------------------------------------------------
   Strings Tables
   -------------------------------------------------------------------------- */

static const struct strbuf empty_strbuf= {
	.cnt = 0,
	.pos = 0
};

/* initialize the variables string buffer to the empty buffer for 
   safety */
struct strbuf * var_strbuf = (struct strbuf *)&empty_strbuf;

void strbuf_init(uint16_t * buf, unsigned int len)
{
	var_strbuf = (struct strbuf *)buf;
	var_strbuf->pos = len; 
	var_strbuf->cnt = 0; 
	var_strbuf->offs[0] = 0;
}

int str_lookup(const char * s, unsigned int len)
{
	int i;

	/* look in the constant pool first */
	for (i = 0; i < const_strbuf.cnt; ++i) {
		char * cstr = (char *)&const_strbuf + const_strbuf.offs[i];
		if ((strncmp(cstr, s, len) == 0) && (cstr[len] == '\0'))
			return i + CONST_NM;
	}

	for (i = 0; i < var_strbuf->cnt; ++i) {
		char * cstr = (char *)var_strbuf + var_strbuf->offs[i];
		if ((strncmp(cstr, s, len) == 0) && (cstr[len] == '\0'))
			return i;
	}

	return -ERR_STRING_NOT_FOUND;
}

int str_add(const char * s, unsigned int len)
{
	char * dst;
	int idx;
	int offs;

	if ((idx = str_lookup(s, len)) >= 0)
		return idx;

	offs = var_strbuf->pos - (len + 1);

	idx = var_strbuf->cnt;
	if (offs < ((idx + 1) * sizeof(uint16_t))) {
		return ERR_STRBUF_OVERFLOW;
	}

	/* Copy the string to the buffer */
	dst = (char *)var_strbuf + offs;
	memcpy(dst, s, len);
	/* NULL terminate the string */
	dst[len] = '\0';
	var_strbuf->offs[idx] = offs;
	var_strbuf->cnt++;
	var_strbuf->pos -= len + 1;

	return idx;
}

/* add a string to the var buffer translating the most 
   common C scape sequences */
int cstr_add(const char * s, unsigned int len)
{
	char * dst;
	bool esc;
	int offs;
	int idx;
	int i;
	int c;

	if ((idx = str_lookup(s, len)) >= 0)
		return idx;

	offs = var_strbuf->pos - (len + 1);
	idx = var_strbuf->cnt;
	if (offs < ((idx + 1) * sizeof(uint16_t)))
		return ERR_STRBUF_OVERFLOW;

	/* Copy the string to the buffer */
	dst = (char *)var_strbuf + offs;

	/* FIXME: as the strings are allocated top-down converting from
	   left to right leave some unused spaces in the end of the allocated
	   string */

	/* FIXME: octal and hexadecimal coded chars */

	/* Copy the string to the buffer */
	esc = false;
	for (i = 0; i < len; ++i) {
		c = s[i];
		if (esc) {
			switch (c) {
			case 'a': /* Alarm (Bell) */
				c = '\a';
				break;
			case 'b': /* Backspace */
				c = '\b';
				break;
			case 'f': /* Formfeed */
				c = '\f';
				break;
			case 'n': /* Newline */
				c = '\n';
				break;
			case 'r': /* Carriage Return */
				c = '\r';
				break;
			case 't': /* Horizontal Tab */
				c = '\t';
				break;
			case 'v': /* Vertical Tab */
				c = '\v';
				break;
			}
			esc = false;
		} else if (c == '\\') {
			esc = true;
			continue;
		} 
		*dst++ = c;
	}
	/* NULL terminate the string */
	*dst = '\0';
	var_strbuf->offs[idx] = offs;
	var_strbuf->cnt++;
	var_strbuf->pos -= len + 1;

	return idx;
}

/* translate a string index to string value. If the index is invalid
   returns the empty string */
const char * str(int idx)
{
	if (idx >= CONST_NM) {
		if (idx > 256)
			idx = CONST_NM;
		return (char *)&const_strbuf + const_strbuf.offs[idx - CONST_NM];
	}

	if (idx >= var_strbuf->cnt) 
		return (char *)&const_strbuf + const_strbuf.offs[0];

	return (char *)var_strbuf + var_strbuf->offs[idx];
}

