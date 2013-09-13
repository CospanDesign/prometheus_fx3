//Pins
#ifndef __PROMETHEUS_DEFINES__
#define __PROMETHEUS_DEFINES__

#include "cyu3types.h"
#include "cyu3usbconst.h"

#include "cyu3externcstart.h"

//Pin Name:GPIO
#define ADJ_REG_EN              30
#define UART_EN                 24
#define OTG_5V_EN               32

#define POWER_SELECT_0          27
#define POWER_SELECT_1          45

#define FMC_POWER_GOOD_OUT      25
#define FMC_POWER_GOOD_IN       26
#define FMC_DETECT_N            57

#define FPGA_SOFT_RESET         51
#define INIT_N                  52
#define DONE                    50

//Pin Direction
#define IN                      0
#define OUT                     1

#define ADJ_REG_EN_DIR          IN
#define UART_EN_DIR             OUT
//XXX: THIS OTG 5V EN PIN IS AN INPUT ONLY BECAUSE THERE ISN"T THE CORRECT CONTROL TO CONTROL THIS CORRECTLY
#define OTG_5V_EN_DIR           IN

#define POWER_SELECT_0_DIR      OUT
#define POWER_SELECT_1_DIR      OUT

#define FMC_POWER_GOOD_OUT_DIR  OUT
#define FMC_POWER_GOOD_IN_DIR   IN
#define FMC_DETECT_N_DIR        IN

#define FPGA_SOFT_RESET_DIR     OUT
#define INIT_N_DIR              IN
#define DONE_DIR                IN

//Define the GPIO Dir Bitmap
#define LOW_GPIO_DIR  (uint32_t) (ADJ_REG_EN_DIR          <<  ADJ_REG_EN                | \
                                  UART_EN_DIR             <<  UART_EN                   | \
                                  POWER_SELECT_0_DIR      <<  POWER_SELECT_0            | \
                                  FMC_POWER_GOOD_OUT_DIR  <<  FMC_POWER_GOOD_OUT        | \
                                  FMC_POWER_GOOD_IN_DIR   <<  FMC_POWER_GOOD_IN)

#define HIGH_GPIO_DIR (uint32_t) (OTG_5V_EN_DIR           <<  (OTG_5V_EN - 32)          | \
                                  POWER_SELECT_1_DIR      <<  (POWER_SELECT_1 - 32)     | \
                                  FMC_DETECT_N_DIR        <<  (FMC_DETECT_N - 32)       | \
                                  FPGA_SOFT_RESET_DIR     <<  (FPGA_SOFT_RESET - 32)    | \
                                  INIT_N_DIR              <<  (INIT_N - 32)             | \
                                  DONE_DIR                <<  (DONE - 32))



//USB Defines (User interfaces with the MCU through the control channel through the USB, these specify functions)
#define RESET_TO_BOOTMODE                    0xE0
#define START_DEBUG                          0xB0
#define ENTER_FPGA_COMM_MODE                 0xB1
#define ENTER_FPGA_CONFIG_MODE               0xB2
#define INTERNAL_CONFIG                      0xB3

#define USB_SET_REG_EN_TO_OUT                0xB4
#define USB_DISABLE_REGULATOR                0xB5
#define USB_ENABLE_REGULATOR                 0xB6


//Events that are raised when user calls the message
#define RESET_PROC_BOOT_EVENT                (1 << 0)
#define ENTER_FPGA_CONFIG_MODE_EVENT         (1 << 1)
#define ENTER_FPGA_COMM_MODE_EVENT           (1 << 2)
#define EVT_SET_REG_EN_TO_OUTPUT             (1 << 3)
#define EVT_DISABLE_REGULATOR                (1 << 4)
#define EVT_ENABLE_REGULATOR                 (1 << 5)


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

#define PROMETHEUS_PRODUCER_USB_SOCKET        CY_U3P_UIB_SOCKET_PROD_2 /* Socket 2 is producer */
#define PROMETHEUS_CONSUMER_USB_SOCKET        CY_U3P_UIB_SOCKET_CONS_2 /* Socket 2 is consumer */

#define CY_FX_EP_COMM_DMA_BUF_COUNT_U_2_P     (2)
#define CY_FX_EP_COMM_DMA_BUF_COUNT_P_2_U     (2)

#define PROMETHEUS_PRODUCER_PPORT_0           CY_U3P_PIB_SOCKET_0      /* P-port Socket 0 producer */
#define PROMETHEUS_PRODUCER_PPORT_1           CY_U3P_PIB_SOCKET_1      /* P-port Socket 1 producer */

#define PROMETHEUS_CONSUMER_PPORT_0           CY_U3P_PIB_SOCKET_2      /* P-port socket 2 consumer */
#define PROMETHEUS_CONSUMER_PPORT_1           CY_U3P_PIB_SOCKET_3      /* P-port socket 3 consumer */

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


#endif  //__PROMETHEUS_DEFINES__
