/* 
 * Copyright(C) 2013 Robinson Mittmann. All Rights Reserved.
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
 * @file stm32f-usb_fs_dev.c
 * @brief STM32F USB Full Speed Device Driver
 * @author Robinson Mittmann <bobmittmann@gmail.com>
 */ 

#include <sys/stm32f.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arch/cortex-m3.h>
#include <sys/delay.h>
#include <sys/usb-dev.h>
#include <sys/param.h>

#include <sys/dcclog.h>

#if defined(STM32F103) || defined(STM32F30X)

/* Endpoint state */
typedef enum {
	EP_DISABLED,
	EP_IDLE,
	EP_STALLED,
	EP_SETUP,
	EP_IN_DATA,
	EP_IN_DATA_LAST,
	EP_WAIT_STATUS_IN,
	EP_WAIT_STATUS_OUT,
	EP_OUT_DATA,
	EP_OUT_DATA_LAST,
} ep_state_t;

/* Endpoint control structure */
struct stm32f_usb_ep {
	uint16_t mxpktsz; /* Maximum packet size for this EP */
	ep_state_t state; /* Current EP state */

	uint16_t xfr_rem; /* Bytes pendig in the transfer buffer */
	uint16_t xfr_buf_len; /* Length of the transfer buffer */
	uint8_t * xfr_buf; /* Transfer buffer pointer */
	uint8_t * xfr_ptr; /* Pointer to the next transfer */

	/* Endpoint callback */
	union {
		usb_class_on_ep_ev_t on_ev;
		usb_class_on_ep_in_t on_in;
		usb_class_on_ep_out_t on_out;
		usb_class_on_ep_setup_t on_setup;
	};
};

#define USB_DRIVER_EP_MAX 8

//#define DEBUG_DBLBUF

/* USB Driver */
struct stm32f_usb_drv {
	struct stm32f_usb_ep ep[USB_DRIVER_EP_MAX];
	usb_class_t * cl;
	const struct usb_class_events * ev;
	struct usb_request req;
#ifdef DEBUG_DBLBUF
	uint32_t pkt_recv;
	uint32_t pkt_read;
#endif

};

/* -------------------------------------------------------------------------
 * End point packet buffer helpers
 * ------------------------------------------------------------------------- */

static void __copy_from_pktbuf(void * ptr,
							   struct stm32f_usb_rx_pktbuf * rx,
							   unsigned int cnt)
{
	uint32_t * src;
	uint8_t * dst = (uint8_t *)ptr;
	uint32_t data;
	int i;

	/* copy data to destination buffer */
	src = (uint32_t *)STM32F_USB_PKTBUF_ADDR + (rx->addr / 2);
	for (i = 0; i < (cnt + 1) / 2; i++) {
		data = *src++;
		*dst++ = data;
		*dst++ = data >> 8;
	}
}

static void __copy_to_pktbuf(struct stm32f_usb_tx_pktbuf * tx,
							 uint8_t * src, int len)
{
	uint32_t * dst;
	int i;

	/* copy data to destination buffer */
	dst = (uint32_t *)STM32F_USB_PKTBUF_ADDR + (tx->addr / 2);
	for (i = 0; i < ((len + 1) / 2); i++) {
		*dst++ = src[0] | (src[1] << 8);
		src += 2;
	}

	tx->count = len;
}

/* -------------------------------------------------------------------------
 * End point low level operations
 * ------------------------------------------------------------------------- */

static void __ep_stall(struct stm32f_usb * usb, int ep_id, 
					   struct stm32f_usb_ep * ep)
{
	__set_ep_txstat(usb, ep_id, USB_TX_STALL);
	__set_ep_rxstat(usb, ep_id, USB_RX_STALL);

	DCC_LOG1(LOG_TRACE, "ep_id=%d [STALLED]", ep_id);
	ep->state = EP_STALLED;
}

static int __ep_pkt_send(struct stm32f_usb * usb, int ep_id,
						 struct stm32f_usb_ep * ep, 
						 struct stm32f_usb_tx_pktbuf * tx_pktbuf)
{
	int len;

	len = MIN(ep->xfr_rem, ep->mxpktsz);

	DCC_LOG2(LOG_INFO, "ep_id=%d, len=%d", ep_id, len);

	__copy_to_pktbuf(tx_pktbuf, ep->xfr_ptr, len);
	ep->xfr_rem -= len;
	ep->xfr_ptr += len;

	if ((ep->xfr_rem == 0) && (len != ep->mxpktsz)) {
		/* if we put all data into the TX packet buffer but the data
		 * didn't filled the whole packet, this is the last packet,
		 * otherwise we need to send a ZLP to finish the transaction */
		DCC_LOG1(LOG_INFO, "ep_id=%d [EP_IN_DATA_LAST]", ep_id);
		ep->state = EP_IN_DATA_LAST;
	} else {
		DCC_LOG1(LOG_INFO, "ep_id=%d [EP_IN_DATA]", ep_id);
		ep->state = EP_IN_DATA;
	}

	return len;
}

static void __ep_zlp_send(struct stm32f_usb * usb, int ep_id)
{
	struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;

	pktbuf[ep_id].tx.count = 0;
	__set_ep_txstat(usb, ep_id, USB_TX_VALID);
}

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */

void stm32f_usb_dev_suspend(struct stm32f_usb_drv * drv)
{
	struct stm32f_usb * usb = STM32F_USB;
//	int ep_id;
	/* A brief description of a typical suspend procedure is provided below, 
	   focused on the USB-related aspects of the application software routine 
	   responding to the SUSP notification of the USB peripheral:
	1. Set the FSUSP bit in the USB_CNTR register to 1. This action activates 
	the suspend mode within the USB peripheral. As soon as the suspend mode is 
	activated, the check on SOF reception is disabled to avoid any further 
	SUSP interrupts being issued while the USB is suspended.
	2. Remove or reduce any static power consumption in blocks different from 
	the USB peripheral.
	3. Set LP_MODE bit in USB_CNTR register to 1 to remove static power 
	consumption in the analog USB transceivers but keeping them able to 
	detect resume activity.
	4. Optionally turn off external oscillator and device PLL to stop 
	any activity inside the device. */

	usb->cntr |= USB_FSUSP;

#if 0
	for (ep_id = 0; ep_id < 8; ep_id++) {
		drv->ep[ep_id].state = EP_DISABLED;
		__set_ep_txstat(usb, ep_id, USB_TX_DISABLED);
		__set_ep_rxstat(usb, ep_id, USB_RX_DISABLED);
		__clr_ep_flag(usb, ep_id, USB_CTR_RX | USB_CTR_TX);
		__set_ep_addr(usb, ep_id, 0);
	}
#endif

	/* Enable Reset and Wakeup interrupts */
	usb->cntr |= USB_WKUPM | USB_RESETM;

	drv->ev->on_suspend(drv->cl);

	/* Low power mode */
	usb->cntr |= USB_LP_MODE;

	DCC_LOG(LOG_TRACE, "suspended.");
}

void stm32f_usb_dev_wakeup(struct stm32f_usb_drv * drv)
{
	struct stm32f_usb * usb = STM32F_USB;
	uint32_t fnr = usb->fnr;

	if (((fnr & (USB_RXDP | USB_RXDM)) == 0) || 
		((fnr & (USB_RXDP | USB_RXDM)) == USB_RXDM)) {
	
		/* Clear FSUSP flag */
		usb->cntr = USB_WKUPM | USB_RESETM;

		DCC_LOG(LOG_TRACE, "wakeup.");

		drv->ev->on_wakeup(drv->cl);
	}
}

void stm32f_usb_dev_reset(struct stm32f_usb_drv * drv)
{
	struct stm32f_usb * usb = STM32F_USB;

	DCC_LOG(LOG_TRACE, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

	/* set the btable address */
	usb->btable = 0x000;

	drv->ev->on_reset(drv->cl);

	/* Enable the device and set the address to 0 */
	usb->daddr = USB_EF + 0;

	/* Enable Correct transfer interrupts */
	usb->cntr |= USB_CTRM | USB_SUSPM;
}

/* ------------------------------------------------------------------------- */

int stm32f_usb_dev_ep_stall(struct stm32f_usb_drv * drv, int ep_id)
{
	struct stm32f_usb * usb = STM32F_USB;

	DCC_LOG1(LOG_TRACE, "ep_id=%d", ep_id);

	__ep_stall(usb, ep_id, &drv->ep[ep_id]);

	return 0;
}

/* start sending */
int stm32f_usb_ep_tx_start(struct stm32f_usb_drv * drv, int ep_id,
						   void * buf, unsigned int len)
{
	struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;
	struct stm32f_usb_ep * ep = &drv->ep[ep_id];
	struct stm32f_usb_tx_pktbuf * tx_pktbuf;
	struct stm32f_usb * usb = STM32F_USB;
	uint32_t pri;
	uint32_t epr;

	DCC_LOG2(LOG_INFO, "ep_id=%d len=%d", ep_id, len);

	ep = &drv->ep[ep_id];

	pri = cm3_primask_get();
	cm3_primask_set(1);

	if (ep->state != EP_IDLE) {
		DCC_LOG1(LOG_WARNING, "ep_id=%d invalid endpoint state!", ep_id);
		cm3_primask_set(pri);
		return -1;
	}

	if (len == 0) {
		cm3_primask_set(pri);
		return 0;
	}

	ep->xfr_ptr = buf;
	ep->xfr_rem = len;

	epr = usb->epr[ep_id];
	if (epr & USB_EP_DBL_BUF) {
		DCC_LOG2(LOG_INFO, "double buffer: DTOG=%d SW_BUF=%d", 
				 (epr & USB_DTOG_TX) ? 1: 0,
				 (epr & USB_SWBUF_TX) ? 1: 0);
		/* select the descriptor according to the data toggle bit */
		tx_pktbuf = &pktbuf[ep_id].dbtx[(epr & USB_SWBUF_TX) ? 1: 0];
	} else {
		DCC_LOG(LOG_TRACE, "single");
		tx_pktbuf = &pktbuf[ep_id].tx;
	}

	if (__ep_pkt_send(usb, ep_id, ep, tx_pktbuf) > 0) {
		DCC_LOG(LOG_INFO, "__ep_pkt_send() > 0");
	} else {
		DCC_LOG(LOG_INFO, "__ep_pkt_send() <= 0");
	}

	if (epr & USB_EP_DBL_BUF) {
		__toggle_ep_flag(usb, ep_id, USB_SWBUF_TX);
	} else {
		__set_ep_txstat(usb, ep_id, USB_TX_VALID);
	}

	cm3_primask_set(pri);

	return len;
}

int stm32f_usb_dev_ep_pkt_recv(struct stm32f_usb_drv * drv, int ep_id,
							   void * buf, int len)
{
	struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;
	struct stm32f_usb * usb = STM32F_USB;
	struct stm32f_usb_rx_pktbuf * rx_pktbuf;
	uint32_t epr;
	int cnt;
	int ea;

	epr = usb->epr[ep_id];
	ea = USB_EA_GET(epr);
	(void)ea;

	if (epr & USB_EP_DBL_BUF) {
		/* double buffer */
		/* select the descriptor according to the data toggle bit */
		rx_pktbuf = &pktbuf[ep_id].dbrx[(epr & USB_SWBUF_RX) ? 1 : 0];
		DCC_LOG2(LOG_INFO, "RX dbl buf DOTG=%d SW_BUF=%d", 
				 (epr & USB_DTOG_RX) ? 1 : 0, (epr & USB_SWBUF_RX) ? 1 : 0);
	} else {
		/* single buffer */
		rx_pktbuf = &pktbuf[ep_id].rx;
		DCC_LOG1(LOG_INFO, "===== ep_id=%d: single buffer", ep_id);
	}

	DCC_LOG1(LOG_INFO, "cnt=%d", rx_pktbuf->count);

	cnt = MIN(rx_pktbuf->count, len);

	if (cnt > 0) {
		/* Data received */
		__copy_from_pktbuf(buf, rx_pktbuf, cnt);

		if (epr & USB_EP_DBL_BUF) {
			/* release the buffer to the USB controller */
			__toggle_ep_flag(usb, ep_id, USB_SWBUF_RX);
		} else {
			/* free the out(rx) packet buffer */
			__set_ep_rxstat(usb, ep_id, USB_RX_VALID);
		}

#ifdef DEBUG_DBLBUF
		drv.pkt_read++;

		if (drv.pkt_recv != drv.pkt_read) {
			DCC_LOG2(LOG_WARNING, "recv=%d read=%d", 
					 drv.pkt_recv, drv.pkt_read); 
		}
#endif
	} else {
		DCC_LOG1(LOG_WARNING, "cnt==%d", cnt);
	}

	DCC_LOG2(LOG_INFO, "OUT EP%d, cnt=%d", ea, cnt);

	return cnt;
}

int stm32f_usb_dev_ep_zlp_send(struct stm32f_usb_drv * drv, int ep_id)
{
	struct stm32f_usb * usb = STM32F_USB;

	DCC_LOG1(LOG_TRACE, "ep_id=%d", ep_id);

	__ep_zlp_send(usb, ep_id);

	return 0;
}

void stm32f_usb_dev_ep_nak(struct stm32f_usb_drv * drv, int ep_id, bool flag)
{
#if 0
	struct stm32f_usb * usb = STM32F_USB;

	if (flag)
		__set_ep_txstat(usb, ep_id, USB_TX_NAK);
	else
		__set_ep_txstat(usb, ep_id, USB_TX_VALID);
#endif
	DCC_LOG1(LOG_INFO, "ep_id=%d", ep_id);
}


/* FIXME: find another way of initializing the packet buffer addresses */
#define PKTBUF_BUF_BASE (8 * 8)
static unsigned int addr = PKTBUF_BUF_BASE;

int stm32f_usb_dev_ep_init(struct stm32f_usb_drv * drv, 
						   const usb_dev_ep_info_t * info,
						   void * xfr_buf, int buf_len)
{
	struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;
	unsigned int sz;
	struct stm32f_usb * usb = STM32F_USB;
	struct stm32f_usb_ep * ep;
	int mxpktsz = info->mxpktsz;
	int ep_id;

	if ((ep_id = info->addr & 0x7f) > 7) {
		DCC_LOG1(LOG_WARNING, "addr(%d) > 7", ep_id);
		return -1;
	}

	DCC_LOG2(LOG_MSG, "ep_id=%d mxpktsz=%d", ep_id, mxpktsz);

	ep = &drv->ep[ep_id];
	ep->mxpktsz = mxpktsz;
	ep->state = EP_IDLE;
	ep->on_ev = info->on_ev;

	ep->xfr_buf = (uint8_t *)xfr_buf;
	ep->xfr_buf_len = buf_len;

	DCC_LOG3(LOG_TRACE, "ep_id=%d addr=%d mxpktsz=%d",
			 ep_id, info->addr & 0x7f, mxpktsz);

	if (ep_id == 0) {

		/* clear the correct transfer bits */
		__clr_ep_flag(usb, 0, USB_CTR_RX | USB_CTR_TX);
		__set_ep_type(usb, 0, USB_EP_CONTROL);
		__set_ep_txstat(usb, 0, USB_TX_NAK);

		/* reset packet buffer address pointer */
		addr = PKTBUF_BUF_BASE;

		/* allocate single buffers for TX and RX */
		sz = __pktbuf_tx_cfg(&pktbuf[0].tx, addr, mxpktsz);
		addr += sz;
		sz = __pktbuf_rx_cfg(&pktbuf[0].rx, addr, mxpktsz);
		addr += sz;

		__clr_ep_flag(usb, 0, USB_EP_STATUS_OUT);
		__set_ep_rxstat(usb, 0, USB_RX_VALID);

		DCC_LOG1(LOG_TRACE, "epr=0x%04x...", usb->epr[0]);

		return 0;
	}

	/* clear the correct transfer bits */
	__clr_ep_flag(usb, ep_id, USB_CTR_RX | USB_CTR_TX);
	__set_ep_addr(usb, ep_id, info->addr & 0x7f);

	__set_ep_rxstat(usb, ep_id, USB_RX_NAK);
	__set_ep_txstat(usb, ep_id, USB_TX_NAK);

	switch (info->attr & 0x03) {
	case ENDPOINT_TYPE_CONTROL:
		__set_ep_type(usb, ep_id, USB_EP_CONTROL);
		/* allocate single buffers for TX and RX */
		sz = __pktbuf_tx_cfg(&pktbuf[ep_id].tx, addr, mxpktsz);
		addr += sz;
		sz = __pktbuf_rx_cfg(&pktbuf[ep_id].rx, addr, mxpktsz);
		addr += sz;
		DCC_LOG(LOG_TRACE, "CONTROL");
		break;

	case ENDPOINT_TYPE_ISOCHRONOUS:
		DCC_LOG(LOG_WARNING, "ISOCHRONOUS");
		break;

	case ENDPOINT_TYPE_BULK:
		__set_ep_type(usb, ep_id, USB_EP_BULK);
		__set_ep_flag(usb, ep_id, USB_EP_DBL_BUF);
		if (info->addr & USB_ENDPOINT_IN) {
			DCC_LOG(LOG_TRACE, "BULK IN");
			sz = __pktbuf_tx_cfg(&pktbuf[ep_id].dbtx[0], addr, mxpktsz);
			addr += sz;
			sz = __pktbuf_tx_cfg(&pktbuf[ep_id].dbtx[1], addr, mxpktsz);
			addr += sz;
			__set_ep_txstat(usb, ep_id, USB_TX_VALID);
		} else {
			DCC_LOG(LOG_TRACE, "BULK OUT");
			sz = __pktbuf_rx_cfg(&pktbuf[ep_id].dbrx[0], addr, mxpktsz);
			addr += sz;
			sz = __pktbuf_rx_cfg(&pktbuf[ep_id].dbrx[1], addr, mxpktsz);
			addr += sz;
			__set_ep_rxstat(usb, ep_id, USB_RX_VALID);
		}
		break;

	case ENDPOINT_TYPE_INTERRUPT:
		__set_ep_type(usb, ep_id, USB_EP_INTERRUPT);
		if (info->addr & USB_ENDPOINT_IN) {
			DCC_LOG(LOG_TRACE, "INTERRUPT IN");
			sz = __pktbuf_tx_cfg(&pktbuf[ep_id].tx, addr, mxpktsz);
			addr += sz;
		} else {
			DCC_LOG(LOG_TRACE, "INTERRUPT OUT");
			sz = __pktbuf_rx_cfg(&pktbuf[ep_id].rx, addr, mxpktsz);
			addr += sz;
			__set_ep_rxstat(usb, ep_id, USB_RX_VALID);
		}
		break;
	}

	DCC_LOG1(LOG_TRACE, "epr=0x%04x...", usb->epr[ep_id]);

	return ep_id;
}

int stm32f_usb_dev_ep_disable(struct stm32f_usb_drv * drv,  int ep_id)
{
	struct stm32f_usb_ep * ep = &drv->ep[ep_id];
	struct stm32f_usb * usb = STM32F_USB;

	DCC_LOG1(LOG_TRACE, "ep_id=%d...", ep_id);

	ep->state = EP_DISABLED;
	__set_ep_txstat(usb, ep_id, USB_TX_DISABLED);
	__set_ep_rxstat(usb, ep_id, USB_RX_DISABLED);
	__clr_ep_flag(usb, ep_id, USB_CTR_RX | USB_CTR_TX);
	__set_ep_addr(usb, ep_id, 0);

	return 0;
}

void stm32f_usb_dev_ep0_out(struct stm32f_usb * usb,
							struct stm32f_usb_ep * ep, 
							struct stm32f_usb_drv * drv)
{
	struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;
	int cnt;
	int n;

	if (ep->state == EP_WAIT_STATUS_OUT) {
		ep->state = EP_IDLE;
		DCC_LOG(LOG_INFO, "EP0 OUT [IDLE] >>>>>>>> SETUP END");
		__set_ep_rxstat(usb, 0, USB_RX_VALID);
		return;
	}

	if ((ep->state == EP_IN_DATA) || (ep->state == EP_IN_DATA_LAST)) {
		DCC_LOG(LOG_TRACE, "EP0 OUT [STALLED]");
		__ep_stall(usb, 0, ep);
		return;
	}

	//	cnt = MIN(rx->count, max);

	cnt = pktbuf[0].rx.count;
	n = MIN(cnt, ep->xfr_rem);

	__copy_from_pktbuf(ep->xfr_ptr, &pktbuf[0].rx, n);

	ep->xfr_ptr += n;
	ep->xfr_rem -= n;

	if (ep->state == EP_OUT_DATA_LAST) {
		ep->state = EP_WAIT_STATUS_IN;
		DCC_LOG1(LOG_INFO, "EP0 cnt=%d [EP_WAIT_STATUS_IN]", cnt);
		__ep_zlp_send(usb, 0);
		return;
	}

	if (ep->xfr_rem < ep->mxpktsz) {
		/* last transfer */
		ep->state = EP_OUT_DATA_LAST;
		DCC_LOG1(LOG_INFO, "EP0 cnt=%d [OUT_DATA_LAST]", cnt);
	}
	__set_ep_rxstat(usb, 0, USB_RX_VALID);

	DCC_LOG1(LOG_INFO, "EP0 cnt=%d", cnt);
}

static void stm32f_usb_dev_ep0_in(struct stm32f_usb * usb,
								  struct stm32f_usb_ep * ep,
								  struct stm32f_usb_drv * drv)
{
	struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;
	void * dummy = NULL;

	DCC_LOG(LOG_MSG, "EP0 IN");

	if (ep->state == EP_WAIT_STATUS_IN) {
		struct usb_request * req = &drv->req;

		if (((req->request << 8) | req->type) == STD_SET_ADDRESS) {
			usb->daddr = req->value | USB_EF;
			DCC_LOG(LOG_INFO, "address set!");
		}
		__set_ep_rxstat(usb, 0, USB_RX_STALL);
		__set_ep_txstat(usb, 0, USB_TX_STALL);
		ep->on_setup(drv->cl, req, dummy);
		ep->state = EP_IDLE;
		DCC_LOG(LOG_INFO, ">>>>>>>> SETUP END");
		return;
	}

	if (ep->state == EP_IN_DATA_LAST) {
		ep->state = EP_WAIT_STATUS_OUT;
		DCC_LOG(LOG_INFO, "EP0 [WAIT_STATUS_OUT]");
		__set_ep_rxstat(usb, 0, USB_RX_VALID);
		return;
	}

	__ep_pkt_send(usb, 0, ep, &pktbuf[0].tx);
	__set_ep_txstat(usb, 0, USB_TX_VALID);

	if (ep->state == EP_IN_DATA) {
		__set_ep_rxstat(usb, 0, USB_RX_STALL);
	}
}

static void stm32f_usb_dev_ep0_setup(struct stm32f_usb * usb,
									 struct stm32f_usb_ep * ep, 
									 struct stm32f_usb_drv * drv) {
	struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;
	struct usb_request * req = &drv->req;
	int cnt;
	int len;

	DCC_LOG(LOG_INFO, "SETUP START <<<<<<<<<<<<<<<");

	cnt = pktbuf[0].rx.count;

#if ENABLE_PEDANTIC_CHECK
	if (cnt != 8) {
		__ep_stall(usb, 0, ep);
		DCC_LOG1(LOG_ERROR, "cnt(%d) != 8 [ERROR]", cnt);
		return;
	}
#endif

	/* copy data from packet buffer */
	__copy_from_pktbuf(req, &pktbuf[0].rx, cnt);

	/* No-Data control SETUP transaction */
	if (req->length == 0) {
		__ep_zlp_send(usb, 0);
		DCC_LOG(LOG_INFO, "EP0 [WAIT_STATUS_IN] no data Dev->Host");
		ep->state = EP_WAIT_STATUS_IN;
		return;
	}

	if (req->type & 0x80) {
		/* Control Read SETUP transaction (IN Data Phase) */

		DCC_LOG(LOG_INFO, "EP0 [SETUP] IN Dev->Host");
		ep->xfr_ptr = NULL;
		len = ep->on_setup(drv->cl, req, (void *)&ep->xfr_ptr);
#if ENABLE_PEDANTIC_CHECK
		if (len < 0) {
			__ep_stall(usb, 0, ep);
			DCC_LOG(LOG_INFO, "EP0 [STALLED] len < 0");
			return;
		}
		if (ep->tx_buf == NULL) {
			__ep_stall(usb, 0, ep);
			DCC_LOG(LOG_INFO, "EP0 [STALLED] tx_buf == NULL");
			return;
		}
#endif
		ep->xfr_rem = MIN(req->length, len);
		DCC_LOG1(LOG_INFO, "EP0 data lenght = %d", ep->xfr_rem);
		__ep_pkt_send(usb, 0, ep, &pktbuf[0].tx);
		__set_ep_txstat(usb, 0, USB_TX_VALID);
		/* While enabling the last data stage, the opposite direction should
		   be set to NAK, so that, if the host reverses the transfer direction
		   (to perform the status stage) immediately, it is kept waiting for
		   the completion of the control operation. If the control operation
		   completes successfully, the software will change NAK to VALID,
		   otherwise to STALL.
		 */
		if (ep->state == EP_IN_DATA) {
			/* A USB device can determine the number and direction of 
			   data stages by interpreting the data transferred in the 
			   SETUP stage, and is required to STALL the transaction in 
			   the case of errors. To do so, at all data stages before 
			   the last, the unused direction should be set to STALL, so 
			   that, if the host reverses the transfer direction too soon, 
			   it gets a STALL as a status stage.
			 */
			__set_ep_rxstat(usb, 0, USB_RX_STALL);
		}

	} else {
		/* Control Write SETUP transaction (OUT Data Phase) */

		ep->xfr_ptr = ep->xfr_buf;
		ep->xfr_rem = req->length;

		DCC_LOG1(LOG_INFO, "xfr_ptr=0x%08x", ep->xfr_ptr);

		if (ep->xfr_rem > ep->xfr_buf_len) {
			ep->xfr_rem = ep->xfr_buf_len;
			DCC_LOG(LOG_ERROR, "transfer to large to fit the buffer!");
		}

		if (ep->xfr_rem < ep->mxpktsz) {
			/* last and only transfer */
			ep->state = EP_OUT_DATA_LAST;
			DCC_LOG(LOG_INFO, "EP0 [OUT_DATA_LAST] OUT Host->Dev!!!!");

		} else {
			ep->state = EP_OUT_DATA;
			DCC_LOG(LOG_INFO, "EP0 [OUT_DATA] OUT Host->Dev!!!!");
		}

		__set_ep_rxstat(usb, 0, USB_RX_VALID);
	}
}

/*
 * Arguments:
 *
 *   drv: pointer to the driver runtime structure.
 *
 *   ep: list of endpoint configuration information. The first item on this list
 *       must be the endpoint 0.
 *
 *  cnt: number of items in the list.
 */

int stm32f_usb_dev_init(struct stm32f_usb_drv * drv, usb_class_t * cl,
						const struct usb_class_events * ev)
{
	struct stm32f_usb * usb = STM32F_USB;
	int i;

	drv->cl = cl;
	drv->ev = ev;

	DCC_LOG1(LOG_TRACE, "ev=0x%08x", drv->ev);

	stm32f_usb_power_off(usb);

	udelay(1000);

	/* Initialize IO pins */
	stm32f_usb_io_init();

	stm32f_usb_power_on(usb);

	for (i = 0;  i < USB_DRIVER_EP_MAX; ++i) {
		drv->ep[i].state = EP_DISABLED;
	}

	/* Enable Reset, SOF  and Wakeup interrupts */
	usb->cntr = USB_WKUP | USB_RESETM;

	DCC_LOG(LOG_INFO, "[ATTACHED]");
	return 0;
}


/* Private USB device driver data */
struct stm32f_usb_drv stm32f_usb_drv0;

/* USB high priority ISR */
void stm32f_can1_tx_usb_hp_isr(void)
{
	struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;
	struct stm32f_usb_drv * drv = &stm32f_usb_drv0;
	struct stm32f_usb * usb = STM32F_USB;
	struct stm32f_usb_rx_pktbuf * rx_pktbuf;
	struct stm32f_usb_tx_pktbuf * tx_pktbuf;
	struct stm32f_usb_ep * ep;
	uint32_t epr;
	int ep_id;
	uint32_t sr;
	int len;

	sr = usb->istr;

	if ((sr & USB_CTR) == 0) {
		DCC_LOG1(LOG_WARNING, "SR 0x%04x", sr);
		return;
	}

	/* clear interrupt */
	usb->istr = sr & ~USB_CTR;

	ep_id = USB_EP_ID_GET(sr);

	DCC_LOG1(LOG_INFO, "CTR ep_id=%d", ep_id);

	epr = usb->epr[ep_id];
	ep = &drv->ep[ep_id];

	if (epr & USB_CTR_RX) {
		/* OUT */
		__clr_ep_flag(usb, ep_id, USB_CTR_RX);

		/* select the descriptor according to the data toggle bit */
		rx_pktbuf = &pktbuf[ep_id].dbrx[(epr & USB_SWBUF_RX) ? 1: 0];

		if (((epr & USB_DTOG_RX) ? 1: 0) == ((epr & USB_SWBUF_RX) ? 1: 0)) {
			DCC_LOG3(LOG_WARNING, "RX dblbuf DOTG=%d SW_BUF=%d cnt=%d", 
					 (epr & USB_DTOG_RX) ? 1: 0, (epr & USB_SWBUF_RX) ? 1: 0, 
					 rx_pktbuf->count);
		}

		DCC_LOG3(LOG_INFO, "RX dblbuf DOTG=%d SW_BUF=%d cnt=%d", 
				 (epr & USB_DTOG_RX) ? 1: 0, (epr & USB_SWBUF_RX) ? 1: 0, 
				 rx_pktbuf->count);

#ifdef DEBUG_DBLBUF
		drv->pkt_recv++;
#endif

		/* call class endpoint callback */
		ep->on_out(drv->cl, ep_id, rx_pktbuf->count);
	}

	if (epr & USB_CTR_TX) {
		/* IN */
		__clr_ep_flag(usb, ep_id, USB_CTR_TX);

		DCC_LOG3(LOG_INFO, "ep%d: TX double buffer: DTOG=%d SW_BUF=%d", 
				 ep_id, (epr & USB_DTOG_TX) ? 1: 0,
				 (epr & USB_SWBUF_TX) ? 1: 0);

		switch (ep->state) {
		case EP_IDLE:
			DCC_LOG1(LOG_WARNING, "ep%d: IDLE!!!", ep_id);
			break;

		case EP_IN_DATA_LAST:
			DCC_LOG1(LOG_INFO, "ep_id=%d [EP_IDLE]", ep_id);
			ep->state = EP_IDLE;
			/* call class endpoint callback */
			ep->on_in(drv->cl, ep_id);
			break;

		case EP_IN_DATA: 
			/* select the descriptor according to the data toggle bit */
			tx_pktbuf = &pktbuf[ep_id].dbtx[(epr & USB_SWBUF_TX) ? 1: 0];
			/* send the next data chunk */
			len = MIN(ep->xfr_rem, ep->mxpktsz);

			DCC_LOG2(LOG_INFO, "ep_id=%d, len=%d", ep_id, len);

			/* copy to packet buffer */
			__copy_to_pktbuf(tx_pktbuf, ep->xfr_ptr, len);

			/* release the previous packet buffer */
			__toggle_ep_flag(usb, ep_id, USB_SWBUF_TX);

			ep->xfr_rem -= len;
			ep->xfr_ptr += len;

			if ((ep->xfr_rem == 0) && (len != ep->mxpktsz)) {
				/* if we put all data into the TX packet buffer but the data
				 * didn't filled the whole packet, this is the last packet,
				 * otherwise we need to send a ZLP to finish the transaction */
				DCC_LOG1(LOG_INFO, "ep_id=%d [EP_IN_DATA_LAST]", ep_id);
				ep->state = EP_IN_DATA_LAST;
			}

			break;

		default:
			DCC_LOG1(LOG_ERROR, "ep%d: invalid state!!!", ep_id);
			break;

		}
	}
}

/* USB low priority ISR */
void stm32f_can1_rx0_usb_lp_isr(void)
{
	struct stm32f_usb_drv * drv = &stm32f_usb_drv0;
	struct stm32f_usb * usb = STM32F_USB;
	uint32_t sr = usb->istr;

	if (sr & USB_SOF) {
		usb->istr = sr & ~USB_SOF;
		DCC_LOG(LOG_INFO, "SOF");
	}

	if (sr & USB_SUSP) {
		/* clear interrupts */
		usb->istr = sr & ~USB_SUSP;
		DCC_LOG(LOG_TRACE, "SUSP");
		stm32f_usb_dev_suspend(drv);
	}

	if (sr & USB_WKUP) {
		usb->istr = sr & ~USB_WKUP;
		DCC_LOG(LOG_TRACE, "WKUP");
		stm32f_usb_dev_wakeup(drv);
	}

	if (sr & USB_RESET) {
		usb->istr = sr & ~USB_RESET;
		DCC_LOG(LOG_TRACE, "RESET");
		stm32f_usb_dev_reset(drv);
	}

	if (sr & USB_ERR) {
		usb->istr = sr & ~USB_ERR;
		DCC_LOG(LOG_TRACE, "ERR");
	}

	if (sr & USB_PMAOVR) {
		usb->istr = sr & ~USB_PMAOVR;
		DCC_LOG(LOG_TRACE, "PMAOVR");
	}

	if (sr & USB_CTR) {
		struct stm32f_usb_pktbuf * pktbuf = STM32F_USB_PKTBUF;
		struct stm32f_usb_ep * ep;
		uint32_t epr;
		int ep_id;

		/*
		 * The CTR_RX event is serviced by first
		 determining the transaction type (SETUP bit in the USB_EPnR 
		 register); the application software must clear the interrupt 
		 flag bit and get the number of received bytes reading the 
		 COUNTn_RX location inside the buffer description table entry 
		 related to the endpoint being processed. After the received 
		 data is processed, the application software should set the 
		 STAT_RX bits to ‘11 (Valid) in the USB_EPnR, enabling further 
		 transactions. While the STAT_RX bits are equal to ‘10 (NAK), 
		 any OUT request addressed to that endpoint is NAKed, indicating 
		 a flow control condition: the USB host will retry the 
		 transaction until it succeeds. It is mandatory to execute the 
		 sequence of operations in the above mentioned order to avoid 
		 losing the notification of a second OUT transaction addressed 
		 to the same endpoint following immediately the one which 
		 triggered the CTR interrupt.
		 */


		/* clear interrupt */
		usb->istr = sr & ~USB_CTR;

		ep_id = USB_EP_ID_GET(sr);

		DCC_LOG1(LOG_INFO, "CTR ep_id=%d", ep_id);

		epr = usb->epr[ep_id];
		ep = &drv->ep[ep_id];

		if (ep_id == 0) {
			/* Service control point */
			__set_ep_rxstat(usb, ep_id, USB_RX_NAK);
			__set_ep_txstat(usb, ep_id, USB_TX_NAK);

			if (((sr & USB_DIR) == 0) || (epr & USB_CTR_TX)) {
				/* DIR = 0 -> IN */
				__clr_ep_flag(usb, 0, USB_CTR_TX);
				stm32f_usb_dev_ep0_in(usb, ep, drv);
			} else {
				/* DIR = 1 -> OUT/SETUP */
				__clr_ep_flag(usb, 0, USB_CTR_RX);
				if (epr & USB_SETUP) {
					/* SETUP */
					stm32f_usb_dev_ep0_setup(usb, ep, drv);
				} else {
					/* OUT */
					stm32f_usb_dev_ep0_out(usb, ep, drv);
				}
			}
			return;
		}

		if (epr & USB_CTR_RX) {
			struct stm32f_usb_rx_pktbuf * rx_pktbuf;

			/* OUT */
			__clr_ep_flag(usb, ep_id, USB_CTR_RX);

			/* single buffer */
			rx_pktbuf = &pktbuf[ep_id].rx;
			DCC_LOG1(LOG_TRACE, "cnt=%d", rx_pktbuf->count);

			/* call class endpoint callback */
			ep->on_out(drv->cl, ep_id, rx_pktbuf->count);
		}

		if (epr & USB_CTR_TX) {
			/* IN */
			__clr_ep_flag(usb, ep_id, USB_CTR_TX);

			if (ep->state != EP_IN_DATA_LAST) {
				__ep_pkt_send(usb, ep_id, ep, &pktbuf[ep_id].tx);
			} else {
				__set_ep_txstat(usb, ep_id, USB_TX_NAK);
				ep->state = EP_IDLE;
				/* call class endpoint callback */
				ep->on_in(drv->cl, ep_id);
			}
		}
	}
}

/* USB device operations */
const struct usb_dev_ops stm32f_usb_ops = {
	.dev_init = (usb_dev_init_t)stm32f_usb_dev_init,
	.ep_tx_start = (usb_dev_ep_tx_start_t)stm32f_usb_ep_tx_start,
	.ep_init = (usb_dev_ep_init_t)stm32f_usb_dev_ep_init,
	.ep_disable = (usb_dev_ep_disable_t)stm32f_usb_dev_ep_disable,
	.ep_stall = (usb_dev_ep_stall_t)stm32f_usb_dev_ep_stall,
	.ep_zlp_send = (usb_dev_ep_zlp_send_t)stm32f_usb_dev_ep_zlp_send,
	.ep_nak = (usb_dev_ep_nak_t)stm32f_usb_dev_ep_nak,
	.ep_pkt_recv = (usb_dev_ep_pkt_recv_t)stm32f_usb_dev_ep_pkt_recv
};

/* USB device driver */
const struct usb_dev stm32f_usb_fs_dev = {
	.priv = (void *)&stm32f_usb_drv0,
	.op = &stm32f_usb_ops
};

#endif /* defined(STM32F103) || defined(STM32F30X) */

