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
 * @file mod_mem.c
 * @brief YARD-ICE
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */


#include <stdlib.h>
#include <string.h>

#include "target.h"
#include "dbglog.h"

#include "module.h"
#include "val.h"
#include "var.h"

int mod_mem_var_get(struct ice_mem_entry * mem, int var_id, value_t * val)
{
	DCC_LOG1(LOG_INFO, "var_id=%d", var_id);

	val->uint32 = mem[var_id].addr.base + mem[var_id].addr.offs;
	return 0;
}

int mod_mem_var_set(struct ice_mem_entry * mem, int var_id, const value_t * val)
{
	return -1;
}


int mod_mem_on_load(struct ice_mem_entry * mem, int mod_id)
{
	int id;

	DCC_LOG2(LOG_INFO, "mem=0x%p mod_id=%d", mem, mod_id);

	for(id = 0; mem[id].op != NULL; ++id) {
		uint32_t addr;
#if 0
		uint32_t size;

		if ((size = mem[id].blk.count * mem[id].blk.size) == 0)
			continue; /* skip empty blocks */
#endif


		addr = mem[id].addr.base + mem[id].addr.offs;
		(void)addr;

		DCC_LOG3(LOG_INFO, "mem=\"%s\" id=%d addr=0x%08x", 
				 mem[id].name, id, addr);

		if (var_global_add(mod_id, mem[id].name, TYPE_UINT32, id) < 0) {
			DCC_LOG(LOG_WARNING, "var_global_add() fail!");
			break;
		}
	}

	return 0;
}

int mod_mem_on_unload(struct ice_mem_entry * mem, int mod_id)
{
	int id;

	DCC_LOG2(LOG_INFO, "mem=0x%p mod_id=%d", mem, mod_id);

	for(id = 0; mem[id].op != NULL; id++) {
#if 0
		uint32_t size;

		if ((size = mem[id].blk.count * mem[id].blk.size) == 0)
			continue; /* skip empty blocks */
#endif

		if (var_global_del(mod_id, id) < 0) {
			DCC_LOG(LOG_WARNING, "var_global_del() fail!");
			break;
		}
	}

	return 0;
}

const struct module_def mem_module = {
	.name = "mem",
	.init = (module_init_t)mod_mem_on_load,
	.done = (module_done_t)mod_mem_on_unload,
	.var_get = (module_var_get_t)mod_mem_var_get,
	.var_set = (module_var_set_t)mod_mem_var_set
};

int mod_mem_register(struct ice_mem_entry * mem)
{
	if (mem == NULL)
		return -1;

	DCC_LOG1(LOG_INFO, "mem=0x%p", mem);

	return module_register(&mem_module, mem);
}

