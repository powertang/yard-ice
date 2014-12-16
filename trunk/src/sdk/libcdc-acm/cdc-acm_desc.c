/* 
 * File:	 cdc_acm.h
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


#include "cdc-acm.h"
#include <sys/param.h>
#include <string.h>

#ifndef CDC_ACM_ENABLE_PRODUCT_SET
#define CDC_ACM_ENABLE_PRODUCT_SET 0
#endif

#define ATMEL_VCOM_PRODUCT_ID 0x6119
#define ST_VCOM_PRODUCT_ID 0x5740

#ifndef CDC_ACM_PRODUCT_ID
#define CDC_ACM_PRODUCT_ID ST_VCOM_PRODUCT_ID 
#endif


const struct usb_descriptor_device cdc_acm_desc_dev = {
		/* Size of this descriptor in bytes */
		sizeof(struct usb_descriptor_device),
		/* DEVICE descriptor type */
		USB_DESCRIPTOR_DEVICE,
		/* USB specification release number */
		USB1_10,
		/* Class code */
		USB_CLASS_COMMUNICATION,
		/* Subclass code */
		0x00,
		/* Protocol code */
		0x00,
		/* Control endpoint 0 max. packet size */
		EP0_MAX_PKT_SIZE,
		/* Vendor ID */
		USB_VENDOR_ST,
		/* Product ID */
		CDC_ACM_PRODUCT_ID,
		/* Device release number */
		0x0002,
		/* Index of manufacturer string descriptor */
		0x01,
		/* Index of product string descriptor */
		0x02,
		/* Index of S.N.  string descriptor */
		0x03,
		/* Number of possible configurations */
		0x01
};

/* Configuration 1 descriptor */
const struct cdc_acm_descriptor_config cdc_acm_desc_cfg = {
		{
			/* Size of this descriptor in bytes */
			sizeof(struct usb_descriptor_configuration),
			/* CONFIGURATION descriptor type */
			USB_DESCRIPTOR_CONFIGURATION,
			/* Total length of data returned for this configuration */
			sizeof(struct cdc_acm_descriptor_config),
			/* Number of interfaces for this configuration */
			2,
			/* Value to use as an argument for the
			   Set Configuration request to select this configuration */
			1,
			/* Index of string descriptor describing this configuration */
			0,
			/* Configuration characteristics - attributes */
			USB_CONFIG_SELF_NOWAKEUP,
			/* Maximum power consumption of the device */
			USB_POWER_MA(100)
		},
		/* Communication Class Interface Descriptor Requirement */
		{
			/* Size of this descriptor in bytes */
			sizeof(struct usb_descriptor_interface),
			/* INTERFACE descriptor type */
			USB_DESCRIPTOR_INTERFACE,
			/* Number of this interface */
			0,
			/* Value used to select this alternate setting */
			0,
			/* Number of endpoints used by this interface
			   (excluding endpoint zero) */
			1,
			/* Class code */
			CDC_INTERFACE_COMMUNICATION,
			/* Sub-class */
			CDC_ABSTRACT_CONTROL_MODEL,
			/* Protocol code: (V.25ter, Common AT commands)*/
			CDC_PROTOCOL_COMMON_AT_COMMANDS,
			/* Index of string descriptor describing this interface */
			0x04
		},
		/* Header Functional Descriptor */
		{
			/* Size of this descriptor in bytes */
			sizeof(struct cdc_header_descriptor),
			/* CS_INTERFACE descriptor type */
			CDC_CS_INTERFACE,
			/* Header functional descriptor subtype */
			CDC_HEADER,
			/* USB CDC specification release version */
			CDC1_10
		},
		/* Call Management Functional Descriptor */
		{
			/* Size of this descriptor in bytes */
			sizeof(struct cdc_call_management_descriptor),
			/* CS_INTERFACE descriptor type */
			CDC_CS_INTERFACE,
			/* Call management functional descriptor subtype */
			CDC_CALL_MANAGEMENT,
			/* The capabilities that this configuration supports */
			1, /* D1 + D0 */
			/* Interface number of the data class
			   interface used for call management */
			1
		},
		/* Abstract Control Management Functional Descriptor */
		{
			/* Size of this descriptor in bytes */
			sizeof(struct cdc_abstract_control_management_descriptor),
			/* CS_INTERFACE descriptor type */
			CDC_CS_INTERFACE,
			/* Abstract control management functional descriptor subtype */
			CDC_ABSTRACT_CONTROL_MANAGEMENT,
			/* Capabilities supported by this configuration */
			0x06
		},
		/* Union Functional Descriptor */
		{
			{
				/* Size of this descriptor in bytes */
				sizeof(struct cdc_union_1slave_descriptor),
				/* CS_INTERFACE descriptor type */
				CDC_CS_INTERFACE,
				/* Union functional descriptor subtype */
				CDC_UNION,
				/* The interface number designated as master */
				0
			},
			/* The interface number designated as first slave */
			{ 1 }
		},
		/* Endpoint 3 descriptor */
		{
			/* Size of this descriptor in bytes */
			sizeof(struct usb_descriptor_endpoint),
			/* ENDPOINT descriptor type */
			USB_DESCRIPTOR_ENDPOINT,
			/* Address of the endpoint on the USB device */
			USB_ENDPOINT_IN + EP_INT_ADDR,
			/* Endpoint attributes when configured */
			ENDPOINT_TYPE_INTERRUPT,
			/* Maximum packet size this endpoint is capable
			   of sending or receiving */
			CDC_EP_INT_MAX_PKT_SIZE,
			/* Interval for polling endpoint (ms) */
			10
		},
		/* Data Class Interface Descriptor Requirement */
		{
			/* Size of this descriptor in bytes */
			sizeof(struct usb_descriptor_interface),
			/* INTERFACE descriptor type */
			USB_DESCRIPTOR_INTERFACE,
			/* Number of this interface */
			1,
			/* Value used to select this alternate setting */
			0,
			/* Number of endpoints used by this interface
			   (excluding endpoint zero) */
			2,
			/* Class code */
			CDC_INTERFACE_DATA,
			/* Sub-class */
			0,
			/* Protocol code */
			0,
			/* Index of string descriptor describing this interface */
			0
		},
		/* First alternate setting */
		/* Endpoint 1 descriptor */
		{
			/* Size of this descriptor in bytes */
			sizeof(struct usb_descriptor_endpoint),
			/* ENDPOINT descriptor type */
			USB_DESCRIPTOR_ENDPOINT,
			/* Address of the endpoint on the USB device */
			USB_ENDPOINT_OUT + EP_OUT_ADDR,
			/* Endpoint attributes when configured */
			ENDPOINT_TYPE_BULK,
			/* Maximum packet size this endpoint is capable of
			   sending or receiving */
			CDC_EP_OUT_MAX_PKT_SIZE,
			/* Interval for polling endpoint for data transfers */
			0x0
		},
		/* Endpoint 2 descriptor */
		{
			/* Size of this descriptor in bytes */
			sizeof(struct usb_descriptor_endpoint),
			/* ENDPOINT descriptor type */
			USB_DESCRIPTOR_ENDPOINT,
			/* Address of the endpoint on the USB device */
			USB_ENDPOINT_IN + EP_IN_ADDR,
			/* Endpoint attributes when configured */
			ENDPOINT_TYPE_BULK,
			/* Maximum packet size this endpoint is capable of
			   sending or receiving */
			CDC_EP_IN_MAX_PKT_SIZE,
			/* Interval for polling endpoint for data transfers */
			0x0
		}
};

#define LANG_STR_SZ              4
/* LangID = 0x0409: U.S. English */
const uint8_t cdc_acm_lang_str[LANG_STR_SZ] = {
	LANG_STR_SZ, USB_DESCRIPTOR_STRING,
	0x09, 0x04
};

#define VENDOR_STR_SZ            38
const uint8_t cdc_acm_vendor_str[VENDOR_STR_SZ] = {
	VENDOR_STR_SZ, USB_DESCRIPTOR_STRING,
	/* Manufacturer: "STMicroelectronics" */
	'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0, 'l', 0, 'e', 0,
	'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0, 'c', 0, 's', 0
};


#define PRODUCT_STR_SZ           44

#if (CDC_ACM_ENABLE_PRODUCT_SET)
#define PRODUCT_STR_SZ_MAX       50
uint8_t cdc_acm_product_str[PRODUCT_STR_SZ_MAX] = {
#else
const uint8_t cdc_acm_product_str[PRODUCT_STR_SZ] = {
#endif
	PRODUCT_STR_SZ, USB_DESCRIPTOR_STRING,
	/* Product name: "STM32 Virtual ComPort" */
	'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, ' ', 0, 'V', 0, 'i', 0, 'r', 0, 't', 0,
	'u', 0, 'a', 0, 'l', 0, ' ', 0, 'C', 0, 'o', 0, 'm', 0, 'P', 0, 'o', 0, 'r', 0,
	't', 0,
};


#define SERIAL_STR_SZ            26
uint8_t cdc_acm_serial_str[SERIAL_STR_SZ] = {
	SERIAL_STR_SZ, USB_DESCRIPTOR_STRING,
	/* Serial number: "0000000000001" */
	'0', 0, '0', 0, '0', 0, '0', 0, '0', 0, 
	'0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '1', 0
};

#define INTERFACE_STR_SZ         16
const uint8_t cdc_acm_interface_str[INTERFACE_STR_SZ] = {
	INTERFACE_STR_SZ, USB_DESCRIPTOR_STRING,
	/* Interface 0: "ST VCOM" */
	'S', 0, 'T', 0, ' ', 0, 'V', 0, 'C', 0, 'O', 0, 'M', 0
};

const uint8_t * const cdc_acm_str[] = {
	cdc_acm_lang_str,
	cdc_acm_vendor_str,
	cdc_acm_product_str,
	cdc_acm_serial_str,
	cdc_acm_interface_str
};

void usb_cdc_sn_set(uint64_t sn)
{
	char s[24];
	char * cp;
	int c;
	int i;
	int n;

	n = sprintf(s, "%llu", sn);
	cp = s + n - 1;

	for (i = (SERIAL_STR_SZ / 2) - 1; i >= 0; --i) {
		if (cp < s)
			break;
		c = *cp--;
		cdc_acm_serial_str[i * 2] = c;
	}
}

void usb_cdc_product_set(const char * s)
{
#if (CDC_ACM_ENABLE_PRODUCT_SET)
	char * cp;
	int len;
	int i;

	len = MIN(strlen(s), ((PRODUCT_STR_SZ_MAX - 1)/2));
	cdc_acm_product_str[0] = (len + 1) * 2;
	cp = (char *)s;
	for (i = 1; i <= len; ++i)
		cdc_acm_product_str[i * 2] = *cp++;
#endif
}



