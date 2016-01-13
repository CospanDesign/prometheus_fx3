#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
//#include "cypress_usb_defines.h"
#include "cyu3usb.h"
#include "cyu3uart.h"

#include "debug_controller.h"
#include "prometheus.h"

CyBool_t  DEBUG_ENABLED;

void uart_debug_setup (void);
CyBool_t is_debug_enabled(void);

void debug_setup (void){
  uint16_t size = 0;
  CyU3PEpConfig_t ep_config;
  CyU3PReturnStatus_t retvalue = CY_U3P_SUCCESS;
  CyU3PUSBSpeed_t usb_speed = CyU3PUsbGetSpeed();
  //uart_debug_setup();

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

  DEBUG_ENABLED = CyTrue;
}

CyBool_t is_debug_enabled(void){
  return DEBUG_ENABLED;
}

void debug_destroy(void){
  CyU3PEpConfig_t ep_config;
  CyU3PReturnStatus_t retvalue = CY_U3P_SUCCESS;

  CyU3PDebugDeInit ();
  CyU3PUsbFlushEp(CY_FX_EP_DEBUG_OUT);
  CyU3PMemSet((uint8_t *)&ep_config, 0, sizeof(ep_config));
  ep_config.enable = CyFalse;

  retvalue = CyU3PSetEpConfig(CY_FX_EP_DEBUG_OUT, &ep_config);
  if (retvalue != CY_U3P_SUCCESS) {
    CyFxAppErrorHandler(retvalue);
  }
}
