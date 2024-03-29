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

#include <sys/stm32f.h>

#include <stdio.h>
#include <sys/param.h>

#include <sys/dcclog.h>
#include <sys/delay.h>

#ifdef STM32F_OTG_FS

void stm32f_otg_fs_core_reset(struct stm32f_otg_fs * otg_fs)
{
	DCC_LOG(LOG_INFO, "...");

	/* Wait for AHB master IDLE state. */
	while (!(otg_fs->grstctl & OTG_FS_AHBIDL)) {
		udelay(3);
	}

	/* Core Soft Reset */
	otg_fs->grstctl = OTG_FS_CSRST;
	do {
		udelay(3);
	} while (otg_fs->grstctl & OTG_FS_CSRST);

	/* Wait for 3 PHY Clocks*/
	udelay(3);
}

void stm32f_otg_fs_txfifo_flush(struct stm32f_otg_fs * otg_fs, 
								unsigned int num)
{
	DCC_LOG1(LOG_INFO, "%02x", num);

	otg_fs->grstctl = OTG_FS_TXFFLSH | OTG_FS_TXFIFO_SET(num);
	do {
	} while (otg_fs->grstctl & OTG_FS_TXFFLSH);
	/* Wait for 3 PHY Clocks ?? */
	udelay(3);
}

void stm32f_otg_fs_rxfifo_flush(struct stm32f_otg_fs * otg_fs)
{
	DCC_LOG(LOG_INFO, "...");

	otg_fs->grstctl = OTG_FS_RXFFLSH;
	do {
	} while (otg_fs->grstctl & OTG_FS_RXFFLSH);
	/* Wait for 3 PHY Clocks ?? */
	udelay(3);
}

void stm32f_otg_fs_addr_set(struct stm32f_otg_fs * otg_fs, unsigned int addr)
{
	DCC_LOG1(LOG_INFO, "addr=0x%02x", addr);

	uint32_t dcfg;
	/*  Endpoint initialization on SetAddress command */

	/* This section describes what the application must do when it 
	   receives a SetAddress command in a SETUP packet.
	   1. Program the OTG_FS_DCFG register with the device address 
	   received in the SetAddress command */
	dcfg = otg_fs->dcfg; 
	dcfg &= ~OTG_FS_DAD_MSK;
	dcfg |= OTG_FS_DAD_SET(addr);
	otg_fs->dcfg = dcfg;

	/* 2. Program the core to send out a status IN packet */
}

void stm32f_otg_fs_ep_disable(struct stm32f_otg_fs * otg_fs, unsigned int addr)
{
	int ep_id = addr & 0x7f;
	int input = addr & 0x80;

	DCC_LOG2(LOG_INFO, "ep_id=%d %s", ep_id, input ? "IN" : "OUT");

	/* Diable endpoint interrupt */
	otg_fs->daintmsk &= ~OTG_FS_IEPM(ep_id);

	/* Disable endpoint */
	if (input) {
		otg_fs->inep[ep_id].diepctl &= ~OTG_FS_USBAEP;
	} else {
		otg_fs->outep[ep_id].doepctl &= ~OTG_FS_USBAEP;
	}
}

void stm32f_otg_fs_ep_out_start(struct stm32f_otg_fs * otg_fs,
								unsigned int addr, unsigned int mpsiz)
{
	int ep_id = addr & 0x7f;
	uint32_t rxfsiz;
	uint32_t pktcnt;

	rxfsiz = otg_fs->grxfsiz * 4;
	pktcnt = rxfsiz / mpsiz;

	/* Prepare EP_OUT to receive */
	otg_fs->outep[ep_id].doeptsiz = OTG_FS_PKTCNT_SET(pktcnt) |
		OTG_FS_XFRSIZ_SET(pktcnt * mpsiz);
	/* EP enable */
	otg_fs->outep[ep_id].doepctl |= OTG_FS_EPENA | OTG_FS_CNAK;
}

const uint8_t stm32f_otg_fs_ep0_mpsiz_lut[] = {
	64, 32, 16, 8
};

int stm32f_otg_fs_txf_setup(struct stm32f_otg_fs * otg_fs, 
							unsigned int ep_id, unsigned int len)
{
	uint32_t deptsiz;
	uint32_t depctl;
	uint32_t mpsiz;
	uint32_t xfrsiz;
	uint32_t pktcnt;

	deptsiz = otg_fs->inep[ep_id].dieptsiz;
	xfrsiz = OTG_FS_XFRSIZ_GET(deptsiz);
	pktcnt = OTG_FS_PKTCNT_GET(deptsiz);
	if ((xfrsiz == 0) && (pktcnt)) {
		DCC_LOG3(LOG_WARNING, "ep_id=%d len=%d %d outstanding packets in FIFO", 
				 ep_id, len, pktcnt);
		return -1;
	}

	depctl = otg_fs->inep[ep_id].diepctl;
	if (ep_id == 0)
		mpsiz = OTGFS_EP0_MPSIZ_GET(depctl);
	else
		mpsiz = OTG_FS_MPSIZ_GET(depctl);

	if (len > 0) {
		/* XXX: check whether to get rid of this division or not,
		 if the CM3 div is used it is not necessary.... */
		pktcnt = (len + (mpsiz - 1)) / mpsiz;
		if (pktcnt > 7) {
			pktcnt = 7;
			xfrsiz = 7 * mpsiz;
		} else {
			xfrsiz = len;
		}
	} else {
		/* zero lenght packet */
		pktcnt = 1;
		xfrsiz = 0;
	}

	DCC_LOG3(LOG_INFO, "ep_id=%d pktcnt=%d xfrsiz=%d", ep_id, pktcnt, xfrsiz);

	otg_fs->inep[ep_id].dieptsiz = OTG_FS_PKTCNT_SET(pktcnt) | 
		OTG_FS_XFRSIZ_SET(xfrsiz); 
	/* enable end point, clear NACK */
	otg_fs->inep[ep_id].diepctl = depctl | OTG_FS_EPENA | OTG_FS_CNAK; 

	deptsiz = otg_fs->inep[ep_id].dieptsiz;
	(void)deptsiz;

	DCC_LOG2(LOG_INFO, "PKTCNT=%d XFRSIZ=%d", OTG_FS_PKTCNT_GET(deptsiz), 
			 OTG_FS_XFRSIZ_GET(deptsiz));

	return xfrsiz;
}

int stm32f_otg_fs_txf_push(struct stm32f_otg_fs * otg_fs, unsigned int ep_id,
						   void * buf)
{
	uint32_t depctl;
	uint32_t deptsiz;
	uint32_t mpsiz;
	uint32_t xfrsiz;
	uint32_t pktcnt;
	uint32_t free;
	uint32_t data;
	uint8_t * cp;
	int cnt;
	int i;

	free = otg_fs->inep[ep_id].dtxfsts * 4;
	depctl = otg_fs->inep[ep_id].diepctl;
	deptsiz = otg_fs->inep[ep_id].dieptsiz;

	if (ep_id == 0)
		mpsiz = OTGFS_EP0_MPSIZ_GET(depctl);
	else
		mpsiz = OTG_FS_MPSIZ_GET(depctl);

	xfrsiz = OTG_FS_XFRSIZ_GET(deptsiz);
	pktcnt = OTG_FS_PKTCNT_GET(deptsiz);
	(void)pktcnt;

	DCC_LOG5(LOG_INFO, "ep_id=%d mpsiz=%d pktcnt=%d xfrsiz=%d free=%d", 
			 ep_id, mpsiz, pktcnt, xfrsiz, free);

	if (xfrsiz < mpsiz) {
		if (free < xfrsiz) {
			DCC_LOG(LOG_PANIC, "free < xfrsiz !!!");
			return -1;
		}
		/* Transfer the last partial packet */
		cnt = xfrsiz;
	} else {
		if (free < mpsiz) {
			DCC_LOG(LOG_PANIC, "free < mpsiz !!!");
			return -1;
		}
		if (free < xfrsiz) {
			/* Transfer only full packets */
			cnt = (free / mpsiz) * mpsiz;
		} else {
			/* Transfer all */
			cnt = xfrsiz;
		}
		// XXX: debug
//		cnt = mpsiz;
	}

	/* push into fifo */
	cp = (uint8_t *)buf;
	for (i = 0; i < cnt; i += 4) {
		data = cp[0] + (cp[1] << 8) + (cp[2] << 16) + (cp[3] << 24);
		otg_fs->dfifo[ep_id].push = data;
		cp += 4;
	}	

	DCC_LOG1(LOG_INFO, "Tx: (%d)", cnt);
	return cnt;
}



void stm32f_otg_fs_device_init(struct stm32f_otg_fs * otg_fs)
{
	uint32_t depctl;
	int i;

	DCC_LOG(LOG_INFO, "1.");

	/* Disable global interrupts */
	otg_fs->gahbcfg &= ~OTG_FS_GINTMSK; 

	/* - Force device mode 
	   - Full Speed serial transceiver select */
	otg_fs->gusbcfg = OTG_FS_FDMOD | OTG_FS_TRDT_SET(5) | 
		OTG_FS_PHYSEL | OTG_FS_SRPCAP | OTG_FS_TOCAL_SET(1);

	/* Reset after a PHY select and set Device mode */
	stm32f_otg_fs_core_reset(otg_fs);

	DCC_LOG(LOG_INFO, "2.");

	/* Restart the Phy Clock */
	otg_fs->pcgcctl = 0;

	/* Flush the FIFOs */
	stm32f_otg_fs_txfifo_flush(otg_fs, 0x10);
	stm32f_otg_fs_rxfifo_flush(otg_fs);

	/* Clear all pending Device Interrupts */
	otg_fs->diepmsk = 0;
	otg_fs->doepmsk = 0;
	otg_fs->daint = 0xffffffff;
	otg_fs->daintmsk = 0;
	for (i = 0; i < 4; i++) {
		depctl = otg_fs->inep[i].diepctl;
		depctl = (depctl & OTG_FS_EPENA) ? OTG_FS_EPDIS | OTG_FS_SNAK : 0;
		otg_fs->inep[i].diepctl = depctl;
		otg_fs->inep[i].dieptsiz = 0;
		otg_fs->inep[i].diepint = 0xff;
	}
	for (i = 0; i < 4; i++) {
		depctl = otg_fs->outep[i].doepctl;
		depctl = (depctl & OTG_FS_EPENA) ? OTG_FS_EPDIS | OTG_FS_SNAK : 0;
		otg_fs->outep[i].doepctl = depctl;
		otg_fs->outep[i].doeptsiz = 0;
		otg_fs->outep[i].doepint = 0xff;
	}

	/* The application must perform the following steps to initialize the 
	   core as a device on power-up or after a mode change from host to device.
	1. Program the following fields in the OTG_FS_DCFG register:
	â€“ Device speed
	- Non-zero-length status OUT handshake */
	otg_fs->dcfg = OTG_FS_PFIVL_80 | OTG_FS_DSPD_FULL;

	/* 2. Program the OTG_FS_GINTMSK register to unmask the 
	   following interrupts:
	   â€“ Wakeup 
	   â€“ USB reset
	   â€“ Enumeration done
	   â€“ Early suspend
	   â€“ USB suspend
	   â€“ SOF */
	otg_fs->gintmsk = OTG_FS_SRQIM | OTG_FS_OTGINT;

//	otg_fs->gintmsk = OTG_FS_WUIM | OTG_FS_USBRSTM | OTG_FS_ENUMDNEM | 
//		OTG_FS_ESUSPM | OTG_FS_USBSUSPM;
//	otg_fs->gintmsk |=  OTG_FS_IEPINTM | OTG_FS_OEPINTM | 
//		OTG_FS_IISOIXFRM | OTG_FS_IISOOXFRM | OTG_FS_RXFLVLM | OTG_FS_SOFM;


	/* 3. Program the VBUSBSEN bit in the OTG_FS_GCCFG register to enable VBUS 
	   sensing in â€œBâ€� device mode and supply the 5 volts across the pull-up 
	   resistor on the DP line. */
	otg_fs->gccfg = OTG_FS_VBUSBSEN | OTG_FS_PWRDWN;

	/* 4. Wait for the USBRST interrupt in OTG_FS_GINTSTS. It indicates that 
	   a reset has been detected on the USB that lasts for about 10 ms on 
	   receiving this interrupt. Wait for the ENUMDNE interrupt in 
	   OTG_FS_GINTSTS.
	   This interrupt indicates the end of reset on the USB. On receiving this 
	   interrupt, the application must read the OTG_FS_DSTS register to 
	   determine the enumeration speed and perform the steps listed in 
	   Endpoint initialization on enumeration completion on page 1035.
	   At this point, the device is ready to accept SOF packets and perform 
	   control transfers on control endpoint 0. */

	/* AHB configuration */
	/* Enable global interrupts */
	otg_fs->gahbcfg |= OTG_FS_PTXFELVL | OTG_FS_TXFELVL | OTG_FS_GINTMSK; 

	DCC_LOG(LOG_INFO, "3.");
}

#ifdef DEBUG

static const char * const eptyp_nm[] = {
	"CTRL",
	"ISOC",
	"BULK",
	"INT"
};

void stm32f_otg_fs_ep_dump(struct stm32f_otg_fs * otg_fs, unsigned int addr) 
{
	int ep_id = addr & 0x7f;
	uint32_t depctl;
	uint32_t eptsiz;
	uint32_t eptfsav;
	uint32_t mpsiz;

	depctl = otg_fs->inep[ep_id].diepctl;
	if (depctl & OTG_FS_USBAEP) {
		eptfsav = otg_fs->inep[ep_id].dtxfsts;
		eptsiz = otg_fs->inep[ep_id].dieptsiz;

		mpsiz = (ep_id == 0) ? OTGFS_EP0_MPSIZ_GET(depctl) : OTG_FS_MPSIZ_GET(depctl);

		(void)eptsiz;
		eptfsav = eptfsav * 4;
		(void)mpsiz;

		DCC_LOG5(LOG_INFO, "EP%d IN %s TXFNUM=%d STALL=%d NAKSTS=%d",
				 ep_id, eptyp_nm[OTG_FS_EPTYP_GET(depctl)],
				 OTG_FS_TXFNUM_GET(depctl),
				 (depctl & OTG_FS_STALL) ? 1 : 0,
				 (depctl & OTG_FS_NAKSTS) ? 1 : 0);

		DCC_LOG4(LOG_INFO, "EONUM=%s DPID=%d USBAEP=%d MPSIZ=%d", 
				 (depctl & OTG_FS_EONUM) ? "EVEN" : "ODD",
				 (depctl & OTG_FS_DPID) ? 1 : 0,
				 (depctl & OTG_FS_USBAEP) ? 1 : 0,
				 mpsiz);


		DCC_LOG3(LOG_INFO, "PKTCNT=%d XFRSIZ=%d FSAVAIL=%d",
				 OTG_FS_PKTCNT_GET(eptsiz), OTG_FS_XFRSIZ_GET(eptsiz), 
				 eptfsav);
	} else {
		depctl = otg_fs->outep[ep_id].doepctl;
		if (depctl & OTG_FS_USBAEP) {
			eptsiz = otg_fs->outep[ep_id].doeptsiz;
			(void)eptsiz;
		} else {
			DCC_LOG1(LOG_WARNING, "EP%d not active!", ep_id);
			return;
		}
		
		mpsiz = (ep_id == 0) ? OTGFS_EP0_MPSIZ_GET(depctl) : OTG_FS_MPSIZ_GET(depctl);
		(void)mpsiz;

		DCC_LOG5(LOG_INFO, "EP%d OUT %s SNPM=%d STALL=%d NAKSTS=%d",
				 ep_id, eptyp_nm[OTG_FS_EPTYP_GET(depctl)],
				 (depctl & OTG_FS_SNPM) ? 1 : 0,
				 (depctl & OTG_FS_STALL) ? 1 : 0,
				 (depctl & OTG_FS_NAKSTS) ? 1 : 0);

		DCC_LOG4(LOG_INFO, "EONUM=%s DPID=%d USBAEP=%d MPSIZ=%d", 
				 (depctl & OTG_FS_EONUM) ? "EVEN" : "ODD",
				 (depctl & OTG_FS_DPID) ? 1 : 0,
				 (depctl & OTG_FS_USBAEP) ? 1 : 0,
				 mpsiz);

		DCC_LOG2(LOG_INFO, "PKTCNT=%d XFRSIZ=%d",
				 OTG_FS_PKTCNT_GET(eptsiz), OTG_FS_XFRSIZ_GET(eptsiz));
	}


}

void otg_fs_fifo(struct stm32f_otg_fs * otg_fs, 
				 unsigned int addr, unsigned int len)
{
	unsigned int q;
	unsigned int r;
	unsigned int n;
	unsigned int i;

	i = ((addr >> 2) / 4) * 4;
	n = (addr + len + 3) >> 2;
	q = (n / 4) * 4;
	r = n - q;

	for (; i < q; i += 4) {
		DCC_LOG5(LOG_INFO, "%04x: %08x %08x %08x %08x", i * 4, 
				 otg_fs->ram[i + 0], otg_fs->ram[i + 1],
				 otg_fs->ram[i + 2], otg_fs->ram[i + 3]);
	}

	switch (r) {
	case 1:
		DCC_LOG2(LOG_INFO, "%04x: %08x ", i * 4, 
				 otg_fs->ram[i + 0]);
		break;
	case 2:
		DCC_LOG3(LOG_INFO, "%04x: %08x %08x", i * 4, 
				 otg_fs->ram[i + 0], otg_fs->ram[i + 1]);
		break;
	case 3:
		DCC_LOG4(LOG_INFO, "%04x: %08x %08x %08x", i * 4, 
				 otg_fs->ram[i + 0], otg_fs->ram[i + 1],
				 otg_fs->ram[i + 2]);
		break;
	};
}

#endif

#endif /* STM32F_OTG_FS */
