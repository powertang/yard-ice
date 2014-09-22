/* 
 * Copyright(C) 2012 Robinson Mittmann. All Rights Reserved.
 * 
 * This file is part of the YARD-ICE.
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
 * @brief YARD-ICE
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */


#ifndef __MICROJS_H__
#define __MICROJS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <microjs-rt.h>

enum {
	MICROJS_OK                 = 0,
	MICROJS_UNEXPECTED_CHAR    = -1,
	MICROJS_TOKEN_BUF_OVF      = -3,
	MICROJS_UNCLOSED_STRING    = -4,
	MICROJS_UNCLOSED_COMMENT   = -5,
	MICROJS_INVALID_LITERAL    = -6,
	MICROJS_BRACKET_MISMATCH   = -7,
	MICROJS_STRING_TOO_LONG    = -8,
	MICROJS_EMPTY_FILE         = -9,
	MICROJS_EMPTY_STACK        = -10,
	MICROJS_STRINGS_UNSUPORTED = -11,
	MICROJS_INVALID_SYMBOL     = -12,
	MICROJS_INVALID_LABEL      = -13,
	MICROJS_OBJECT_EXPECTED    = -14,
};

enum {
	OK                      = 0,
	ERR_UNEXPECED_EOF       = 1,
	ERR_UNEXPECTED_CHAR     = 2,
	ERR_UNEXPECTED_SYMBOL   = 3,
	ERR_UNCLOSED_STRING     = 4,
	ERR_UNCLOSED_COMMENT    = 5,
	ERR_INVALID_LITERAL     = 6,
	ERR_INVALID_ID          = 7,
	ERR_STRINGS_UNSUPORTED  = 8,
	ERR_STRING_TOO_LONG     = 9,
	ERR_STRING_NOT_FOUND    = 10,
	ERR_STRBUF_OVERFLOW     = 11,
	ERR_SYNTAX_ERROR        = 12,
	ERR_HEAP_OVERFLOW       = 13,
	ERR_VAR_UNKNOWN         = 14,
	ERR_EXTERN_UNKNOWN      = 15,
	ERR_ARG_MISSING         = 16,
	ERR_TOO_MANY_ARGS       = 17,
	ERR_SYM_PUSH_FAIL       = 18,
	ERR_SYM_POP_FAIL        = 19,
	ERR_OBJ_NEW_FAIL        = 20,
	ERR_SDT_STACK_OVERFLOW  = 21,
	ERR_GENERAL             = 22,
	ERR_CODE_MEM_OVERFLOW   = 23,
	ERR_RET_COUNT_MISMATCH  = 24,
	ERR_INVALID_INSTRUCTION = 25,
};

struct symstat {
	uint16_t sp;
	uint16_t bp;
};

struct symtab;
struct microjs_sdt;

/* --------------------------------------------------------------------------
   External objects/symbols/functions
   -------------------------------------------------------------------------- */

struct ext_fndef {
	const char * nm;
	uint8_t argmin; /* minimum number of arguments */
	uint8_t argmax; /* maximum number of arguments */
	int8_t ret; /* number of returned values */ 
};

/* --------------------------------------------------------------------------
   External library definition 
   -------------------------------------------------------------------------- */

struct ext_libdef {
	const char * name;
	uint8_t fncnt;
	struct ext_fndef fndef[];
};


#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
   Strings 
   -------------------------------------------------------------------------- */

const char * str(unsigned int idx);

int str_add(const char * s, unsigned int len);

int str_lookup(const char * s, unsigned int len);

int cstr_decode(char * dst, const char * src, unsigned int len);

int cstr_add(const char * s, unsigned int len);


const char * const_str(unsigned int idx);

int const_str_add(const char * s, unsigned int len);

int const_str_lookup(const char * s, unsigned int len);

int const_strbuf_dump(FILE * f);

int const_strbuf_purge(void);


struct symtab * symtab_init(uint32_t sym_buf[], 
							unsigned int buf_len, 
							const struct ext_libdef * libdef);

struct symtab * symtab_open(uint32_t * buf, unsigned int len);

struct symstat symtab_state_save(struct symtab * tab);

void symtab_state_rollback(struct symtab * tab, struct symstat st);

struct microjs_sdt * microjs_sdt_init(uint32_t * sdt_buf, 
									  unsigned int sdt_size,
									  struct symtab * tab, 
									  unsigned int data_size);

int microjs_sdt_begin(struct microjs_sdt * microjs, 
					  uint8_t code[], unsigned int code_size);

int microjs_compile(struct microjs_sdt * microjs, 
					const char * txt, unsigned int len);

int microjs_sdt_end(struct microjs_sdt * microjs);

void microjs_sdt_reset(struct microjs_sdt * microjs);

void microjs_sdt_error(FILE * f, struct microjs_sdt * microjs, int err);

int microjs_tgt_heap(struct microjs_sdt * microjs);

#ifdef __cplusplus
}
#endif

#endif /* __MICROJS_H__ */

