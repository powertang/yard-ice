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


/*****************************************************************************
 * MicroJS internal (private) header file
 *****************************************************************************/

#ifndef __MICROJS_I_H__
#define __MICROJS_I_H__

#ifndef __MICROJS_I__
#error "Never use <microjs-i.h> directly; include <microjs.h> instead."
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef CONFIG_H
#include "config.h"
#endif

#include <microjs.h>

#include "microjs_ll.h"

#ifndef MICROJS_ENABLE_STRING
#define MICROJS_ENABLE_STRING 1
#endif

#ifndef MICROJS_SYMBOL_LEN_MAX 
#define MICROJS_SYMBOL_LEN_MAX 16
#endif

#ifndef MICROJS_STRING_LEN_MAX 
#define MICROJS_STRING_LEN_MAX 128
#endif


/* --------------------------------------------------------------------------
  Lexical Analyzer
  -------------------------------------------------------------------------- */

struct lexer {
	uint16_t off;  /* lexer text offset */
	uint16_t len;  /* lexer text length */
	const char * txt;   /* base pointer (original js txt file) */
};

/* --------------------------------------------------------------------------
   String Pool
   -------------------------------------------------------------------------- */

struct strbuf {
	uint16_t cnt;
	uint16_t pos;
	uint16_t offs[];
};

/* --------------------------------------------------------------------------
   External objects/symbols/functions
   -------------------------------------------------------------------------- */

struct ext_entry {
	uint8_t nm;
	uint8_t argmin;
	uint8_t argmax;
};

#define EXT_RAND 0
#define EXT_SQRT 1
#define EXT_LOG2 2
#define EXT_WRITE 3
#define EXT_TIME 4
#define EXT_SRAND 5
#define EXT_PRINT 6
#define EXT_PRINTF 7

/* --------------------------------------------------------------------------
   Symbol table 
   -------------------------------------------------------------------------- */


struct sym {
	uint8_t flags;
	uint8_t nm;
	uint16_t addr;
	uint16_t size;
};

#define SYM_REFERENCE       (0 << 6)
#define SYM_OBJECT          (1 << 7)
#define SYM_EXTERN	        (1 << 6)

#define SYM_OBJ_ALLOC       (1 << 6)


#define SYM_OBJ_INT         (0x0 << 4)
#define SYM_OBJ_STR         (0x1 << 4)
#define SYM_OBJ_INT_ARRAY   (0x2 << 4)
#define SYM_OBJ_STR_ARRAY   (0x3 << 4)

#define SYM_OBJ_TYPE_MASK   (0x3 << 4)
#define SYM_OBJ_TYPE(SYM)   ((SYM)->flags & SYM_OBJ_TYPE_MASK) 
#define SYM_OBJ_IS_STR(SYM) (SYM_OBJ_TYPE(SYM) == SYM_OBJ_STR)
#define SYM_OBJ_IS_INT(SYM) (SYM_OBJ_TYPE(SYM) == SYM_OBJ_INT)

/* object */
struct sym_obj {
	uint8_t flags;
	uint8_t nm;
	uint16_t addr;
	uint16_t size;
};


#define SYM_METHOD    (1 << 0)
#define SYM_IS_METHOD(SYM) ((SYM)->flags & SYM_METHOD)

struct sym_tmp {
	uint8_t flags;
	uint8_t nm;
	uint8_t xid;
	uint8_t cnt;
	uint8_t min;
	uint8_t max;
};


/* external function */
struct sym_ext {
	uint8_t flags;
	uint8_t nm;
	uint16_t addr;
};

/* object reference, this represent a pointer to a 
   target's memory location */
struct sym_ref {
	uint8_t flags;
	uint8_t oid;
	uint16_t addr;
};

struct symtab {
	uint16_t global;
	uint16_t local;
	uint16_t top;
	struct sym sym[];
};

/* --------------------------------------------------------------------------
   Virtual machine
   -------------------------------------------------------------------------- */

#define OPC_ASR      0
#define OPC_SHL      1
#define OPC_ADD      2
#define OPC_SUB      3
#define OPC_MUL      4
#define OPC_DIV      5
#define OPC_MOD      6
#define OPC_OR       7
#define OPC_AND      8
#define OPC_XOR      9
#define OPC_INV      10
#define OPC_NEG      11
#define OPC_I8       12
#define OPC_I16      13
#define OPC_I32      14
#define OPC_LD       15
#define OPC_ST       16
#define OPC_CMP      18
#define OPC_JMP      19
#define OPC_JEQ      20
#define OPC_JNE      21
#define OPC_LT       22
#define OPC_GT       23
#define OPC_EQ       24
#define OPC_NE       25
#define OPC_LE       26
#define OPC_GE       27
#define OPC_LOR      28
#define OPC_LAND     29
#define OPC_PRINT_INT 30
#define OPC_PRINT_CHAR 31
#define OPC_EXT      32
#define OPC_CALL     33
#define OPC_RET      34
#define OPC_POP      35

extern int32_t (* extern_call[])(struct microjs_env *, int32_t [], int);

#ifdef __cplusplus
extern "C" {
#endif

int lexer_open(struct lexer * lex, const char * txt, unsigned int len);

struct token lexer_scan(struct lexer * lex);

void lexer_print_err(FILE * f, struct lexer * lex, int err);

char * tok2str(struct token tok);

int ll_stack_dump(FILE * f, uint8_t * sp, unsigned int cnt);

int sym_dump(FILE * f, struct symtab * tab);

int sym_lookup(struct symtab * tab, const char * s, unsigned int len);


struct sym_obj * sym_obj_new(struct symtab * tab, 
							 const char * s, unsigned int len);

struct sym_obj * sym_obj_lookup(struct symtab * tab, int nm);

struct sym_ref * sym_ref_new(struct symtab * tab, void * sym);

struct sym_ext * sym_ext_new(struct symtab * tab, int nm);

int sym_ext_id(struct symtab * tab, struct sym_ext * ext);

int sym_add_local(struct symtab * tab, const char * s, unsigned int len);

int sym_anom_push(struct symtab * tab);

int sym_anom_pop(struct symtab * tab);

int sym_anom_get(struct symtab * tab, int pos);

struct sym_tmp * sym_tmp_push(struct symtab * tab, 
							  const char * s, unsigned int len);

const char * sym_name(struct symtab * tab, int nm);

int sym_addr_get(struct symtab * tab, int id);

void sym_addr_set(struct symtab * tab, int id, int addr);

struct sym_tmp * sym_tmp_get(struct symtab * tab, int pos);

void sym_pop(struct symtab * tab);

int extern_lookup(int nm);

struct ext_entry * extern_get(unsigned int exid);

/* --------------------------------------------------------------------------
   Strings 
   -------------------------------------------------------------------------- */

const char * str(int idx);

int str_add(const char * s, unsigned int len);

int str_lookup(const char * s, unsigned int len);

int cstr_add(const char * s, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif /* __MICROJS_I_H__ */
