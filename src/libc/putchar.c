/* $Id: putchar.c,v 1.1 2007/09/26 02:01:22 bob Exp $ 
 *
 * File:	putchar.c
 * Module:
 * Project:	
 * Author:	Robinson Mittmann (bob@boreste.com)
 * Target:	
 * Comment:
 * Copyright(c) 2006-2007 BORESTE (www.boreste.com). All Rights Reserved.
 *
 */

#include <stdio.h>

int putchar(int c)
{
	return fputc(c, stdout);
}

