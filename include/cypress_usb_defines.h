/*
 ## Cypress USB 3.0 Platform header file (cyfxbulklpautoenum.h)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2011,
 ##  All Rights Reserved
 ##  UNPUBLISHED, LICENSED SOFTWARE.
 ##
 ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
 ##  WHICH IS THE PROPERTY OF CYPRESS.
 ##
 ##  Use of this file is governed
 ##  by the license agreement included in the file
 ##
 ##     <install>/license/license.txt
 ##
 ##  where <install> is the Cypress software
 ##  installation root directory path.
 ##
 ## ===========================
*/

/* This file contains the externants used by the bulk loop application example */

#ifndef _INCLUDED_CYFXCOMMAUTO_ENUM_H_
#define _INCLUDED_CYFXCOMMAUTO_ENUM_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define CY_FX_COMM_DMA_BUF_COUNT             (8)        /* Bulk loop channel buffer count */
#define CY_FX_COMM_DMA_TX_SIZE               (0)        /* DMA transfer size is set to infinite */
#define CY_FX_COMM_DMA_RX_SIZE               (0)        /* DMA transfer size is set to infinite */
#define CY_FX_COMM_THREAD_STACK              (0x1000)   /* Bulk loop application thread stack size */
#define CY_FX_COMM_THREAD_PRIORITY           (8)        /* Bulk loop application thread priority */

#define CY_FX_GPIOAPP_THREAD_STACK           (0x0400)   /* GPIO application thread stack size */
#define CY_FX_GPIOAPP_THREAD_PRIORITY        (8)        /* GPIO application thread priority */


#define CY_FX_EP_DEBUG_IN                     0x01                     /* EP 1 OUT (Relative to host)*/
#define CY_FX_EP_DEBUG_OUT                    0x81                     /* EP 1 IN  (Relative to host)*/

#define CY_FX_EP_DEBUG_IN_SOCKET              CY_U3P_UIB_SOCKET_PROD_1
#define CY_FX_EP_DEBUG_OUT_SOCKET             CY_U3P_UIB_SOCKET_CONS_1

#define CY_FX_EP_PRODUCER                     0x02                     /* EP 2 OUT (Relative to host)*/
#define CY_FX_EP_CONSUMER                     0x82                     /* EP 2 IN  (Relative to host)*/

#define CY_FX_EP_PRODUCER_USB_SOCKET          CY_U3P_UIB_SOCKET_PROD_2 /* Socket 2 is producer */
#define CY_FX_EP_CONSUMER_USB_SOCKET          CY_U3P_UIB_SOCKET_CONS_2 /* Socket 2 is consumer */

#define CY_FX_EP_COMM_DMA_BUF_COUNT_U_2_P     (2)
#define CY_FX_EP_COMM_DMA_BUF_COUNT_P_2_U     (2)

#define CY_FX_EP_PRODUCER_PPORT_SOCKET        CY_U3P_PIB_SOCKET_0      /* P-port Socket 0 producer */
#define CY_FX_EP_CONSUMER_PPORT_SOCKET        CY_U3P_PIB_SOCKET_3      /* P-port socket 3 consumer */

#define DMA_BUF_SIZE						              (16)

#define BURST_LEN 16                                                   /*For USB 3.0 */

/* Extern definitions for the USB Descriptors */
extern const uint8_t CyFxUSB20DeviceDscr[];
extern const uint8_t CyFxUSB30DeviceDscr[];
extern const uint8_t CyFxUSBDeviceQualDscr[];
extern const uint8_t CyFxUSBFSConfigDscr[];
extern const uint8_t CyFxUSBHSConfigDscr[];
extern const uint8_t CyFxUSBBOSDscr[];
extern const uint8_t CyFxUSBSSConfigDscr[];
extern const uint8_t CyFxUSBStringLangIDDscr[];
extern const uint8_t CyFxUSBManufactureDscr[];
extern const uint8_t CyFxUSBProductDscr[];

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYFXCOMMAUTO_ENUM_H_ */

/*[]*/
