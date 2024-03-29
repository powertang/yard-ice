/* 
 * File:	ice-comm.c
 * Author:  Robinson Mittmann (bobmittmann@gmail.com)
 * Target: 
 * Comment: 
 * Copyright(C) 2012 Bob Mittmann. All Rights Reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <sys/ice-comm.h>
#include <arch/cortex-m3.h>

struct ice_comm_blk ice_comm_blk;

void ice_comm_sync(void) 
{
	uint32_t fm = cm3_faultmask_get(); /* save fault mask */

	cm3_cpsid_f(); /* disable interrupts and faults */

	ice_comm_blk.dev = DEV_SYNC;
	ice_comm_blk.tx_head = 0;
	ice_comm_blk.tx_tail = 0;

	cm3_faultmask_set(fm);  /* restore fault mask */
}

void ice_comm_connect(void) 
{
	uint32_t fm = cm3_faultmask_get(); /* save fault mask */

	cm3_cpsid_f(); /* disable interrupts and faults */
	ice_comm_blk.dev = DEV_SYNC;
	while (ice_comm_blk.dbg != DBG_CONNECTED) {
		if (ice_comm_blk.dbg == DBG_SYNC) {
			ice_comm_blk.dev = DEV_CONNECTED;
		}
	}
	cm3_faultmask_set(fm);  /* restore fault mask */
}

