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
 * @file yard-ice.c
 * @brief YARD-ICE UART console
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

#ifdef STM32F_OTG_FS

typedef enum {
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

/* Endpoint control */
struct stm32f_otg_ep {


	ep_state_t state;

	uint16_t xfr_max;
	uint16_t xfr_rem;
	uint16_t xfr_buf_len;

	uint8_t * xfr_buf;
	uint8_t * xfr_ptr;

	/* reload value for the DOEPTSIZ register */
	uint32_t doeptsiz;

	union {
		usb_class_on_ep_ev_t on_ev;
		usb_class_on_ep_in_t on_in;
		usb_class_on_ep_out_t on_out;
		usb_class_on_ep_setup_t on_setup;
	};
};

#define OTG_FS_DRIVER_EP_MAX 4

/* USB Device runtime driver data */
struct stm32f_otg_drv {
	struct stm32f_otg_ep ep[OTG_FS_DRIVER_EP_MAX];
	usb_class_t * cl;
	const struct usb_class_events * ev;
	struct usb_request req;
	uint16_t fifo_addr;
/*
	uint16_t rx_tmr;
	uint16_t rx_tmr;
	*/
};

#define OTG_FS_RX_FIFO_SIZE 512

/* EP TX fifo memory allocation */
void __ep_pktbuf_alloc(struct stm32f_otg_drv * drv, int ep_id, int siz)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;

	switch (ep_id) {
	case 0:
		otg_fs->dieptxf0 = OTG_FS_TX0FD_SET(siz / 4) | 
			OTG_FS_TX0FSA_SET(drv->fifo_addr / 4);
		break;
	case 1:
		otg_fs->dieptxf1 = OTG_FS_INEPTXFD_SET(siz / 4) | 
			OTG_FS_INEPTXSA_SET(drv->fifo_addr / 4);
		break;
	case 2:
		otg_fs->dieptxf2 = OTG_FS_INEPTXFD_SET(siz / 4) | 
			OTG_FS_INEPTXSA_SET(drv->fifo_addr / 4);
		break;
	case 3:
		otg_fs->dieptxf3 = OTG_FS_INEPTXFD_SET(siz / 4) | 
			OTG_FS_INEPTXSA_SET(drv->fifo_addr / 4);
		break;
	}

	DCC_LOG2(LOG_TRACE, "addr=%d siz=%d", drv->fifo_addr, siz);

	drv->fifo_addr += siz;
}

static void __copy_from_pktbuf(void * ptr,
							volatile uint32_t * pop,
							unsigned int cnt)
{
	uint8_t * dst = (uint8_t *)ptr;
	uint32_t data;
	int i;

	/* pop data from the fifo and copy to destination buffer */
	for (i = 0; i < (cnt + 3) / 4; i++) {
		data = *pop;
		*dst++ = data;
		*dst++ = data >> 8;
		*dst++ = data >> 16;
		*dst++ = data >> 24;
	}
}

static bool __ep_tx_push(struct stm32f_otg_drv * drv, int ep_id)
{
	struct stm32f_otg_ep * ep = &drv->ep[ep_id];
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	int cnt;

	/* push data into transmit fifo */
	cnt = stm32f_otg_fs_txf_push(otg_fs, ep_id, ep->xfr_ptr);

	if (cnt < 0) {
		DCC_LOG(LOG_WARNING, "stm32f_otg_fs_txf_push() failed!");
		otg_fs->diepempmsk &= ~(1 << ep_id);
		return false;
	}

	ep->xfr_ptr += cnt;
	ep->xfr_rem -= cnt;

	return true;
}

static void __ep_rx_pop(struct stm32f_otg_drv * drv, int ep_id, int len)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	volatile uint32_t * pop = &otg_fs->dfifo[ep_id].pop;
	struct stm32f_otg_ep * ep = &drv->ep[ep_id];
	uint8_t * dst;
	uint32_t data;
	int wcnt;
	int i;

	/* Number of words in the receive fifo */
	wcnt = (len + 3) / 4;
	DCC_LOG1(LOG_INFO, "poping %d words from FIFO.", wcnt);

	if (ep->xfr_rem >= len) {
		/* If we have enough room in the destination buffer
		 * pop data from the fifo and copy to destination buffer */
		dst = ep->xfr_ptr;
		for (i = 0; i < wcnt; ++i) {
			data = *pop;
			*dst++ = data;
			*dst++ = data >> 8;
			*dst++ = data >> 16;
			*dst++ = data >> 24;
		}
		ep->xfr_ptr += len;
		ep->xfr_rem -= len;
	} else {
		DCC_LOG(LOG_WARNING, "not room to copy the whole packet, discarding!");
		for (i = 0; i < wcnt; ++i) {
			data = *pop;
			(void)data;
		}
	}
}

static void __ep_zlp_send(struct stm32f_otg_fs * otg_fs, int epnum)
{
	DCC_LOG(LOG_INFO, "Send: ZLP");

	otg_fs->inep[epnum].dieptsiz = OTG_FS_PKTCNT_SET(1) | OTG_FS_XFRSIZ_SET(0);
	otg_fs->inep[epnum].diepctl |= OTG_FS_EPENA | OTG_FS_CNAK;
}

void stm32f_otg_dev_ep_nak(struct stm32f_otg_drv * drv, int ep_id, bool flag)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;

	/* FIXME: select outep/inep ... */
	if (flag)
		otg_fs->outep[ep_id].doepctl |= OTG_FS_SNAK;
	else
		otg_fs->outep[ep_id].doepctl |= OTG_FS_CNAK;
}

int stm32f_otg_dev_ep_stall(struct stm32f_otg_drv * drv, int ep_id)
{
	DCC_LOG1(LOG_TRACE, "ep_id=%d", ep_id);
#if 0
	__ep_stall(drv, ep_id);
#endif
	return 0;
}

/* start sending */
int stm32f_otg_dev_ep_tx_start(struct stm32f_otg_drv * drv, int ep_id,
		void * buf, unsigned int len)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	struct stm32f_otg_ep * ep;
	int ret;

	ep = &drv->ep[ep_id];
	ep->xfr_ptr = buf;
	ep->xfr_rem = MIN(len, ep->xfr_max);

	/* prepare fifo to transmit */
	if ((ret = stm32f_otg_fs_txf_setup(otg_fs, ep_id, ep->xfr_rem)) < 0) {
		DCC_LOG(LOG_WARNING, "stm32f_otg_fs_txf_setup() failed!");
	} else {
		/* umask FIFO empty interrupt */
		otg_fs->diepempmsk |= (1 << ep_id);
	}

	DCC_LOG4(LOG_TRACE, "ep_id=%d len=%d xfr_max=%d ret=%d", 
			 ep_id, len, ep->xfr_max, ret);

	return ret;
}

int stm32f_otg_dev_ep_pkt_recv(struct stm32f_otg_drv * drv, int ep_id,
		void * buf, int len)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	uint8_t * cp = (uint8_t *)buf;
	struct stm32f_otg_ep * ep;
	unsigned int rem;
	unsigned int cnt;
	uint32_t data;

	DCC_LOG1(LOG_TRACE, "ep_id=%d", ep_id);

	ep = &drv->ep[ep_id];
	
	/* transfer data from fifo */
	cnt = MIN(ep->xfr_rem, len);

	rem = cnt;
	while (rem >= 4) {
		/* word trasfer */
		data = otg_fs->dfifo[ep_id].pop;
		cp[0] = data;
		cp[1] = data >> 8;
		cp[2] = data >> 16;
		cp[3] = data >> 24;
		cp += 4;
		rem -= 4;
	}

	if (rem > 0) {
		/* remaining data */
		data = otg_fs->dfifo[ep_id].pop;
		cp[0] = data;
		if (rem > 1)
			cp[1] = data >> 8;
		if (rem > 2)
			cp[2] = data >> 16;
		rem = 0;
	}

	if ((rem = ep->xfr_rem - cnt) > 0) {
		DCC_LOG1(LOG_WARNING, "dropping %d bytes...", rem);
		/* remove remaining data from fifo */
		do {
			data = otg_fs->dfifo[ep_id].pop;
			(void)data;
			rem -= 4;
		} while (rem);
	}

	/* reset transfer pointer */
	ep->xfr_rem = 0;

	/* 5. After the data payload is popped from the receive FIFO, the 
	   RXFLVL interrupt (OTG_FS_GINTSTS) must be unmasked. */
	DCC_LOG1(LOG_TRACE, "cnt=%d enabling RXFLVL interrupt", cnt);

	/* Reenable RX fifo interrupts */
	otg_fs->gintmsk |= OTG_FS_RXFLVLM;

	return cnt;
}

int stm32f_otg_dev_ep_zlp_send(struct stm32f_otg_drv * drv, int ep_id)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;

	DCC_LOG1(LOG_TRACE, "ep_id=%d", ep_id);

	__ep_zlp_send(otg_fs, ep_id);

	return 0;
}

int stm32f_otg_dev_ep_init(struct stm32f_otg_drv * drv, 
						   const usb_dev_ep_info_t * info, 
						   void * xfr_buf, int buf_len)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	struct stm32f_otg_ep * ep;
	int mxpktsz = info->mxpktsz;
	int ep_id;

	if ((ep_id = info->addr & 0x7f) > 3) {
		DCC_LOG1(LOG_ERROR, "invalid address addr=%d", ep_id);
		return -1;
	}

	DCC_LOG3(LOG_TRACE, "ep_id=%d addr=%d mxpktsz=%d", 
			 ep_id, info->addr & 0x7f, mxpktsz);

	ep = &drv->ep[ep_id];
	ep->xfr_buf = (uint8_t *)xfr_buf;
	ep->xfr_buf_len = buf_len;
	ep->xfr_ptr = NULL;
	ep->xfr_rem = 0;
	ep->state = EP_IDLE;
	ep->on_ev = info->on_ev;

	/* mask FIFO empty interrupt */
	otg_fs->diepempmsk &= ~(1 << ep_id);

	if (ep_id == 0) {
		ep->xfr_max = mxpktsz;

		/* Initialize EP0 */
		otg_fs->inep[0].diepctl = OTG_FS_TXFNUM_SET(0) |
			OTG_FS_EP0_MPSIZ_SET(mxpktsz);

		/* 3. Set up the Data FIFO RAM for each of the FIFOs
		   – Program the OTG_FS_GRXFSIZ register, to be able to receive
		   control OUT data and setup data. If thresholding is not enabled,
		   at a minimum, this must be equal to 1 max packet size of
		   control endpoint 0 + 2 Words (for the status of the control OUT
		   data packet) + 10 Words (for setup packets).
		   - Program the OTG_FS_TX0FSIZ register (depending on the FIFO number
		   chosen) to be able to transmit control IN data. At a minimum, this
		   must be equal to 1 max packet size of control endpoint 0. */

		__ep_pktbuf_alloc(drv, 0, mxpktsz);

		/*  4. Program the following fields in the endpoint-specific registers
			for control OUT endpoint 0 to receive a SETUP packet
			– STUPCNT = 3 in OTG_FS_DOEPTSIZ0 (to receive up to 3 back-to-back
			SETUP packets)
			At this point, all initialization required to receive SETUP packets
			is done. */

		/* cache this value to be reused */
		ep->doeptsiz = OTG_FS_STUPCNT_SET(3) | 
			OTG_FS_PKTCNT_SET(1) | OTG_FS_XFRSIZ_SET(40);

		/* Prepare to receive */
		otg_fs->outep[0].doeptsiz = ep->doeptsiz;
		/* EP enable */
		otg_fs->outep[0].doepctl |= OTG_FS_EPENA | OTG_FS_CNAK;

		/* Unmask EP0 interrupts */
		otg_fs->daintmsk = OTG_FS_IEPM0 | OTG_FS_OEPM0;

		DCC_LOG2(LOG_TRACE, "TX[0]: addr=%04x size=%d",
				 OTG_FS_TX0FSA_GET(otg_fs->dieptxf0) * 4,
				 OTG_FS_TX0FD_GET(otg_fs->dieptxf0) * 4);
	} else {
		uint32_t depctl;

		if (info->addr & USB_ENDPOINT_IN) {
			DCC_LOG(LOG_TRACE, "IN ENDPOINT");

			if ((info->attr & 0x03) == ENDPOINT_TYPE_BULK) {
				ep->xfr_max = 6 * mxpktsz;
			} else {
				ep->xfr_max = mxpktsz;
			}

			__ep_pktbuf_alloc(drv, ep_id, ep->xfr_max);
			depctl = otg_fs->inep[ep_id].diepctl;
		} else {
			DCC_LOG(LOG_TRACE, "OUT ENDPOINT");
			depctl = otg_fs->outep[ep_id].doepctl;
		}

		depctl &= ~(OTG_FS_MPSIZ_MSK | OTG_FS_EPTYP_MSK | OTG_FS_TXFNUM_MSK);

		depctl |= OTG_FS_MPSIZ_SET(mxpktsz);
		depctl |= OTG_FS_SD0PID | OTG_FS_USBAEP;

		switch (info->attr & 0x03) {
		case ENDPOINT_TYPE_CONTROL:
			DCC_LOG(LOG_TRACE, "ENDPOINT_TYPE_CONTROL");
			depctl |= OTG_FS_EPTYP_SET(OTG_FS_EPTYP_CTRL);
			break;

		case ENDPOINT_TYPE_ISOCHRONOUS:
			DCC_LOG(LOG_TRACE, "ENDPOINT_TYPE_ISOCHRONOUS");
			depctl |= OTG_FS_EPTYP_SET(OTG_FS_EPTYP_ISOC);
			break;

		case ENDPOINT_TYPE_BULK:
			DCC_LOG(LOG_TRACE, "ENDPOINT_TYPE_BULK");
			depctl |= OTG_FS_EPTYP_SET(OTG_FS_EPTYP_BULK);
			break;

		case ENDPOINT_TYPE_INTERRUPT:
			DCC_LOG(LOG_TRACE, "ENDPOINT_TYPE_INTERRUPT");
			depctl |= OTG_FS_EPTYP_SET(OTG_FS_EPTYP_INT);
			break;
		}

		if (info->addr & USB_ENDPOINT_IN) {
			/* Activate IN endpoint */
			otg_fs->inep[ep_id].diepctl = depctl | OTG_FS_TXFNUM_SET(ep_id);

			/* Enable endpoint interrupt */
			otg_fs->daintmsk |= OTG_FS_IEPM(ep_id);
		} else {
			uint32_t rxfsiz;
			uint32_t pktcnt;

			/* get the size of the RX fifio */
			rxfsiz = otg_fs->grxfsiz * 4;

			pktcnt = rxfsiz / mxpktsz;

			/* cache this value to be reused */
			ep->doeptsiz = OTG_FS_PKTCNT_SET(pktcnt) |
				OTG_FS_XFRSIZ_SET(pktcnt * mxpktsz);

			/* Prepare EP_OUT to receive */
			otg_fs->outep[ep_id].doeptsiz = ep->doeptsiz;

			/* EP enable */
			otg_fs->outep[ep_id].doepctl = depctl | OTG_FS_EPENA | OTG_FS_CNAK;

			/* Enable endpoint interrupt */
			otg_fs->daintmsk |= OTG_FS_OEPM(ep_id);
		}

	}

	/* 2. Once the endpoint is activated, the core starts decoding the
	   tokens addressed to that endpoint and sends out a valid
	   handshake for each valid token received for the
	   endpoint. */

	return ep_id;
}

#define OTG_FS_DP STM32F_GPIOA, 12
#define OTG_FS_DM STM32F_GPIOA, 11
#define OTG_FS_VBUS STM32F_GPIOA, 9
#define OTG_FS_ID STM32F_GPIOA, 10

static void otg_fs_io_init(void)
{
	DCC_LOG(LOG_MSG, "Enabling GPIO clock...");
	stm32f_gpio_clock_en(STM32F_GPIOA);

	DCC_LOG(LOG_MSG, "Configuring GPIO pins...");

	stm32f_gpio_af(OTG_FS_DP, GPIO_AF10);
	stm32f_gpio_af(OTG_FS_DM, GPIO_AF10);
	stm32f_gpio_af(OTG_FS_VBUS, GPIO_AF10);
	stm32f_gpio_af(OTG_FS_ID, GPIO_AF10);

	stm32f_gpio_mode(OTG_FS_DP, ALT_FUNC, PUSH_PULL | SPEED_HIGH);
	stm32f_gpio_mode(OTG_FS_DM, ALT_FUNC, PUSH_PULL | SPEED_HIGH);
	stm32f_gpio_mode(OTG_FS_VBUS, ALT_FUNC, SPEED_LOW);
	stm32f_gpio_mode(OTG_FS_ID, ALT_FUNC, PUSH_PULL | SPEED_HIGH);
}

void otg_fs_vbus_connect(bool connect)
{
	if (connect)
		stm32f_gpio_mode(OTG_FS_VBUS, ALT_FUNC, SPEED_LOW);
	else
		stm32f_gpio_mode(OTG_FS_VBUS, INPUT, 0);
}

void otg_fs_connect(struct stm32f_otg_fs * otg_fs)
{
	/* Connect device */
	otg_fs->dctl &= ~OTG_FS_SDIS;
	udelay(3000);
}

void otg_fs_disconnect(struct stm32f_otg_fs * otg_fs)
{
	otg_fs->dctl |= OTG_FS_SDIS;
	udelay(3000);
}

void otg_fs_power_on(struct stm32f_otg_fs * otg_fs)
{
	struct stm32f_rcc * rcc = STM32F_RCC;

	otg_fs_vbus_connect(true);

	DCC_LOG(LOG_TRACE, "Enabling USB FS clock...");
	rcc->ahb2enr |= RCC_OTGFSEN;

	/* Initialize as a device */
	stm32f_otg_fs_device_init(otg_fs);

	otg_fs_connect(otg_fs);

	/* Enable Cortex interrupt */
	cm3_irq_enable(STM32F_IRQ_OTG_FS);
}

void otg_fs_power_off(struct stm32f_otg_fs * otg_fs)
{
	struct stm32f_rcc * rcc = STM32F_RCC;

//	usb->cntr = USB_FRES;
	/* Removing any spurious pending interrupts */
//	usb->istr = 0;
	otg_fs_disconnect(otg_fs);
//	otg_fs_vbus_connect(false);

//	usb->cntr = USB_FRES | USB_PDWN;

	DCC_LOG(LOG_TRACE, "Disabling USB device clock...");
	rcc->ahb2enr &= ~RCC_OTGFSEN;

	/* disabling IO pins */
	stm32f_gpio_mode(OTG_FS_DP, INPUT, 0);
	stm32f_gpio_mode(OTG_FS_DM, INPUT, 0);

}

int stm32f_otg_fs_dev_init(struct stm32f_otg_drv * drv, usb_class_t * cl,
		const usb_class_events_t * ev)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;

	drv->cl = cl;
	drv->ev = ev;

	DCC_LOG1(LOG_TRACE, "ev=0x%08x", drv->ev);

	otg_fs_power_off(otg_fs);

	udelay(10000);

	/* Initialize IO pins */
	otg_fs_io_init();

	otg_fs_power_on(otg_fs);

	DCC_LOG(LOG_TRACE, "[ATTACHED]");
	return 0;
}

/* 
 *  ---------------------------------------------------------------------------
 * Interrupt handling
 * ---------------------------------------------------------------------------
 */

static void stm32f_otg_dev_ep_out(struct stm32f_otg_drv * drv, 
								  int ep_id, int len)
{
	struct stm32f_otg_ep * ep = &drv->ep[ep_id];

	DCC_LOG1(LOG_TRACE, "ep_id=%d", ep_id);

	ep->xfr_rem = len;

	/* call class endpoint callback */
	ep->on_out(drv->cl, ep_id, len);
}


static void stm32f_otg_dev_ep_in(struct stm32f_otg_drv * drv, int ep_id)
{
	struct stm32f_otg_ep * ep = &drv->ep[ep_id];

	DCC_LOG1(LOG_TRACE, "ep_id=%d", ep_id);

	/* call class endpoint callback */
	ep->on_in(drv->cl, ep_id);
}

static void stm32f_otg_dev_ep0_in(struct stm32f_otg_drv * drv)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	struct stm32f_otg_ep * ep = &drv->ep[0];

	if (ep->state == EP_WAIT_STATUS_IN) {
		struct usb_request * req = &drv->req;
		void * dummy = NULL;

		ep->on_setup(drv->cl, req, dummy);
		ep->state = EP_IDLE;
		DCC_LOG(LOG_INFO, "EP0 [IDLE]");
		return;
	}


	if (ep->xfr_rem == 0) {
		otg_fs->diepempmsk &= ~(1 << 0);
		/* Prepare to receive */
		otg_fs->outep[0].doeptsiz = ep->doeptsiz;
		/* EP enable */
		otg_fs->outep[0].doepctl |= OTG_FS_EPENA | OTG_FS_CNAK;
		ep->state = EP_IN_DATA;
		DCC_LOG(LOG_INFO, "EP0 [IN_DATA]");
		return;
	} 
	
	if (stm32f_otg_fs_txf_setup(otg_fs, 0, ep->xfr_rem) < 0) {
		DCC_LOG(LOG_ERROR, "stm32f_otg_fs_txf_setup() failed!");
	} else {
		/* umask FIFO empty interrupt */
		otg_fs->diepempmsk |= (1 << 0);
	}
}

static void stm32f_otg_dev_ep0_out(struct stm32f_otg_drv * drv)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	struct stm32f_otg_ep * ep = &drv->ep[0];

	/* last and only transfer */
	if (ep->state == EP_OUT_DATA_LAST) {
		ep->state = EP_WAIT_STATUS_IN;
		DCC_LOG(LOG_INFO, "EP0 [WAIT_STATUS_IN]");
		__ep_zlp_send(otg_fs, 0);
		return;
	}
}

static void stm32f_otg_dev_ep0_setup(struct stm32f_otg_drv * drv)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	struct usb_request * req = &drv->req;
	struct stm32f_otg_ep * ep = &drv->ep[0];
	int len;

	DCC_LOG3(LOG_INFO, "type=0x%02x request=0x%02x len=%d ",
			req->type, req->request, req->length);

	/* No-Data control SETUP transaction */
	if (req->length == 0) {
		struct usb_request * req = &drv->req;
		void * dummy = NULL;

		if (((req->request << 8) | req->type) == STD_SET_ADDRESS) {
			DCC_LOG(LOG_TRACE, "address set!");
			stm32f_otg_fs_addr_set(otg_fs, req->value);
		}
		ep->on_setup(drv->cl, req, dummy);
		ep->state = EP_IDLE;
		__ep_zlp_send(otg_fs, 0);
		return;
	}

	if (req->type & 0x80) {
		/* Control Read SETUP transaction (IN Data Phase) */
		DCC_LOG(LOG_INFO, "EP0 [SETUP] IN Dev->Host");
		ep->xfr_ptr = NULL;
		len = ep->on_setup(drv->cl, req, (void *)&ep->xfr_ptr);
		ep->xfr_rem = MIN(req->length, len);
		DCC_LOG1(LOG_INFO, "EP0 data lenght = %d", ep->xfr_rem);
		/* prepare fifo to transmit */
		if (stm32f_otg_fs_txf_setup(otg_fs, 0, ep->xfr_rem) < 0) {
			DCC_LOG(LOG_WARNING, "stm32f_otg_fs_txf_setup() failed!");
		} else {
			/* umask FIFO empty interrupt */
			otg_fs->diepempmsk |= (1 << 0);
		}
	} else {
		/* Control Write SETUP transaction (OUT Data Phase) */
		ep->xfr_ptr = ep->xfr_buf;
		ep->xfr_rem = req->length;

		DCC_LOG1(LOG_INFO, "xfr_ptr=0x%08x", ep->xfr_ptr);

		if (ep->xfr_rem > ep->xfr_buf_len) {
			ep->xfr_rem = ep->xfr_buf_len;
			DCC_LOG(LOG_ERROR, "transfer to large to fit in the buffer!");
		}

		if (ep->xfr_rem < ep->xfr_max) {
			/* last and only transfer */
			ep->state = EP_OUT_DATA_LAST;
			DCC_LOG(LOG_INFO, "EP0 [OUT_DATA_LAST] OUT Host->Dev!!!!");
		} else {
			ep->state = EP_OUT_DATA;
			DCC_LOG(LOG_INFO, "EP0 [OUT_DATA] OUT Host->Dev!!!!");
		}

		DCC_LOG(LOG_INFO, "Prepare to receive");

		/* Prepare to receive */
		otg_fs->outep[0].doeptsiz = OTG_FS_STUPCNT_SET(0) |
			OTG_FS_PKTCNT_SET(1) | OTG_FS_XFRSIZ_SET(ep->xfr_max);

		/* EP enable */
		otg_fs->outep[0].doepctl |= OTG_FS_EPENA | OTG_FS_CNAK;
	}

}

static void stm32f_otg_dev_reset(struct stm32f_otg_drv * drv)
{
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	uint32_t siz;
	int i;

	DCC_LOG(LOG_INFO, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

	/* Clear the Remote Wake-up Signaling */
	otg_fs->dctl &= ~OTG_FS_RWUSIG;

	/* Clear all pending interrupts */
	otg_fs->diepmsk = 0;
	otg_fs->doepmsk = 0;
	otg_fs->daint = 0xffffffff;
	otg_fs->daintmsk = 0;
	otg_fs->diepempmsk = 0;
	for (i = 0; i < 4; i++) {
		otg_fs->inep[i].diepint = 0xff;
		otg_fs->outep[i].doepint = 0xff;
		otg_fs->inep[i].dieptsiz = 0;

		drv->ep[i].xfr_rem = 0;
	}

	/* Flush the Tx FIFO */
	stm32f_otg_fs_txfifo_flush(otg_fs, TXFIFO_ALL);
	/* Flush the Rx FIFO */
	stm32f_otg_fs_rxfifo_flush(otg_fs);


	/* Reset global interrupts mask */
	otg_fs->gintmsk =  OTG_FS_SRQIM | OTG_FS_OTGINT | OTG_FS_WUIM |
			OTG_FS_USBRSTM | OTG_FS_ENUMDNEM |
			OTG_FS_ESUSPM | OTG_FS_USBSUSPM;

	/* Reset Device Address */
	otg_fs->dcfg &= ~OTG_FS_DAD_MSK;

	/*  Set global IN NAK */
	otg_fs->dctl |= OTG_FS_SGINAK;

	/* Endpoint initialization on USB reset */

	/* 1. Set the NAK bit for all OUT endpoints
	   – SNAK = 1 in OTG_FS_DOEPCTLx (for all OUT endpoints) */
	for (i = 0; i < 4; i++) {
		otg_fs->outep[i].doepctl = OTG_FS_SNAK;
	}

	/* 2. Unmask the following interrupt bits
	   – INEP0 = 1 in OTG_FS_DAINTMSK (control 0 IN endpoint)
	   – OUTEP0 = 1 in OTG_FS_DAINTMSK (control 0 OUT endpoint)
	   – STUP = 1 in DOEPMSK
	   – XFRC = 1 in DOEPMSK
	   – XFRC = 1 in DIEPMSK
	   – TOC = 1 in DIEPMSK */
	otg_fs->doepmsk = OTG_FS_STUPM | OTG_FS_XFRCM | OTG_FS_EPDM;
	otg_fs->diepmsk = OTG_FS_TOM | OTG_FS_EPDM | OTG_FS_XFRCM;

	/* 3. Set up the Data FIFO RAM for each of the FIFOs
	   – Program the OTG_FS_GRXFSIZ register, to be able to receive
	   control OUT data and setup data. If thresholding is not enabled,
	   at a minimum, this must be equal to 1 max packet size of
	   control endpoint 0 + 2 Words (for the status of the control OUT
	   data packet) + 10 Words (for setup packets).
	   - Program the OTG_FS_TX0FSIZ register (depending on the FIFO number
	   chosen) to be able to transmit control IN data. At a minimum, this
	   must be equal to 1 max packet size of control endpoint 0. */

	/* reset fifo memory (packet buffer) allocation pointer */
	drv->fifo_addr = 0;
	/* initialize RX fifo size */
	siz = OTG_FS_RX_FIFO_SIZE;
	otg_fs->grxfsiz = siz / 4;
	/* update fifo memory allocation pointer */
	drv->fifo_addr += siz;

	DCC_LOG2(LOG_TRACE, "   RX: addr=%04x size=%d",
			 0, otg_fs->grxfsiz * 4);

	drv->ev->on_reset(drv->cl);
}

/* Private USB device driver data */
struct stm32f_otg_drv stm32f_otg_fs_drv0;

void stm32f_otg_fs_isr(void)
{
	struct stm32f_otg_drv * drv = &stm32f_otg_fs_drv0;
	struct stm32f_otg_fs * otg_fs = STM32F_OTG_FS;
	uint32_t gintsts;
	uint32_t ep_intr;

	gintsts = otg_fs->gintsts & otg_fs->gintmsk;

	DCC_LOG1(LOG_MSG, "GINTSTS=0x%08x", gintsts);

	if (gintsts & OTG_FS_SRQINT) {
		/* Session request/new session detected interrupt */
		DCC_LOG(LOG_TRACE, "<SRQINT>  [POWERED]");
		otg_fs->gintmsk |= OTG_FS_WUIM | OTG_FS_USBRSTM | OTG_FS_ENUMDNEM |
			OTG_FS_ESUSPM | OTG_FS_USBSUSPM;
	}

	if (gintsts & OTG_FS_PTXFE) {
		DCC_LOG(LOG_TRACE, "<PTXFE>");
	}

	if (gintsts & OTG_FS_OTGINT) {
		uint32_t gotgint = otg_fs->gotgint;
		DCC_LOG(LOG_INFO, "<OTGINT>");
		if (gotgint & OTG_FS_OTGINT) {
			DCC_LOG(LOG_INFO, "<SEDET>  [ATTACHED]");
			otg_fs->gintmsk = OTG_FS_SRQIM | OTG_FS_OTGINT;
		}
		otg_fs->gotgint = gotgint;
	}

	if (gintsts & OTG_FS_GONAKEFF) {
		DCC_LOG(LOG_TRACE, "<GONAKEFF>");
		otg_fs->gintmsk &= ~OTG_FS_GONAKEFFM;
	}

	/* RxFIFO non-empty */
	if (gintsts & OTG_FS_RXFLVL) {
		uint32_t grxsts;
		int epnum;
		int len;
		int stat;

		/* 1. On catching an RXFLVL interrupt (OTG_FS_GINTSTS register),
		   the application must read the Receive status pop
		   register (OTG_FS_GRXSTSP). */
		grxsts = otg_fs->grxstsp;

		epnum = OTG_FS_EPNUM_GET(grxsts);
		len = OTG_FS_BCNT_GET(grxsts);
		(void)len;
		stat = OTG_FS_PKTSTS_GET(grxsts);
		(void)stat;

		DCC_LOG3(LOG_INFO, "[%d] <RXFLVL> len=%d status=%d", epnum, len, stat);

		if (epnum == 0) {

			/* 3. If the received packet’s byte count is not 0, the byte count
			   amount of data is popped from the receive Data FIFO and stored in
			   memory. If the received packet byte count is 0, no data is popped
			   from the receive data FIFO. */
			switch (grxsts & OTG_FS_PKTSTS_MSK) {
			case OTG_FS_PKTSTS_GOUT_NACK:
				/* Global OUT NAK (triggers an interrupt) */
				DCC_LOG1(LOG_TRACE, "[%d] <RXFLVL> <GOUT_NACK>", epnum);
				break;
			case OTG_FS_PKTSTS_OUT_DATA_UPDT: {
				/* OUT data packet received */
				DCC_LOG1(LOG_TRACE, "[%d] <RXFLVL> <OUT_DATA_UPDT>", epnum);
				__ep_rx_pop(drv, 0, len);
				break;
			}
			case OTG_FS_PKTSTS_OUT_XFER_COMP:
				DCC_LOG1(LOG_TRACE, "[%d] <RXFLVL> <OUT_XFER_COMP>", epnum);
				break;
			case OTG_FS_PKTSTS_SETUP_COMP:
				/* SETUP transaction completed (triggers an interrupt) */
				DCC_LOG1(LOG_INFO, "[%d] <RXFLVL> <SETUP_COMP>", epnum);
				break;
			case OTG_FS_PKTSTS_SETUP_UPDT:
				/* SETUP data packet received */
				DCC_LOG1(LOG_INFO, "[%d] <RXFLVL> <SETUP_UPDT>", epnum);

				if (len != 8) {
					DCC_LOG(LOG_ERROR, "setup data len != 8!");
				}

				/* Copy the received setup packet into the setup buffer */
				__copy_from_pktbuf(&drv->req, &otg_fs->dfifo[0].pop, len);

				DCC_LOG3(LOG_INFO, "type=0x%02x request=0x%02x len=%d ",
						drv->req.type, drv->req.request, drv->req.length);

				break;
			}
		} else {
			switch (grxsts & OTG_FS_PKTSTS_MSK) {
			case OTG_FS_PKTSTS_OUT_DATA_UPDT:
				/* OUT data packet received */
				DCC_LOG1(LOG_TRACE, "[%d] <RXFLVL> <OUT_DATA_UPDT>", epnum);
				/* 2. The application can mask the RXFLVL interrupt (in
				   OTG_FS_GINTSTS) by writing to RXFLVL = 0 (in
				   OTG_FS_GINTMSK), until it has read the packet from
				   the receive FIFO. */
				otg_fs->gintmsk &= ~OTG_FS_RXFLVLM;
				/* Enable SOF interrupts */
//				otg_fs->gintmsk |=  OTG_FS_SOFM;
//				__ep_rx_pop(drv, 0, len);
				stm32f_otg_dev_ep_out(drv, epnum, len);
				break;
			case OTG_FS_PKTSTS_OUT_XFER_COMP:
				DCC_LOG1(LOG_TRACE, "[%d] <RXFLVL> <OUT_XFER_COMP>", epnum);
				DCC_LOG2(LOG_TRACE, "[%d] doeptsiz=%08x", epnum, 
						 otg_fs->outep[epnum].doeptsiz);	 
				/* Prepare to receive more */
				otg_fs->outep[epnum].doeptsiz = drv->ep[epnum].doeptsiz;
				/* EP enable */
				/* FIXME: not clearing the NAK here reduces the performance,
				   but we have to garantee tha the class driver
				   periodically remove the packets from 
				   the fifo. Otherwise the EP0 will not receive its
				   packets and we end up with a deadlock situation */

//				otg_fs->outep[epnum].doepctl |= OTG_FS_EPENA | OTG_FS_CNAK;
				/* Disable SOF interrupts */
//				otg_fs->gintmsk &= ~OTG_FS_SOFM;
				break;
			}
		}

		/* 5. After the data payload is popped from the receive FIFO, the
		   RXFLVL interrupt (OTG_FS_GINTSTS) must be unmasked. */
		//	otg_fs->gintmsk |= OTG_FS_RXFLVLM;
	}

	if (gintsts & OTG_FS_SOF) {
		DCC_LOG(LOG_MSG, "<SOF>");
#if 0
		if (--dev->rx_tmr == 0) {
			int i;
			/* Disable SOF interrupts */
			otg_fs->gintmsk &= ~OTG_FS_SOFM;

			DCC_LOG(LOG_TRACE, "RX timeout"); 
			/* pop data from fifo */
			for (i = 0; i < dev->ep1_rx.len; i++) {
				(void)otg_fs->dfifo[EP_OUT].pop;
			}
			ep->xfer_rem = 0;
			ep->xfer_len = 0;
			/* Reenable RX fifo interrupts */
			otg_fs->gintmsk |= OTG_FS_RXFLVLM;
		}
#endif
	}

	if (gintsts & OTG_FS_WKUPINT) {
		DCC_LOG(LOG_TRACE, "<WKUPINT>");
		/* disable resume/wakeup interrupts */
	}

	if (gintsts & OTG_FS_USBRST ) {
		/* end of bus reset */
		DCC_LOG(LOG_TRACE, "<USBRST> --------------- [DEFAULT]");
		stm32f_otg_dev_reset(drv);
	}

	if (gintsts & OTG_FS_ENUMDNE) {
		DCC_LOG(LOG_TRACE, "<ENUMDNE>");
		/* Unmask global interrupts */
		otg_fs->gintmsk |=  OTG_FS_IEPINTM | OTG_FS_OEPINTM |
			OTG_FS_IISOIXFRM | OTG_FS_IISOOXFRM | OTG_FS_RXFLVLM;
		/*  Clear global IN NAK */
		otg_fs->dctl |= OTG_FS_CGINAK;
	}

	if (gintsts & OTG_FS_ESUSP) {
		DCC_LOG(LOG_INFO, "<ESUSP>");
	}

	if (gintsts & OTG_FS_USBSUSP) {
		DCC_LOG(LOG_INFO, "<USBSUSP>");
	}

	if (gintsts & OTG_FS_IEPINT) {
		uint32_t diepmsk;
		uint32_t diepint;
		uint32_t diepempmsk;
		uint32_t msk;

		DCC_LOG(LOG_INFO, "<IEPINT>");

		ep_intr = (otg_fs->daint & otg_fs->daintmsk);
		diepmsk = otg_fs->diepmsk;
		diepempmsk = otg_fs->diepempmsk;

		if (ep_intr & OTG_FS_IEPINT0) {
			/* add the Transmit FIFO empty bit to the mask */
			msk = diepmsk | ((diepempmsk >> 0) & 0x1) << 7;
			diepint = otg_fs->inep[0].diepint & msk;
			if (diepint & OTG_FS_XFRC) {
				DCC_LOG(LOG_TRACE, "[0] <IEPINT> <XFRC>");
				stm32f_otg_dev_ep0_in(drv);
			} else if (diepint & OTG_FS_TXFE) {
				DCC_LOG(LOG_TRACE, "[0] <IEPINT> <TXFE>");
				__ep_tx_push(drv, 0);
			}
			/* clear interrupts */
			otg_fs->inep[0].diepint = diepint;
		}

		if (ep_intr & OTG_FS_IEPINT1) {
			/* add the Transmit FIFO empty bit to the mask */
			msk = diepmsk | ((diepempmsk >> 1) & 0x1) << 7;
			diepint = otg_fs->inep[1].diepint & msk;
			if (diepint & OTG_FS_XFRC) {
				DCC_LOG(LOG_TRACE, "[1] <IEPINT> <XFRC>");
				stm32f_otg_dev_ep_in(drv, 1);
			}
			if (diepint & OTG_FS_TXFE) {
				DCC_LOG(LOG_TRACE, "[1] <IEPINT> <TXFE>");
				__ep_tx_push(drv, 1);
			}
			/* clear interrupts */
			otg_fs->inep[1].diepint = diepint;
		}

		if (ep_intr & OTG_FS_IEPINT2) {
			/* add the Transmit FIFO empty bit to the mask */
			msk = diepmsk | ((diepempmsk >> 2) & 0x1) << 7;
			diepint = otg_fs->inep[2].diepint & msk;
			if (diepint & OTG_FS_XFRC) {
				DCC_LOG(LOG_TRACE, "[2] <IEPINT> <XFRC>");
				stm32f_otg_dev_ep_in(drv, 2);
			}
			if (diepint & OTG_FS_TXFE) {
				DCC_LOG(LOG_TRACE, "[2] <IEPINT> <TXFE>");
				__ep_tx_push(drv, 2);
			}
			otg_fs->inep[2].diepint = diepint;
		}

		if (ep_intr & OTG_FS_IEPINT3) {
			/* add the Transmit FIFO empty bit to the mask */
			msk = diepmsk | ((diepempmsk >> 3) & 0x1) << 7;
			diepint = otg_fs->inep[3].diepint & msk;
			if (diepint & OTG_FS_XFRC) {
				DCC_LOG(LOG_TRACE, "[3] <IEPINT> <XFRC>");
				stm32f_otg_dev_ep_in(drv, 3);
			}
			if (diepint & OTG_FS_TXFE) {
				DCC_LOG(LOG_TRACE, "[3] <IEPINT> <TXFE>");
				__ep_tx_push(drv, 3);
			}
			otg_fs->inep[3].diepint = diepint;
		}
	}

	if (gintsts & OTG_FS_OEPINT) {
		ep_intr = (otg_fs->daint & otg_fs->daintmsk);

		DCC_LOG(LOG_INFO, "<OEPINT>");

		if (ep_intr & OTG_FS_OEPINT0) {
			uint32_t doepint;

			doepint = otg_fs->outep[0].doepint & otg_fs->doepmsk;

			if (doepint & OTG_FS_XFRC) {
				DCC_LOG(LOG_INFO, "[0] <OEPINT> <OUT XFRC>");
				stm32f_otg_dev_ep0_out(drv);
			}
			if (doepint & OTG_FS_EPDISD) {
				DCC_LOG(LOG_INFO, "[0] <OEPINT> <EPDISD>");
			}
			if (doepint & OTG_FS_STUP) {

				DCC_LOG(LOG_INFO, "[0] <OEPINT> <STUP>");
#if 0
				uint32_t doeptsiz;
				int pktcnt;
				int stupcnt;
				int xfrsiz;


				doeptsiz = otg_fs->outep[0].doeptsiz;
				stupcnt = OTG_FS_STUPCNT_GET(doeptsiz);
				(void)stupcnt;
				pktcnt = OTG_FS_PKTCNT_GET(doeptsiz);
				(void)pktcnt;
				xfrsiz = OTG_FS_XFRSIZ_GET(doeptsiz);
				(void)xfrsiz;

				DCC_LOG3(LOG_INFO, "stupcnt=%d pktcnt=%d xfrsiz=%d",
						stupcnt, pktcnt, xfrsiz);
#endif
				stm32f_otg_dev_ep0_setup(drv);
			}
			/* clear interrupts */
			otg_fs->outep[0].doepint = doepint;
		}

		if (ep_intr & OTG_FS_OEPINT1) {
			uint32_t doepint;

			doepint = otg_fs->outep[1].doepint & otg_fs->doepmsk;

			if (doepint & OTG_FS_XFRC) {
				DCC_LOG(LOG_TRACE, "[1] <OEPINT> <OUT XFRC>");
//				stm32f_otg_dev_ep_out(drv, 1);
			}
			if (doepint & OTG_FS_EPDISD) {
				DCC_LOG(LOG_TRACE, "[1] <OEPINT> <EPDISD>");
			}
			if (doepint & OTG_FS_STUP) {
				DCC_LOG(LOG_TRACE, "[1] <OEPINT> <STUP>");
			}
			/* clear interrupts */
			otg_fs->outep[1].doepint = doepint;
		}

		if (ep_intr & OTG_FS_OEPINT2) {
			uint32_t doepint;

			doepint = otg_fs->outep[2].doepint & otg_fs->doepmsk;

			if (doepint & OTG_FS_XFRC) {
				DCC_LOG(LOG_TRACE, "[2] <OEPINT> <OUT XFRC>");
//				stm32f_otg_dev_ep_out(drv, 2);
			}
			if (doepint & OTG_FS_EPDISD) {
				DCC_LOG(LOG_TRACE, "[2] <OEPINT> <EPDISD>");
			}
			if (doepint & OTG_FS_STUP) {
				DCC_LOG(LOG_TRACE, "[2] <OEPINT> <STUP>");
			}
			/* clear interrupts */
			otg_fs->outep[1].doepint = doepint;
		}

		if (ep_intr & OTG_FS_OEPINT3) {
			uint32_t doepint;

			doepint = otg_fs->outep[3].doepint & otg_fs->doepmsk;

			if (doepint & OTG_FS_XFRC) {
				DCC_LOG(LOG_TRACE, "[3] <OEPINT> <OUT XFRC>");
//				stm32f_otg_dev_ep_out(drv, 3);
			}
			if (doepint & OTG_FS_EPDISD) {
				DCC_LOG(LOG_TRACE, "[3] <OEPINT> <EPDISD>");
			}
			if (doepint & OTG_FS_STUP) {
				DCC_LOG(LOG_TRACE, "[3] <OEPINT> <STUP>");
			}

			/* clear interrupts */
			otg_fs->outep[1].doepint = doepint;
		}
	}

	if (gintsts & OTG_FS_IISOIXFR) {
		DCC_LOG(LOG_TRACE, "<IISOIXFR>");
	}

	if (gintsts & OTG_FS_INCOMPISOOUT) {
		DCC_LOG(LOG_TRACE, "<INCOMPISOOUT>");
	}

	if (gintsts & OTG_FS_MMIS) {
		DCC_LOG(LOG_TRACE, "<MMIS>");
	}

	/* clear pending interrupts */
	otg_fs->gintsts = gintsts;
}


/* USB device operations */
const struct usb_dev_ops stm32f_otg_fs_ops = {
	.dev_init = (usb_dev_init_t)stm32f_otg_fs_dev_init,
	.ep_tx_start= (usb_dev_ep_tx_start_t)stm32f_otg_dev_ep_tx_start,
	.ep_init = (usb_dev_ep_init_t)stm32f_otg_dev_ep_init,
	.ep_stall = (usb_dev_ep_stall_t)stm32f_otg_dev_ep_stall,
	.ep_zlp_send = (usb_dev_ep_zlp_send_t)stm32f_otg_dev_ep_zlp_send,
	.ep_nak = (usb_dev_ep_nak_t)stm32f_otg_dev_ep_nak,
	.ep_pkt_recv = (usb_dev_ep_pkt_recv_t)stm32f_otg_dev_ep_pkt_recv
};

/* USB device driver */
const struct usb_dev stm32f_otg_fs_dev = {
	.priv = (void *)&stm32f_otg_fs_drv0,
	.op = &stm32f_otg_fs_ops
};

#endif /* STM32F2X */