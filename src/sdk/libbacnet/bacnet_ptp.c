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

#include <sys/param.h>
#include <thinkos.h>
#include <crc.h>
#include <errno.h>

#include <sys/dcclog.h>

#include <bacnet/bacnet-ptp.h>

#include "dcc.h"


/* -------------------------------------------------------------------------
 * 10 DATA LINK/PHYSICAL LAYERS: POINT-TO-POINT (PTP)
 * ------------------------------------------------------------------------- */

/* PTP Frame  types */
enum {
	BACNET_PTP_FRM_HEARTBEAT_XOFF  = 0x00,
	BACNET_PTP_FRM_HEARTBEAT_XON   = 0x01,
	BACNET_PTP_FRM_DATA_0          = 0x02,
	BACNET_PTP_FRM_DATA_1          = 0x03,
	BACNET_PTP_FRM_DATA_ACK_0_XOFF = 0x04,
	BACNET_PTP_FRM_DATA_ACK_1_XOFF = 0x05,
	BACNET_PTP_FRM_DATA_ACK_0_XON  = 0x06,
	BACNET_PTP_FRM_DATA_ACK_1_XON  = 0x07,
	BACNET_PTP_FRM_DATA_NAK_0_XOFF = 0x08,
	BACNET_PTP_FRM_DATA_NAK_1_XOFF = 0x09,
	BACNET_PTP_FRM_DATA_NAK_0_XON  = 0x0a,
	BACNET_PTP_FRM_DATA_NAK_1_XON  = 0x0b,
	BACNET_PTP_FRM_CONNECT_REQ     = 0x0c,
	BACNET_PTP_FRM_CONNECT_RESP    = 0x0d,
	BACNET_PTP_FRM_DISCONNECT_REQ  = 0x0e,
	BACNET_PTP_FRM_DISCONNECT_RESP = 0x0f,
	BACNET_PTP_FRM_TEST_REQ        = 0x14,
	BACNET_PTP_FRM_TEST_RESP       = 0x15
};

/* PTP Connection State Machine states */
enum {
	BACNET_PTP_DISCONNECTED = 0,
	BACNET_PTP_DISCONNECTING,
	BACNET_PTP_INBOUND,
	BACNET_PTP_OUTBOUND,
	BACNET_PTP_CONNECTED
};

#define DLE  0x10
#define XON  0x11
#define XOFF 0x13

static const uint8_t bacnet_crc8_lut[256] = {
	0x00, 0xfe, 0xff, 0x01, 0xfd, 0x03, 0x02, 0xfc,
	0xf9, 0x07, 0x06, 0xf8, 0x04, 0xfa, 0xfb, 0x05,
	0xf1, 0x0f, 0x0e, 0xf0, 0x0c, 0xf2, 0xf3, 0x0d,
	0x08, 0xf6, 0xf7, 0x09, 0xf5, 0x0b, 0x0a, 0xf4,
	0xe1, 0x1f, 0x1e, 0xe0, 0x1c, 0xe2, 0xe3, 0x1d,
	0x18, 0xe6, 0xe7, 0x19, 0xe5, 0x1b, 0x1a, 0xe4,
	0x10, 0xee, 0xef, 0x11, 0xed, 0x13, 0x12, 0xec,
	0xe9, 0x17, 0x16, 0xe8, 0x14, 0xea, 0xeb, 0x15,
	0xc1, 0x3f, 0x3e, 0xc0, 0x3c, 0xc2, 0xc3, 0x3d,
	0x38, 0xc6, 0xc7, 0x39, 0xc5, 0x3b, 0x3a, 0xc4,
	0x30, 0xce, 0xcf, 0x31, 0xcd, 0x33, 0x32, 0xcc,
	0xc9, 0x37, 0x36, 0xc8, 0x34, 0xca, 0xcb, 0x35,
	0x20, 0xde, 0xdf, 0x21, 0xdd, 0x23, 0x22, 0xdc,
	0xd9, 0x27, 0x26, 0xd8, 0x24, 0xda, 0xdb, 0x25,
	0xd1, 0x2f, 0x2e, 0xd0, 0x2c, 0xd2, 0xd3, 0x2d,
	0x28, 0xd6, 0xd7, 0x29, 0xd5, 0x2b, 0x2a, 0xd4,
	0x81, 0x7f, 0x7e, 0x80, 0x7c, 0x82, 0x83, 0x7d,
	0x78, 0x86, 0x87, 0x79, 0x85, 0x7b, 0x7a, 0x84,
	0x70, 0x8e, 0x8f, 0x71, 0x8d, 0x73, 0x72, 0x8c,
	0x89, 0x77, 0x76, 0x88, 0x74, 0x8a, 0x8b, 0x75,
	0x60, 0x9e, 0x9f, 0x61, 0x9d, 0x63, 0x62, 0x9c,
	0x99, 0x67, 0x66, 0x98, 0x64, 0x9a, 0x9b, 0x65,
	0x91, 0x6f, 0x6e, 0x90, 0x6c, 0x92, 0x93, 0x6d,
	0x68, 0x96, 0x97, 0x69, 0x95, 0x6b, 0x6a, 0x94,
	0x40, 0xbe, 0xbf, 0x41, 0xbd, 0x43, 0x42, 0xbc,
	0xb9, 0x47, 0x46, 0xb8, 0x44, 0xba, 0xbb, 0x45,
	0xb1, 0x4f, 0x4e, 0xb0, 0x4c, 0xb2, 0xb3, 0x4d,
	0x48, 0xb6, 0xb7, 0x49, 0xb5, 0x4b, 0x4a, 0xb4,
	0xa1, 0x5f, 0x5e, 0xa0, 0x5c, 0xa2, 0xa3, 0x5d,
	0x58, 0xa6, 0xa7, 0x59, 0xa5, 0x5b, 0x5a, 0xa4,
	0x50, 0xae, 0xaf, 0x51, 0xad, 0x53, 0x52, 0xac,
	0xa9, 0x57, 0x56, 0xa8, 0x54, 0xaa, 0xab, 0x55
};

unsigned int __bacnet_crc8(unsigned int crc, int c)
{
	return bacnet_crc8_lut[crc ^ c];
}

unsigned int bacnet_crc8(unsigned int crc, const void * buf, int len)
{
	uint8_t * cp = (uint8_t *)buf;
	int i;

	for (i = 0; i < len; ++i )
		crc = bacnet_crc8_lut[crc ^ cp[i]];

	return crc;
}

static const uint16_t bacnet_crc16_lut[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

unsigned int bacnet_crc16(unsigned int crc, const void * buf, int len)
{
	uint8_t * cp = (uint8_t *)buf;
	int i;

	for (i = 0; i < len; ++i)
		crc = (crc >> 8) ^ bacnet_crc16_lut[(crc & 0xff) ^ cp[i]];

	return crc;
}

#define TX_IDLE_HBEAT_TMO_MS 15000
#define RX_IDLE_DISC_TMO_MS 30000
#define CONN_RXMT_TIME_MS  5000
#define DISC_RXMT_TIME_MS  5000
#define NPDU_RXMT_TIME_MS  1000
#define DCC_TIME_MS        1000

struct bacnet_ptp_hdr {
	uint16_t preamble;
	uint8_t type;
	uint16_t len;
	uint8_t crc;
};

struct bacnet_ptp_frm {
	struct bacnet_ptp_hdr hdr;
	uint8_t data[];
};

#define PTP_LINK_RXMT_MAX 3


static void bacnet_ptp_lnk_rx_rst(struct bacnet_ptp_lnk * lnk)
{
	lnk->rx.off = 0;
	lnk->rx.cnt = 0;
	lnk->rx.dle = false;
}

#if 0
int bacnet_ptp_frame_decode(uint8_t * frm, int frm_len)
{
	static uint8_t frm_buf[512];
	uint8_t * buf = frm_buf;
	bool dle = false;
	int off = 0;
	int cnt = 0;
	uint8_t * cp;
	int len;
	int pdu_len;
	
	for (;;) {
		uint8_t crc;

		/* move remaining octets to the beginning of the buffer */
		if (off) {
			DCC_LOG2(LOG_INFO, "shift off=%d cnt=%d", off, cnt);
			cnt = cnt - off;
			memcpy(buf, &buf[off], cnt);
		} else
			cnt = 0;

		off = 0;
		cp = &buf[cnt];

		/* receive a packet header */
		while (cnt < 5) {
			int i;
			int j;
			int n;
			int rem = BACNET_PTP_MTU - cnt;

			memcpy(buf, frm, frm_len);
			n = frm_len;
			if (n <= 0) {
				DCC_LOG1(LOG_INFO, "serdrv_recv=%d", n);
				return n;
			}

			for (i = 0, j = 0; i < n; ++i) {
				int c = cp[i];

				/* byte destuff */
				if (dle) {
					DCC_LOG1(LOG_INFO, "DLE %02x", c);
					c &= 0x7f;
					dle = false;
				} else if (c == DLE) {
					dle = true;
					continue;
				} 

				cp[j++] = c;
				DCC_LOG1(LOG_INFO, "%02x", c);
				dle = false;
			}

			cp += j;
			cnt += j;
		}

		if (buf[0] != 0x55) {
			DCC_LOG(LOG_WARNING, "frame error 1");
			off = 1;
			continue;
		}
		if (buf[1] != 0xff) {
			DCC_LOG(LOG_WARNING, "frame error 2");
			off = 2;
			continue;
		}

		crc = ~bacnet_crc8(0xff, &buf[2], 3);

		if (buf[5] != crc) {
			DCC_LOG2(LOG_WARNING, "CRC error: %02x != %02x", buf[5], crc);
			off = 3;
			continue;
		}

		break;
	}

	len = (buf[3] << 8) + buf[4];
	DCC_LOG2(LOG_TRACE, "cnt=%d len=%d", cnt, len);

	if (len > 0) {
		uint16_t crc;
		uint16_t chk;
		int i;

		chk = (buf[len + 7] << 8) + buf[len + 6];
		crc = ~bacnet_crc16(0xffff, &buf[6], len);
		if (crc != chk) {
			DCC_LOG2(LOG_WARNING, "Data CRC error %04x != %04x", crc, chk);
			return -1;
		}
#if 0
		{
			int i;
			printf("RX:");
			for (i = 0; i < len + 8; ++i)
				printf(" %02x", buf[i]);
			printf("\n");
		}
#endif
		pdu_len = len;
		off = len + 8;
	} else {
#if 0
		{
			int i;
			printf("RX:");
			for (i = 0; i < 6; ++i)
				printf(" %02x", buf[i]);
			printf("\n");
		}
#endif
		pdu_len = 0;
		off = 6;
	}


	switch (buf[2]) {
	case BACNET_PTP_FRM_HEARTBEAT_XOFF:
		DCC_LOG(LOG_TRACE, "HEARTBEAT_XOFF");
		break;
	case BACNET_PTP_FRM_HEARTBEAT_XON:
		DCC_LOG(LOG_TRACE, "HEARTBEAT_XON");
		break;
	case BACNET_PTP_FRM_DATA_0:
		DCC_LOG1(LOG_TRACE, "DATA_0(%d)", pdu_len);
		break;
	case BACNET_PTP_FRM_DATA_1:
		DCC_LOG1(LOG_TRACE, "DATA_1(%d)", pdu_len);
		break;
	case BACNET_PTP_FRM_DATA_ACK_0_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_ACK_0_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_ACK_1_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_ACK_1_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_ACK_0_XON:
		DCC_LOG(LOG_TRACE, "DATA_ACK_0_XON");
		break;
	case BACNET_PTP_FRM_DATA_ACK_1_XON:
		DCC_LOG(LOG_TRACE, "DATA_ACK_1_XON");
		break;
	case BACNET_PTP_FRM_DATA_NAK_0_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_NAK_0_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_NAK_1_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_NAK_1_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_NAK_0_XON:
		DCC_LOG(LOG_TRACE, "DATA_NAK_0_XON");
		break;
	case BACNET_PTP_FRM_DATA_NAK_1_XON:
		DCC_LOG(LOG_TRACE, "DATA_NAK_1_XON");
		break;
	case BACNET_PTP_FRM_CONNECT_REQ:
		DCC_LOG(LOG_TRACE, "CONNECT_REQ");
		break;
	case BACNET_PTP_FRM_CONNECT_RESP:
		DCC_LOG(LOG_TRACE, "CONNECT_RESP");
		break;
	case BACNET_PTP_FRM_DISCONNECT_REQ:
		DCC_LOG(LOG_TRACE, "DISCONNECT_REQ");
		break;
	case BACNET_PTP_FRM_DISCONNECT_RESP:
		DCC_LOG(LOG_TRACE, "DISCONNECT_RESP");
		break;
	case BACNET_PTP_FRM_TEST_REQ:
		DCC_LOG(LOG_TRACE, "TEST_REQ");
		break;
	case BACNET_PTP_FRM_TEST_RESP:
		DCC_LOG(LOG_TRACE, "TEST_RESP");
		break;
	}

	return buf[2]; /* Frame Type */
}

#endif


int bacnet_ptp_frame_recv(struct bacnet_ptp_lnk * lnk, unsigned int tmo)
{
	bool dle = lnk->rx.dle;
	int off = lnk->rx.off;
	int cnt = lnk->rx.cnt;
	uint8_t * buf = lnk->rx.buf;
	uint8_t * cp;
	int len;
	
	for (;;) {
		uint8_t crc;

		/* move remaining octets to the beginning of the buffer */
		if (off) {
			DCC_LOG2(LOG_INFO, "shift off=%d cnt=%d", off, cnt);
			cnt = cnt - off;
			memcpy(buf, &buf[off], cnt);
		} else
			cnt = 0;

		off = 0;
		cp = &buf[cnt];

		/* receive a packet header */
		while (cnt < 5) {
			int i;
			int j;
			int n;
			int rem = BACNET_PTP_MTU - cnt;

			n = serial_recv(lnk->dev, cp, rem, tmo);
			if (n <= 0) {
				DCC_LOG1(LOG_INFO, "serdrv_recv=%d", n);
				bacnet_ptp_lnk_rx_rst(lnk);
				return n;
			}

			for (i = 0, j = 0; i < n; ++i) {
				int c = cp[i];

				/* byte destuff */
				if (dle) {
					DCC_LOG1(LOG_INFO, "DLE %02x", c);
					c &= 0x7f;
					dle = false;
				} else if (c == DLE) {
					dle = true;
					continue;
				} 

				cp[j++] = c;
				DCC_LOG1(LOG_INFO, "%02x", c);
				dle = false;
			}

			cp += j;
			cnt += j;
		}

		if (buf[0] != 0x55) {
			DCC_LOG(LOG_WARNING, "frame error 1");
			off = 1;
			continue;
		}
		if (buf[1] != 0xff) {
			DCC_LOG(LOG_WARNING, "frame error 2");
			off = 2;
			continue;
		}

		crc = ~bacnet_crc8(0xff, &buf[2], 3);

		if (buf[5] != crc) {
			DCC_LOG2(LOG_WARNING, "CRC error: %02x != %02x", buf[5], crc);
			off = 3;
			continue;
		}

		break;
	}

	len = (buf[3] << 8) + buf[4];
	DCC_LOG2(LOG_INFO, "cnt=%d len=%d", cnt, len);

	if (len > 0) {
		uint16_t crc;
		uint16_t chk;
		int i;

		/* receive the frame data */
		while (cnt < len + 8) {
			int j;
			int n;
			int rem = BACNET_PTP_MTU - cnt;

			n = serial_recv(lnk->dev, cp, rem, tmo);
			if (n <= 0) {
				DCC_LOG1(LOG_WARNING, "serdrv_recv=%d", n);
				bacnet_ptp_lnk_rx_rst(lnk);
				return n;
			}

			for (i = 0, j = 0; i < n; ++i) {
				int c = cp[i];

				/* byte destuff */
				if (dle) {
					DCC_LOG1(LOG_INFO, "DLE %02x", c);
					c &= 0x7f;
					dle = false;
				} else if (c == DLE) {
					dle = true;
					continue;
				} 

				cp[j++] = c;
				DCC_LOG1(LOG_INFO, "%02x", c);
			}
			cp += j;
			cnt += j;
		}

		chk = (buf[len + 7] << 8) + buf[len + 6];
		crc = ~bacnet_crc16(0xffff, &buf[6], len);
		if (crc != chk) {
			DCC_LOG2(LOG_WARNING, "Data CRC error %04x != %04x", crc, chk);
			bacnet_ptp_lnk_rx_rst(lnk);
			return -1;
		}

#if 0
		{
			int i;
			printf("RX:");
			for (i = 0; i < len + 8; ++i)
				printf(" %02x", buf[i]);
			printf("\n");
		}
#endif
		lnk->rx.pdu_len = len;
		off = len + 8;
	} else {
#if 0
		{
			int i;
			printf("RX:");
			for (i = 0; i < 6; ++i)
				printf(" %02x", buf[i]);
			printf("\n");
		}
#endif
		lnk->rx.pdu_len = 0;
		off = 6;
	}

	lnk->rx.dle = dle;
	lnk->rx.off = off;
	lnk->rx.cnt = cnt;

	switch (buf[2]) {
	case BACNET_PTP_FRM_HEARTBEAT_XOFF:
		DCC_LOG(LOG_TRACE, "HEARTBEAT_XOFF");
		break;
	case BACNET_PTP_FRM_HEARTBEAT_XON:
		DCC_LOG(LOG_TRACE, "HEARTBEAT_XON");
		break;
	case BACNET_PTP_FRM_DATA_0:
		DCC_LOG1(LOG_TRACE, "DATA_0(%d)", len);
		break;
	case BACNET_PTP_FRM_DATA_1:
		DCC_LOG1(LOG_TRACE, "DATA_1(%d)", len);
		break;
	case BACNET_PTP_FRM_DATA_ACK_0_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_ACK_0_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_ACK_1_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_ACK_1_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_ACK_0_XON:
		DCC_LOG(LOG_TRACE, "DATA_ACK_0_XON");
		break;
	case BACNET_PTP_FRM_DATA_ACK_1_XON:
		DCC_LOG(LOG_TRACE, "DATA_ACK_1_XON");
		break;
	case BACNET_PTP_FRM_DATA_NAK_0_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_NAK_0_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_NAK_1_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_NAK_1_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_NAK_0_XON:
		DCC_LOG(LOG_TRACE, "DATA_NAK_0_XON");
		break;
	case BACNET_PTP_FRM_DATA_NAK_1_XON:
		DCC_LOG(LOG_TRACE, "DATA_NAK_1_XON");
		break;
	case BACNET_PTP_FRM_CONNECT_REQ:
		DCC_LOG(LOG_TRACE, "CONNECT_REQ");
		break;
	case BACNET_PTP_FRM_CONNECT_RESP:
		DCC_LOG(LOG_TRACE, "CONNECT_RESP");
		break;
	case BACNET_PTP_FRM_DISCONNECT_REQ:
		DCC_LOG(LOG_TRACE, "DISCONNECT_REQ");
		break;
	case BACNET_PTP_FRM_DISCONNECT_RESP:
		DCC_LOG(LOG_TRACE, "DISCONNECT_RESP");
		break;
	case BACNET_PTP_FRM_TEST_REQ:
		DCC_LOG(LOG_TRACE, "TEST_REQ");
		break;
	case BACNET_PTP_FRM_TEST_RESP:
		DCC_LOG(LOG_TRACE, "TEST_RESP");
		break;
	}

	return buf[2]; /* Frame Type */
}

int bacnet_ptp_frame_send(struct bacnet_ptp_lnk * lnk, int typ, 
						uint8_t * pdu, unsigned int len)
{
	uint8_t hdr[6];
	uint8_t buf[9 + 2 * len];
	int cnt;
	int i;
	int j;
	int c;

	/* reset TX idle timer */
	lnk->tx.idle_tmr = lnk->clk + TX_IDLE_HBEAT_TMO_MS; 

	hdr[0] = 0x55;
	hdr[1] = 0xff;
	hdr[2] = typ;
	hdr[3] = (len >> 8) & 0xff;
	hdr[4] = len & 0xff;
	hdr[5] = ~bacnet_crc8(0xff, &hdr[2], 3);

	/* encode header */
	for (i = 0, j = 0; i < 6; ++i) {
		c = hdr[i];
		if ((c == DLE) || (c == XON) || (c == XOFF)) {
			buf[j++] = DLE;
			c |= 0x80;
		}
		buf[j++] = c;
	}
	cnt = j;

	if (len > 0) {
		uint16_t crc;

		crc = ~bacnet_crc16(0xffff, pdu, len);
		for (i = 0, j = cnt; i < len; ++i) {
			c = pdu[i];
			if ((c == DLE) || (c == XON) || (c == XOFF)) {
				buf[j++] = DLE;
				c |= 0x80;
			}
			buf[j++] = c;
		}
		c = (crc & 0xff);
		if ((c == DLE) || (c == XON) || (c == XOFF)) {
			buf[j++] = DLE;
			c |= 0x80;
		}
		buf[j++] = c;

		c = (crc >> 8) & 0xff;
		if ((c == DLE) || (c == XON) || (c == XOFF)) {
			buf[j++] = DLE;
			c |= 0x80;
		}
		buf[j++] = c;

		cnt = j;
	} 

#if 0
	printf("TX:");
	for (i = 0; i < cnt; ++i) {
		c = buf[i];
		printf(" %02x", c);
	}
	printf("\n");
#endif

	switch (typ) {
	case BACNET_PTP_FRM_HEARTBEAT_XOFF:
		DCC_LOG(LOG_TRACE, "HEARTBEAT_XOFF");
		break;
	case BACNET_PTP_FRM_HEARTBEAT_XON:
		DCC_LOG(LOG_TRACE, "HEARTBEAT_XON");
		break;
	case BACNET_PTP_FRM_DATA_0:
		DCC_LOG1(LOG_TRACE, "DATA_0(%d)", len);
		break;
	case BACNET_PTP_FRM_DATA_1:
		DCC_LOG1(LOG_TRACE, "DATA_1(%d)", len);
		break;
	case BACNET_PTP_FRM_DATA_ACK_0_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_ACK_0_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_ACK_1_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_ACK_1_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_ACK_0_XON:
		DCC_LOG(LOG_TRACE, "DATA_ACK_0_XON");
		break;
	case BACNET_PTP_FRM_DATA_ACK_1_XON:
		DCC_LOG(LOG_TRACE, "DATA_ACK_1_XON");
		break;
	case BACNET_PTP_FRM_DATA_NAK_0_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_NAK_0_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_NAK_1_XOFF:
		DCC_LOG(LOG_TRACE, "DATA_NAK_1_XOFF");
		break;
	case BACNET_PTP_FRM_DATA_NAK_0_XON:
		DCC_LOG(LOG_TRACE, "DATA_NAK_0_XON");
		break;
	case BACNET_PTP_FRM_DATA_NAK_1_XON:
		DCC_LOG(LOG_TRACE, "DATA_NAK_1_XON");
		break;
	case BACNET_PTP_FRM_CONNECT_REQ:
		DCC_LOG(LOG_TRACE, "CONNECT_REQ");
		break;
	case BACNET_PTP_FRM_CONNECT_RESP:
		DCC_LOG(LOG_TRACE, "CONNECT_RESP");
		break;
	case BACNET_PTP_FRM_DISCONNECT_REQ:
		DCC_LOG(LOG_TRACE, "DISCONNECT_REQ");
		break;
	case BACNET_PTP_FRM_DISCONNECT_RESP:
		DCC_LOG(LOG_TRACE, "DISCONNECT_RESP");
		break;
	case BACNET_PTP_FRM_TEST_REQ:
		DCC_LOG(LOG_TRACE, "TEST_REQ");
		break;
	case BACNET_PTP_FRM_TEST_RESP:
		DCC_LOG(LOG_TRACE, "TEST_RESP");
		break;
	}

	return serial_send(lnk->dev, buf, cnt);
}

int __attribute__((noreturn)) bacnet_ptp_task(struct bacnet_ptp_lnk * lnk)
{
	uint8_t * pdu;
	int pdu_len;
	uint32_t clk;
	int typ;
	int len;

wait_connect:
	lnk->tx.xon = false;
	lnk->tx.seq = 1;
	lnk->tx.len = 0;
	thinkos_flag_give(lnk->rx.flag);

	lnk->rx.xon = false;
	lnk->rx.seq = 1;
	lnk->rx.pdu_len = 0;
	thinkos_flag_give(lnk->tx.flag);

	while (lnk->state == BACNET_PTP_DISCONNECTED) {
		/* In this state, the device waits for the network layer to 
		   initiate a PTP data link connection or for the physical 
		   layer to indicate the occurrence of a physical 
		   layer connection. */
		thinkos_sleep(100);
		clk = thinkos_clock();
		lnk->clk = clk;
	}

	lnk->dcc_tmr = lnk->clk + DCC_TIME_MS; 
	lnk->rx.idle_tmr = lnk->clk + RX_IDLE_DISC_TMO_MS; 

again:
	clk = lnk->clk;
	if (((int32_t)(lnk->dcc_tmr - clk)) < 0) {
		lnk->dcc_tmr = clk + DCC_TIME_MS; 
		DCC_LOG(LOG_INFO, "DCC timer");
		dcc_timer_seconds((DCC_TIME_MS + 500) / 1000);
	}

	typ = bacnet_ptp_frame_recv(lnk, 100);
	clk = thinkos_clock();
	lnk->clk = clk;

	if (typ > 0) {
#if 0
		int len = *pdu_len;
		if (len) {
			uint8_t * cp = *pdu;
			int i;
			printf("RX PDU: ");
			for (i = 0; i < len; ++i)
				printf(" %02x", cp[i]);
			printf("\n");
		}
#endif
		/* Reset IDLE timer */
		lnk->rx.idle_tmr = clk + RX_IDLE_DISC_TMO_MS; 
	} 
	
	switch (lnk->state) {
	case BACNET_PTP_DISCONNECTED:
		DCC_LOG(LOG_TRACE, "[DISCONNECTED]");
		goto wait_connect;

	case BACNET_PTP_INBOUND:
		/* In this state, the Connection State Machine has 
		   recognized that the calling device wishes to establish a 
		   BACnet connection, and the local device is waiting for a 
		   Connect Response frame from the calling device. */
		if (typ == BACNET_PTP_FRM_CONNECT_RESP) {
			lnk->state = BACNET_PTP_CONNECTED;
			DCC_LOG(LOG_TRACE, "[CONNECTED]");
			bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_HEARTBEAT_XON,
								NULL, 0);
			/* start the cyclic 1 second timer for DCC */
//			timer_interval_start_seconds(&DCC_Timer, DCC_CYCLE_SECONDS);
			break;
		} 

		if (typ == BACNET_PTP_FRM_DISCONNECT_REQ) {
			bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_DISCONNECT_RESP,
								NULL, 0);
			lnk->state = BACNET_PTP_DISCONNECTED;
			DCC_LOG(LOG_TRACE, "[DISCONNECTED]");
			goto wait_connect;
		}

		if (typ == THINKOS_ETIMEDOUT) {
			if (((int32_t)(lnk->tx.rxmt_tmr - clk)) < 0) {
				if (++lnk->tx.rxmt_cnt < PTP_LINK_RXMT_MAX) {
					lnk->tx.rxmt_tmr = clk + CONN_RXMT_TIME_MS; 
					bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_CONNECT_REQ, 
										  NULL, 0);
					DCC_LOG(LOG_TRACE, "[INBOUND]");
				} else {
					lnk->state = BACNET_PTP_DISCONNECTED;
					DCC_LOG(LOG_TRACE, "[DISCONNECTED]");
					goto wait_connect;
				} 
			}
		}
		break;

	case BACNET_PTP_OUTBOUND:
		/* FIXME: not implemented ... */
		break;

	case BACNET_PTP_CONNECTED:
		switch (typ) {
		case BACNET_PTP_FRM_HEARTBEAT_XOFF:
			if (lnk->tx.xon) {
				DCC_LOG(LOG_TRACE, "[XOFF]");
				lnk->tx.xon = false;
			}
			break;

		case BACNET_PTP_FRM_HEARTBEAT_XON:
			if (!lnk->tx.xon) {
				DCC_LOG(LOG_TRACE, "[XON]");
				lnk->tx.xon = true;
			}
			break;

		case BACNET_PTP_FRM_DATA_0:
			/* FIXME: check for correct sequence number */
			bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_DATA_ACK_0_XON, NULL, 0);
			thinkos_flag_give(lnk->rx.flag);
			break;

		case BACNET_PTP_FRM_DATA_1:
			/* FIXME: check for correct sequence number */
			bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_DATA_ACK_1_XON, NULL, 0);
			thinkos_flag_give(lnk->rx.flag);
			break;

		case BACNET_PTP_FRM_DATA_ACK_0_XOFF:
			break;
		case BACNET_PTP_FRM_DATA_ACK_1_XOFF:
			break;

		case BACNET_PTP_FRM_DATA_ACK_0_XON:
			if ((lnk->tx.seq & 1) == 0) {
				lnk->tx.seq++;
				lnk->tx.len = 0;
				thinkos_flag_give(lnk->tx.flag);
			}
			break;

		case BACNET_PTP_FRM_DATA_ACK_1_XON:
			if ((lnk->tx.seq & 1) == 1) {
				lnk->tx.seq++;
				lnk->tx.len = 0;
				thinkos_flag_give(lnk->tx.flag);
			}	
			break;

		case BACNET_PTP_FRM_DATA_NAK_0_XOFF:
			break;

		case BACNET_PTP_FRM_DATA_NAK_1_XOFF:
			break;

		case BACNET_PTP_FRM_DATA_NAK_0_XON:
			if ((lnk->tx.seq & 1) == 0)
				bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_DATA_0,
									  lnk->tx.buf, lnk->tx.len);
			break;

		case BACNET_PTP_FRM_DATA_NAK_1_XON:
			if ((lnk->tx.seq & 1) == 1)
				bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_DATA_1,
									  lnk->tx.buf, lnk->tx.len);
			break;

		case BACNET_PTP_FRM_CONNECT_REQ:
			bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_CONNECT_RESP, NULL, 0);
			break;

		case BACNET_PTP_FRM_CONNECT_RESP:
			DCC_LOG(LOG_TRACE, "???");
			break;

		case BACNET_PTP_FRM_DISCONNECT_REQ:
			bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_DISCONNECT_RESP,
								NULL, 0);
			lnk->state = BACNET_PTP_DISCONNECTED;
			DCC_LOG(LOG_TRACE, "[DISCONNECTED]");
			goto wait_connect;

		case BACNET_PTP_FRM_DISCONNECT_RESP:
			lnk->enabled = false;
			break;

		case BACNET_PTP_FRM_TEST_REQ:
			break;

		case BACNET_PTP_FRM_TEST_RESP:
			break;

		case THINKOS_ETIMEDOUT:
			if (lnk->tx.len) {
				/* data pending on the TX buffer */
				if (((int32_t)(lnk->tx.rxmt_tmr - clk)) < 0) {
					if (++lnk->tx.rxmt_cnt < PTP_LINK_RXMT_MAX) {
						/* restart retransmission timer */
						lnk->tx.rxmt_tmr = clk + NPDU_RXMT_TIME_MS; 
						if (lnk->tx.xon) {
							DCC_LOG(LOG_TRACE, "retransmitting...");
							bacnet_ptp_frame_send(lnk, (lnk->tx.seq & 1) ? 
												  BACNET_PTP_FRM_DATA_1 :
												  BACNET_PTP_FRM_DATA_0,
												  lnk->tx.buf, lnk->tx.len);
						}
					} else {
						DCC_LOG(LOG_TRACE, "too many retries giving up...");
						lnk->tx.len = 0;
						thinkos_flag_give(lnk->tx.flag);
					}
				} 
			}
			if (((int32_t)(lnk->rx.idle_tmr - clk)) < 0) {
				DCC_LOG(LOG_TRACE, "RX IDLE timeout");
				uint8_t code = 0x01;
				/* IDLE timeout */
				lnk->tx.rxmt_tmr = clk + DISC_RXMT_TIME_MS; 
				lnk->tx.rxmt_cnt = 0;
				bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_DISCONNECT_REQ,
									  &code, 1);

				lnk->state = BACNET_PTP_DISCONNECTING;
				DCC_LOG(LOG_TRACE, "[DISCONNECTING]");
			}
			if (((int32_t)(lnk->tx.idle_tmr - clk)) < 0) {
				DCC_LOG(LOG_INFO, "TX IDLE timeout");
				bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_HEARTBEAT_XON, 
									  NULL, 0);
			}
			break;
		}
		break;

	case BACNET_PTP_DISCONNECTING:

		if (typ == BACNET_PTP_FRM_DISCONNECT_RESP) {
			lnk->state = BACNET_PTP_DISCONNECTED;
			DCC_LOG(LOG_TRACE, "[DISCONNECTED]");
			goto wait_connect;
		}

		if (typ == THINKOS_ETIMEDOUT) {
			if (((int32_t)(lnk->tx.rxmt_tmr - clk)) < 0) {
				if (++lnk->tx.rxmt_cnt < PTP_LINK_RXMT_MAX) {
					uint8_t code = 0x02;
					bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_DISCONNECT_REQ,
										  &code, 1);
					DCC_LOG(LOG_TRACE, "[DISCONNECTING]");
					lnk->tx.rxmt_tmr = clk + DISC_RXMT_TIME_MS; 
				} else {
					lnk->state = BACNET_PTP_DISCONNECTED;
					DCC_LOG(LOG_TRACE, "[DISCONNECTED]");
					goto wait_connect;
				} 
			}
		}

		break;
	}

	goto again;
}

int bacnet_ptp_lnk_init(struct bacnet_ptp_lnk * lnk, struct serial_dev * dev)
{
	uint32_t clk;

	memset(lnk, 0, sizeof(struct bacnet_ptp_lnk));

	lnk->dev = dev;
	lnk->enabled = true;
	lnk->state = BACNET_PTP_DISCONNECTED;
	lnk->rx.seq = 0;
	lnk->tx.seq = 0;
	lnk->tx.xon = false;
	lnk->rx.xon = false;

	DCC_LOG(LOG_TRACE, "[DISCONNECTED]");
	bacnet_ptp_lnk_rx_rst(lnk);
	lnk->rx.flag = thinkos_flag_alloc();
	lnk->tx.flag = thinkos_flag_alloc();

	DCC_LOG2(LOG_TRACE, "tx.flag=%d rx.flag=%d", lnk->rx.flag, lnk->tx.flag);

	clk = thinkos_clock();
	lnk->clk = clk;
	lnk->dcc_tmr = clk + DCC_TIME_MS; 
	lnk->rx.idle_tmr = clk + RX_IDLE_DISC_TMO_MS; 

	return 0;
}

int bacnet_ptp_recv(struct bacnet_ptp_lnk * lnk, uint8_t pdu[], 
					unsigned int max)
{
	int pdu_len;

	for(;;) {
		if (thinkos_flag_take(lnk->rx.flag) < 0) {
			DCC_LOG(LOG_ERROR, "thinkos_flag_take() failed!");
			abort();
		}

		if (lnk->state == BACNET_PTP_DISCONNECTED) {
			DCC_LOG(LOG_TRACE, "Disconnected. Bailing out...");
			return -EAGAIN;
		}

		/* check for a slot in the xmit queue ... */
		if (lnk->rx.pdu_len) 
			break;
	};

	pdu_len = lnk->rx.pdu_len;
	memcpy(pdu, lnk->rx.pdu, pdu_len);

	lnk->rx.pdu_len = 0;

	return pdu_len;
}

int bacnet_ptp_send(struct bacnet_ptp_lnk * lnk, const uint8_t pdu[], 
					unsigned int len)
{
	uint32_t clk;

	for(;;) {
		if (thinkos_flag_take(lnk->tx.flag) < 0) {
			DCC_LOG(LOG_ERROR, "thinkos_flag_take() failed!");
			abort();
		}

		if (lnk->state != BACNET_PTP_CONNECTED)
			return -EAGAIN;

		/* check for a slot in the xmit queue ... */
		if (lnk->tx.len == 0) 
			break;
	};

	/* update the link layer event clock */
	clk = thinkos_clock();
	lnk->clk = clk;

	/* restart the retransmission timer */
	lnk->tx.rxmt_tmr = clk + NPDU_RXMT_TIME_MS; 
	lnk->tx.rxmt_cnt = 0;

	/* insert frame in the transmission queue ...  */
	memcpy(lnk->tx.buf, pdu, len);
	lnk->tx.len = len;

	if (lnk->tx.xon) {
		DCC_LOG1(LOG_INFO, "sending now (%d)...", len);
		/* if the remote side is reception is on send the frame now! */
		bacnet_ptp_frame_send(lnk, (lnk->tx.seq & 1) ? 
							  BACNET_PTP_FRM_DATA_1 :
							  BACNET_PTP_FRM_DATA_0,
							  lnk->tx.buf, lnk->tx.len);
	}

	return len;
}


struct bacnet_ptp_lnk * bacnet_ptp_outbound(struct bacnet_ptp_lnk * lnk)
{
	DCC_LOG(LOG_TRACE, "Starting BACnet PtP Data Link");

	/* 10.4.9.1 DISCONNECTED, ConnectOutbound 
	If a DL-CONNECT.request is received,
	then establish a physical connection; transmit the "BACnet<CR>" trigger 
	sequence; set RetryCount to zero; set
	ResponseTimer to zero; and enter the OUTBOUND state.
	 */
	serial_send(lnk->dev, "BACnet\r", 7);

	lnk->state = BACNET_PTP_OUTBOUND;
	DCC_LOG(LOG_TRACE, "[OUTBOUND]");

	return 0;
}

int bacnet_ptp_inbound(struct bacnet_ptp_lnk * lnk)
{

	DCC_LOG(LOG_TRACE, "Starting BACnet PtP Data Link");

	lnk->tx.rxmt_tmr = lnk->clk + CONN_RXMT_TIME_MS; 
	lnk->tx.rxmt_cnt = 0;
	bacnet_ptp_frame_send(lnk, BACNET_PTP_FRM_CONNECT_REQ, NULL, 0);

	/* 10.4.9.1 DISCONNECTED, ConnectInbound */
	lnk->state = BACNET_PTP_INBOUND;
	DCC_LOG(LOG_TRACE, "[INBOUND]");

	return 0;
}

#if 0

uint32_t bacnet_ptp_stack[256];

const struct thinkos_thread_inf bacnet_ptp_inf = {
	.stack_ptr = bacnet_ptp_stack, 
	.stack_size = sizeof(bacnet_ptp_stack), 
	.priority = 32,
	.thread_id = 7, 
	.paused = 0,
	.tag = "BN PTP"
};

void bacnet_ptp_task_init(struct bacnet_ptp_lnk * lnk)
{
	DCC_LOG(LOG_TRACE, "5. BACnet PtP Task...");
	thinkos_thread_create_inf((void *)bacnet_ptp_task, (void *)lnk, 
							  &bacnet_ptp_inf);
}

#endif

