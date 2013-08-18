#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cypress_usb_defines.h"
#include "cyu3usb.h"
#include "cyu3uart.h"

#include "debug_controller.h"


void uart_debug_init (void);

void debug_init (void){
  uint16_t size = 0;
  CyU3PEpConfig_t ep_config;
  CyU3PReturnStatus_t retvalue = CY_U3P_SUCCESS;
  CyU3PUSBSpeed_t usb_speed = CyU3PUsbGetSpeed();
  //uart_debug_init();

  //Get the USB Speed
  switch (usb_speed) {
    case CY_U3P_FULL_SPEED:
      size = 64;
      break;

    case CY_U3P_HIGH_SPEED:
    case  CY_U3P_SUPER_SPEED:
      size = 128;
      break;

    default:
      CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
      break;
  }

  CyU3PMemSet((uint8_t *) &ep_config, 0, sizeof(ep_config));
  ep_config.enable = CyTrue;
  ep_config.epType = CY_U3P_USB_EP_INTR;
  ep_config.burstLen = 1;
  ep_config.streams = 0;
  ep_config.pcktSize = size;

  //Consumer Enpoint Configuration
  retvalue = CyU3PSetEpConfig(CY_FX_EP_DEBUG_OUT, &ep_config);
  if (retvalue != CY_U3P_SUCCESS) {
    CyFxAppErrorHandler(retvalue);
  }

  //Flush the endpoint memory
  CyU3PUsbFlushEp(CY_FX_EP_DEBUG_OUT);

  retvalue = CyU3PDebugInit(CY_FX_EP_DEBUG_OUT_SOCKET, 8);
  if (retvalue != CY_U3P_SUCCESS) {
    CyFxAppErrorHandler(retvalue);
  }

}

/* This function initializes the debug module. The debug prints
 * are routed to the UART and can be seen using a UART console
 * running at 115200 baud rate. */

void uart_debug_init (void){

    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t retvalue = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    retvalue = CyU3PUartInit();
    if (retvalue != CY_U3P_SUCCESS){
        /* Error handling */
        CyFxAppErrorHandler(retvalue);
    }

    /* Set UART configuration */
    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma    = CyTrue;

    retvalue = CyU3PUartSetConfig (&uartConfig, NULL);
    if (retvalue != CY_U3P_SUCCESS){
        CyFxAppErrorHandler(retvalue);
    }

    /* Set the UART transfer to a really large value. */
    retvalue = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (retvalue != CY_U3P_SUCCESS) {
        CyFxAppErrorHandler(retvalue);
    }

    /* Initialize the debug module. */
    retvalue = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (retvalue != CY_U3P_SUCCESS) {
        CyFxAppErrorHandler(retvalue);
    }
}
