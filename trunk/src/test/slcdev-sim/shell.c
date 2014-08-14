/* 
 * File:	 usb-test.c
 * Author:   Robinson Mittmann (bobmittmann@gmail.com)
 * Target:
 * Comment:
 * Copyright(C) 2011 Bob Mittmann. All Rights Reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <sys/shell.h>
#include <sys/tty.h>
#include <sys/param.h>
#include <xmodem.h>
#include <thinkos.h>

#include <slcdev.h>

#include <sys/dcclog.h>
#include <sys/delay.h>

#include <microjs.h>

#include "flashfs.h"
#include "isink.h"

extern const struct shell_cmd cmd_tab[];

/*****************************************************************************
 * Help
 *****************************************************************************/

int cmd_help(FILE *f, int argc, char ** argv)
{
	struct shell_cmd * cmd;

	if (argc > 2)
		return -1;

	if (argc > 1) {
		if ((cmd = cmd_lookup(argv[1], cmd_tab)) == NULL) {
			fprintf(f, " Not found: '%s'\n", argv[1]);
			return -1;
		}

		fprintf(f, "  %s, %s - %s\n", cmd->name, cmd->alias, cmd->desc);
		fprintf(f, "  usage: %s %s\n\n", argv[1], cmd->usage);

		return 0;
	}

	fprintf(f, "\n Command:   Alias:  Desciption: \n");
	for (cmd = (struct shell_cmd *)cmd_tab; cmd->callback != NULL; cmd++) {
		fprintf(f, "  %-10s %-4s   %s\n", cmd->name, cmd->alias, cmd->desc);
	}

	return 0;
}

/*****************************************************************************
 * FPGA
 *****************************************************************************/

int flash_xmodem_recv(FILE * f, uint32_t offs, unsigned int size)
{
	struct comm_dev comm;
	struct file * raw;
	uint8_t buf[128];
	unsigned int cnt;
	unsigned int rem;
	int ret;

	union {
		struct xmodem_rcv rx;
		struct xmodem_snd sx;
	} xmodem;

	raw = ftty_lowlevel(f);

	comm.arg = raw->data;
	comm.op.send = (int (*)(void *, const void *, unsigned int))raw->op->write;
	comm.op.recv = (int (*)(void *, void *, 
						  unsigned int, unsigned int))raw->op->read;

	DCC_LOG(LOG_TRACE, ".................................");

	xmodem_rcv_init(&xmodem.rx, &comm, XMODEM_RCV_CRC);

	cnt = 0;
	rem = size;
	do {
		unsigned int n;

		if ((ret = xmodem_rcv_loop(&xmodem.rx, buf, 128)) < 0) {
			DCC_LOG1(LOG_ERROR, "ret=%d", ret);
			return ret;
		}

		if (rem == 0) {
			xmodem_rcv_cancel(&xmodem.rx);
			break;
		}

		n = MIN(rem, ret); 
		stm32_flash_write(offs, buf, n);
		cnt += n;
		offs += n;
		rem -= n;
	} while (ret > 0);

	return cnt;
}

int cmd_rx(FILE * f, int argc, char ** argv)
{
	uint32_t blk_offs;
	uint32_t blk_size;
	struct fs_dirent entry;

	if (argc < 2)
		return SHELL_ERR_ARG_MISSING;

	if (argc > 2)
		return SHELL_ERR_EXTRA_ARGS;

	if (!fs_dirent_lookup(argv[1], &entry)) {
		fprintf(f, "invalid file: \"%s\"\n", argv[1]);
		return SHELL_ERR_ARG_INVALID;
	}

	blk_offs = entry.offs;
	blk_size = entry.blk_size;

	fprintf(f, "Erasing block: 0x%06x, %d bytes...\n", blk_offs, blk_size);
	if (stm32_flash_erase(blk_offs, blk_size) < 0) {
		fprintf(f, "stm32f_flash_erase() failed!\n");
		return -1;
	};

	fprintf(f, "RX waiting to receive.");
	if (flash_xmodem_recv(f, blk_offs, blk_size) < 0) {
		fprintf(f, "flash_xmodem_recv() failed!\n");
		return -1;
	}

	return 0;
}

int cmd_cat(FILE * f, int argc, char ** argv)
{
	struct fs_dirent entry;
	char * cp;
	int cnt;

	if (argc < 2)
		return SHELL_ERR_ARG_MISSING;

	if (argc > 2)
		return SHELL_ERR_EXTRA_ARGS;

	if (!fs_dirent_lookup(argv[1], &entry)) {
		fprintf(f, "invalid file: \"%s\"\n", argv[1]);
		return SHELL_ERR_ARG_INVALID;
	}

	cp = (char *)(STM32_MEM_FLASH + entry.offs);
	DCC_LOG2(LOG_TRACE, "cp=0x%08x len=%d", cp, strlen(cp));

	for (cnt = 0; cnt < entry.size; ) { 
		int n;

		n = strlen(cp);
		if (n > 0) {
			fprintf(f, "%s\n", cp);
		} else
			n = 1;

		cnt += n;
		cp += n;
	}

	return 0;
}

int cmd_ls(FILE * f, int argc, char ** argv)
{
	struct fs_dirent entry;

	if (argc > 1)
		return SHELL_ERR_EXTRA_ARGS;

	memset(&entry, 0, sizeof(entry));

	while (fs_dirent_get_next(&entry)) {
		fprintf(f, "0x%06x %6d %6d %s\n", entry.offs, 
				entry.blk_size, entry.size, entry.name);
	}

	return 0;
}


int cmd_trig(FILE * f, int argc, char ** argv)
{
	unsigned int addr;

	if (argc > 2)
		return SHELL_ERR_EXTRA_ARGS;

	if (argc == 2) {
		addr = strtoul(argv[1], NULL, 0);
		if (addr > 319)
			return SHELL_ERR_ARG_INVALID;

		trig_addr_set(addr);
	}

	addr = trig_addr_get();

	fprintf(f, "Trigger address: %d\n", addr);

	return 0;
}

int cmd_enable(FILE * f, int argc, char ** argv)
{
	unsigned int addr;
	int i;

	if (argc < 2)
		return SHELL_ERR_ARG_MISSING;

	if ((argc == 2) && (strcmp(argv[1], "all") == 0)) {
		for (addr = 1; addr < 320; ++addr)
			dev_sim_enable(addr);
		fprintf(f, "All devices are enabled\n");
		return 0;
	}

	for (i = 1; i < argc; ++i) {
		addr = strtoul(argv[i], NULL, 0);
		if ((addr < 1) || (addr > 319))
			return SHELL_ERR_ARG_INVALID;

		fprintf(f, "Device enabled: %d\n", addr);
		dev_sim_enable(addr);
	}

	return 0;
}

int cmd_disable(FILE * f, int argc, char ** argv)
{
	unsigned int addr;
	int i;

	if (argc < 2)
		return SHELL_ERR_ARG_MISSING;

	if ((argc == 2) && (strcmp(argv[1], "all") == 0)) {
		for (addr = 1; addr < 320; ++addr)
			dev_sim_disable(addr);
		fprintf(f, "All devices are disabled\n");
		return 0;
	}

	for (i = 1; i < argc; ++i) {
		addr = strtoul(argv[i], NULL, 0);
		if ((addr < 1) || (addr > 319))
			return SHELL_ERR_ARG_INVALID;

		fprintf(f, "Device disable: %d\n", addr);
		dev_sim_disable(addr);
	}

	return 0;
}

int cmd_self_test(FILE * f, int argc, char ** argv)
{
	if (argc > 1)
		return SHELL_ERR_EXTRA_ARGS;

	lamp_test();

	return 0;
}

void isink_test(void);

int cmd_isink(FILE * f, int argc, char ** argv)
{
	unsigned int mode = 0;
	unsigned int rate = 0;
	unsigned int pre = 50;
	unsigned int pulse = 200;

	if (argc > 5)
		return SHELL_ERR_EXTRA_ARGS;

	if (argc > 1) {
		mode = strtoul(argv[1], NULL, 0);
		if (mode > 25)
			return SHELL_ERR_ARG_INVALID;
		if (argc > 2) {
			rate = strtoul(argv[2], NULL, 0);
			if (rate > 3)
				return SHELL_ERR_ARG_INVALID;
			if (argc > 3) {
				pulse = strtoul(argv[3], NULL, 0);
				if (argc > 4)
					pre = strtoul(argv[4], NULL, 0);
			}
		}

		isink_mode_set(mode | (rate << 5));
		isink_pulse(pre, pulse);
	} else {
		isink_test();
	}
	
	return 0;
}

int cmd_slewrate(FILE * f, int argc, char ** argv)
{
	unsigned int rate = 0;

	if (argc < 2)
		return SHELL_ERR_ARG_MISSING;

	if (argc > 2)
		return SHELL_ERR_EXTRA_ARGS;

	rate = strtoul(argv[1], NULL, 0);

	isink_slewrate_set(rate);

	return 0;
}


int xflash(void * uart, uint32_t addr, unsigned int size);


int cmd_xflash(FILE * f, int argc, char ** argv)
{
//	struct cm3_systick * systick = CM3_SYSTICK;
	struct stm32_usart * uart = STM32_USART2;
	uint32_t offs = 0x00000000;
	uint32_t len = 32768;

	if (argc < 2)
		return SHELL_ERR_ARG_MISSING;

	if (argc > 3)
		return SHELL_ERR_EXTRA_ARGS;

	offs = strtoul(argv[1], NULL, 0);

	if (argc > 2)
		len = strtoul(argv[2], NULL, 0);

	DCC_LOG2(LOG_TRACE, "offs=%08x len=%d", offs, len);

	if (offs > 0x20000)
		return SHELL_ERR_ARG_INVALID;

	if (len == 0)
		return SHELL_ERR_ARG_INVALID;

	if ((offs + len) > 0x20000)
		return SHELL_ERR_ARG_INVALID;

	/* disable interrupts */	
	cm3_cpsid_i();

#if 0
	for (;;) {
		int cnt = 1000;
		while (cnt > 0) {
			if (systick->ctrl & SYSTICK_CTRL_COUNTFLAG)
				cnt--;
		}
		DCC_LOG(LOG_TRACE, "tick.");
	}
#endif

	xflash(uart, offs, len);

	return 0;
}

int cmd_eeprom(FILE * f, int argc, char ** argv)
{
	unsigned int offs;
	unsigned int data;

	if (argc > 3)
		return SHELL_ERR_EXTRA_ARGS;

	stm32_eeprom_unlock();

	if (argc == 2) {
		if ((strcmp(argv[1], "test") == 0) || 
			(strcmp(argv[1], "t") == 0)) {
			fprintf(f, "Testing EEPROM...\n");
			data = rand();
			for (offs = 0; offs < 4096; offs += 4) {
				stm32_eeprom_wr32(offs, data);
				data++;
			}

		} else if ((strcmp(argv[1], "erase") == 0) || 
			(strcmp(argv[1], "e") == 0)) {
			fprintf(f, "Erasing EEPROM...\n");
			for (offs = 0; offs < 4096; offs += 4)
				stm32_eeprom_wr32(offs, 0);
		}
	}

		
	return 0;
}

int cmd_dbase(FILE * f, int argc, char ** argv)
{
//	if (argc < 2)
//		return SHELL_ERR_ARG_MISSING;

	if (argc == 1) {
		device_db_dump(f);
	}

	if (argc == 2) {
		if ((strcmp(argv[1], "compile") == 0) || 
			(strcmp(argv[1], "c") == 0)) {
			device_db_compile();
		} else if ((strcmp(argv[1], "erase") == 0) || 
			(strcmp(argv[1], "e") == 0)) {
			device_db_erase();
		}
	}

	return 0;
}

int cmd_config(FILE * f, int argc, char ** argv)
{
//	if (argc < 2)
//		return SHELL_ERR_ARG_MISSING;

	if (argc == 1) {
		config_dump(f);
	}

	if (argc == 2) {
		if ((strcmp(argv[1], "compile") == 0) || 
			(strcmp(argv[1], "c") == 0)) {
			config_compile();
		} else if ((strcmp(argv[1], "erase") == 0) || 
			(strcmp(argv[1], "e") == 0)) {
			config_erase();
		}
	}

	return 0;
}

int device_dump(FILE * f, int addr);
int device_attr_set(int addr, const char * name, const char * val);
int device_attr_print(FILE * f, int addr, const char * name);

int cmd_dev(FILE * f, int argc, char ** argv)
{
	int addr;

	if (argc < 2)
		return SHELL_ERR_ARG_MISSING;

	if (argc > 4)
		return SHELL_ERR_EXTRA_ARGS;

	addr = strtoul(argv[1], NULL, 0);
	if (addr > 319)
		return SHELL_ERR_ARG_INVALID;

	if (argc > 2) {
		if (argc > 3)
			return device_attr_set(addr, argv[2], argv[3]);
		return device_attr_print(f, addr, argv[2]);
	} else {
		device_dump(f, addr);
	}

	return 0;
}

int device_pw3_lookup(unsigned int addr, unsigned int sel);

int cmd_pw2(FILE * f, int argc, char ** argv)
{
	return 0;
}

int cmd_pw3(FILE * f, int argc, char ** argv)
{
	int addr;
	int sel;

	if (argc < 2)
		return SHELL_ERR_ARG_MISSING;

	if (argc > 3)
		return SHELL_ERR_EXTRA_ARGS;

	addr = strtoul(argv[1], NULL, 0);
	if (addr > 319)
		return SHELL_ERR_ARG_INVALID;

	sel = strtoul(argv[1], NULL, 0);
	if (sel > 10)
		return SHELL_ERR_ARG_INVALID;

	return device_pw3_lookup(addr, sel);
}

int cmd_pw4(FILE * f, int argc, char ** argv)
{
	return 0;
}

int cmd_pw5(FILE * f, int argc, char ** argv)
{
	return 0;
}

int cmd_str(FILE * f, int argc, char ** argv)
{
	int ret = 0;
	int i;

	if (argc == 1)
		return microjs_str_pool_dump(&microjs_str_const);

	for (i = 1; i < argc; ++i) {
		if ((ret = slcdev_const_str_write(argv[i], strlen(argv[i]))) < 0)
			break;
	}

	return ret;
}

int cmd_reboot(FILE * f, int argc, char ** argv)
{
	if (argc > 1)
		return SHELL_ERR_EXTRA_ARGS;

	cm3_sysrst();

	return 0;
}


const struct shell_cmd cmd_tab[] = {

	{ cmd_help, "help", "?", 
		"[COMMAND]", "show command usage (help [CMD])" },

	{ cmd_rx, "rx", "r", "FILENAME", "XMODEM file receive" },

	{ cmd_trig, "trig", "t", "[ADDR]", "Trigger module address get/set" },

	{ cmd_enable, "enable", "e", "ADDR0 .. ADDR7", "Device enable" },

	{ cmd_disable, "disable", "d", "ADDR0 .. ADDR7", "Device disable" },

	{ cmd_self_test, "selftest", "st", "", "Self test" },

	{ cmd_isink, "isink", "i", "[MODE [PRE [PULSE]]]", "Self test" },

	{ cmd_slewrate, "slewrate", "sr", "[VALUE]", "Current slewrate set" },

	{ cmd_xflash, "xflash", "xf", "OFFS [LEN]", "Firmware update" },

	{ cmd_eeprom, "eeprom", "ee", "", "EEPROM test" },

	{ cmd_dbase, "dbase", "db", "[compile|stat]", "device database" },

	{ cmd_config, "config", "cfg", "[compile|stat]", 
		"simulator configuration" },

	{ cmd_cat, "cat", "", "<filename>", "display file content" },

	{ cmd_dev, "dev", "", "<addr> [attr [VAL]]", "get/set device attribute" },

	{ cmd_ls, "ls", "", "<filename>", "list files" },

	{ cmd_pw2, "pw2", "", "<addr> [set [VAL]] | [lookup [SEL]]>", 
		"get set PW2 value" },

	{ cmd_pw3, "pw3", "", "<addr> [set [VAL]] | [lookup [SEL]]>", 
		"get set PW3 value" },

	{ cmd_pw4, "pw4", "", "<addr> [set [VAL]] | [lookup [SEL]]>", 
		"get set PW4 value" },

	{ cmd_pw5, "pw5", "", "<addr> [set [VAL]] | [lookup [SEL]]>", 
		"get set PW4 value" },

	{ cmd_str, "str", "", "", "dump string pool" },

	{ cmd_reboot, "reboot", "rst", "", "reboot" },

	{ NULL, "", "", NULL, NULL }
};
