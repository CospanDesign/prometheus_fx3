#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
//#include "cypress_usb_defines.h"
#include "cyu3usb.h"
#include "cyu3uart.h"

#include "usb_controller.h"
#include "prometheus.h"

#define USER_WRITING(x) ((x & 0x80) == 0)
#define USER_READING(x) ((x & 0x80) != 0)

//Found in the main file
extern CyU3PEvent     main_event;                       /* Events that change the behavior of the system*/
extern CyBool_t       CONFIG_DONE;                      /* FPGA Configuration Finished */
extern CyU3PDmaChannel FPGA_CONFIG_CHANNEL;

extern CyU3PDmaChannel COMM_CHANNEL_USB_TO_GPIF;
extern CyU3PDmaChannel COMM_CHANNEL_GPIF_TO_USB;


        CyBool_t BASE_APP_ACTIVE = CyFalse;

uint8_t ep0_buffer[32] __attribute__ ((aligned (32)));  /* Buffer used for sending EP0 data.    */
uint8_t usb_configuration = 0;                          /* Active USB configuration.            */
uint8_t usb_interface = 0;                              /* Active USB interface.                */
uint8_t *select_buffer = 0;                             /* Buffer to hold SEL values.           */


/* This is the callback function to handle the USB events. */
void usb_event_cb (CyU3PUsbEventType_t evtype,  uint16_t evdata){
  switch (evtype){
    case CY_U3P_USB_EVENT_CONNECT:
      CyU3PEventSet(&main_event, EVT_USB_CONNECT, CYU3P_EVENT_OR);
      break;
    case CY_U3P_USB_EVENT_RESET:
    case CY_U3P_USB_EVENT_DISCONNECT:
      CyU3PEventSet(&main_event, EVT_USB_DISCONNECT, CYU3P_EVENT_OR);
      break;
    default:
      break;
  }
}

//Sets up USB
void usb_start(void){
  //Update the base app activate flag
  BASE_APP_ACTIVE = CyTrue;
}

//Stop A USB configuration
void usb_stop (void){
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;
  BASE_APP_ACTIVE = CyFalse;
  if (is_debug_enabled()) {
    debug_destroy();
  }
}

//USB Setup
CyBool_t usb_setup_cb (uint32_t setupdat0, uint32_t setupdat1){

  CyBool_t isHandled = CyTrue;
  CyBool_t gpio_value = CyTrue;
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

  uint8_t  bRequest, bReqType;
  uint8_t  bType, bTarget;
  uint16_t wValue, wIndex, wLength;

  /* Decode the fields from the setup request. */
  bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
  bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
  bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
  bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
  wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
  wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);
  wLength  = ((setupdat1 & CY_U3P_USB_LENGTH_MASK)  >> CY_U3P_USB_LENGTH_POS);

  //Standard Request
  if (bType == CY_U3P_USB_STANDARD_RQT){

    if ((bTarget == CY_U3P_USB_TARGET_INTF) &&
        ((bRequest == CY_U3P_USB_SC_SET_FEATURE) || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) &&
        (wValue == 0)){

      if (BASE_APP_ACTIVE){
        CyU3PUsbAckSetup ();
      }
      else{
        CyU3PUsbStall (0, CyTrue, CyFalse);
      }
      isHandled = CyTrue;
    }
    if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
        && (wValue == CY_U3P_USBX_FS_EP_HALT)){
      if (BASE_APP_ACTIVE){
        if (wIndex == CY_FX_EP_DEBUG_IN){
          //Flush/Reset Debug Endpoints
          CyU3PUsbFlushEp (CY_FX_EP_DEBUG_IN);
          CyU3PUsbResetEp (CY_FX_EP_DEBUG_IN);
        }

        if (wIndex == CY_FX_EP_DEBUG_OUT){
          //Flush/Reset Debug Endpoints
          CyU3PUsbFlushEp (CY_FX_EP_DEBUG_OUT);
          CyU3PUsbResetEp (CY_FX_EP_DEBUG_OUT);
        }

      }
      if (is_fpga_config_enabled()) {
        if (wIndex == CY_FX_EP_PRODUCER){
          CyU3PDmaChannelReset    (&FPGA_CONFIG_CHANNEL);
          CyU3PUsbFlushEp         (CY_FX_EP_PRODUCER);
          CyU3PUsbResetEp         (CY_FX_EP_PRODUCER);
          CyU3PDmaChannelSetXfer  (&FPGA_CONFIG_CHANNEL, CY_FX_COMM_DMA_TX_SIZE);
        }
      }
      if (is_comm_enabled()){
        if (wIndex == CY_FX_EP_PRODUCER){
          CyU3PDmaChannelReset    (&FPGA_CONFIG_CHANNEL);
          CyU3PUsbFlushEp         (CY_FX_EP_PRODUCER);
          CyU3PUsbResetEp         (CY_FX_EP_PRODUCER);
          CyU3PDmaChannelSetXfer  (&FPGA_CONFIG_CHANNEL, CY_FX_COMM_DMA_TX_SIZE);
        }
        if (wIndex == CY_FX_EP_CONSUMER){
          CyU3PDmaChannelReset    (&COMM_CHANNEL_USB_TO_GPIF);
          CyU3PUsbFlushEp         (CY_FX_EP_CONSUMER);
          CyU3PUsbResetEp         (CY_FX_EP_CONSUMER);
          CyU3PDmaChannelSetXfer  (&COMM_CHANNEL_GPIF_TO_USB, CY_FX_COMM_DMA_RX_SIZE);
        }
      }
      CyU3PUsbStall (wIndex, CyFalse, CyTrue);
      isHandled = CyTrue;
    }
  }

  //Vendor Specific Requests
  if (bType == CY_U3P_USB_VENDOR_RQT){
    switch (bRequest) {
      case (RESET_TO_BOOTMODE):
        //Reset to boot mode
        CyU3PEventSet(&main_event, RESET_PROC_BOOT_EVENT, CYU3P_EVENT_OR);
        CyU3PUsbAckSetup();
        isHandled = CyTrue;
        break;

      case (GPIO_CONTROL):
        if (USER_WRITING(bReqType)) {
          CyU3PUsbGetEP0Data (wLength, ep0_buffer, NULL);
          CyU3PGpioSetValue (FPGA_SOFT_RESET,     !((ep0_buffer[0] & GPIO_FPGA_SOFT_RESET) > 0));
          CyU3PGpioSetValue (FMC_POWER_GOOD_OUT,  ((ep0_buffer[0] & GPIO_FMC_POWER_GOOD_OUT) > 0));
          CyU3PUsbAckSetup();
        }
        else {
          ep0_buffer[0] = 0;
          if (!CyU3PGpioGetValue(FPGA_SOFT_RESET))    ep0_buffer[0] |= GPIO_FPGA_SOFT_RESET;
          if (CyU3PGpioGetValue(DONE))                ep0_buffer[0] |= GPIO_FPGA_DONE;
          if (CyU3PGpioGetValue(FPGA_INTERRUPT))      ep0_buffer[0] |= GPIO_FPGA_INTERRUPT;
          if (!CyU3PGpioGetValue(FMC_DETECT_N))       ep0_buffer[0] |= GPIO_FMC_DETECT;
          /*** THERE IS A PROBLEM WITH THIS PIN!***/
          //if (CyU3PGpioGetValue(FMC_POWER_GOOD_OUT))  ep0_buffer[0] |= GPIO_FMC_POWER_GOOD_OUT;
          if (CyU3PGpioGetValue(FMC_POWER_GOOD_IN))   ep0_buffer[0] |= GPIO_FMC_POWER_GOOD_IN;
          CyU3PUsbSendEP0Data (wLength, ep0_buffer);
        }
        isHandled = CyTrue;
        break;
      case (INTERNAL_CONFIG):
        //This is used to read and write values within the MCU, use this to set GPIOs and other features

        CyU3PDebugPrint(2, "usb_controller: wLength: %d, Req: %X", wLength, bReqType);
        //Internal Controls
        if (USER_WRITING(bReqType)) {
          CyU3PUsbGetEP0Data (wLength, ep0_buffer, NULL);
          CyU3PUsbAckSetup();
        }
        else {
          ep0_buffer[0] = 0;
          CyU3PUsbSendEP0Data (wLength, ep0_buffer);
        }
        isHandled = CyTrue;
        break;
      case (START_DEBUG):
        if (USER_WRITING(bReqType)){
          ep0_buffer[0] = 0;
          CyU3PUsbGetEP0Data (wLength, ep0_buffer, NULL);
          if (wLength == 0)
            ep0_buffer[0] = 0x01;
          if (ep0_buffer[0]) {
            if (!is_debug_enabled()) {
              debug_setup();
              CyU3PDebugPrint (2, "Debugger Started");
            }
            else
              CyU3PDebugPrint (2, "Debugger Already Running");
          }
          else {
            if (is_debug_enabled())
              debug_destroy();
          }
          CyU3PUsbAckSetup();
        }
        else {
          //Do not handle reading at this time
          ep0_buffer[0] = is_debug_enabled() ? 0x01: 0x00;
          CyU3PUsbSendEP0Data (wLength, ep0_buffer);
        }
        isHandled = CyTrue;
        break;

      case (ENTER_FPGA_CONFIG_MODE):
        return_to_base();
        if (USER_WRITING(bReqType)) {
          uint32_t value;
          //fpga_confiure_mcu();
          //What is this flag for?
          //Extract the size from byte array
          CyU3PUsbGetEP0Data (wLength, ep0_buffer, NULL);
          value = (uint32_t)  ((ep0_buffer[3] << 24) |
                               (ep0_buffer[2] << 16) |
                               (ep0_buffer[1] << 8)  |
                                ep0_buffer[0]);
          fpga_config_set_file_length(value);
          //CyU3PDebugPrint (2, "usb_controller: File Length: %X, %d", file_length, file_length);
          CONFIG_DONE = CyTrue;
          //Set CONFIG FPGA APP Start Event to start configurin the FPGA
          CyU3PEventSet (&main_event, ENTER_FPGA_CONFIG_MODE_EVENT, CYU3P_EVENT_OR);
        }
        CyU3PUsbAckSetup();
        isHandled = CyTrue;
        break;

      case (ENTER_FPGA_COMM_MODE):
        if (USER_WRITING(bReqType)){
          CyU3PDebugPrint (2, "usb_controller: Enable COMM Mode");
          retval = CyU3PGpioSetValue (FPGA_SOFT_RESET, CyTrue);
          CyU3PUsbGetEP0Data (wLength, ep0_buffer, NULL);
          CyU3PEventSet(&main_event, ENTER_FPGA_COMM_MODE_EVENT, CYU3P_EVENT_OR);
          CyU3PUsbAckSetup();
        }
        else{
          ep0_buffer[0] = is_comm_enabled() ? 0x01 : 0x00;
          CyU3PUsbSendEP0Data (wLength, ep0_buffer);
        }
        isHandled = CyTrue;
        break;
      default:
        CyU3PDebugPrint (2, "usb_controller: Unrecognized command: %X", bRequest);
        break;
    }
  }
  return isHandled;
}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
   whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, the
   FX3 device is retained in the low power state. If we return CyFalse, the FX3 device immediately tries
   to trigger an exit back to U0.

   This application does not have any state in which we should not allow U1/U2 transitions; and therefore
   the function always return CyTrue.
 */
CyBool_t lpm_request_cb (CyU3PUsbLinkPowerMode link_mode){
    return CyTrue;
}

/* This function initializes the USB Module, sets the enumeration descriptors.
 * This function does not start the bulk streaming and this is done only when
 * SET_CONF event is received. */
void usb_setup_mcu (void) {
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

  /* Allocate a buffer to hold the SEL (System Exit Latency) values set by the host. */
  select_buffer = (uint8_t *)CyU3PDmaBufferAlloc (32);
  CyU3PMemSet (select_buffer, 0, 32);

  /* This part needs to be commented out in your application as this enumeration part is
    taken care in the CyFxConfigFpgaApplnInit function */
  /* Start the USB functionality. */
  retval = CyU3PUsbStart();
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "CyU3PUsbStart failed to Start: Error code: %d\n", retval);
      CyFxAppErrorHandler(retval);
  }

  /* The fast enumeration is the easiest way to setup a USB connection,
   * where all enumeration phase is handled by the library. Only the
   * class / vendor requests need to be handled by the application. */
  CyU3PUsbRegisterSetupCallback(usb_setup_cb, CyTrue);

  /* Setup the callback to handle the USB events. */
  CyU3PUsbRegisterEventCallback(usb_event_cb);

  /* Register a callback to handle LPM requests from the USB 3.0 host. */
  CyU3PUsbRegisterLPMRequestCallback(lpm_request_cb);

  /* Set the USB Enumeration descriptors */

  /* Super speed device descriptor. */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB30DeviceDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB set device descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* High speed device descriptor. */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB20DeviceDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB set device descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* BOS descriptor */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB set configuration descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* Device qualifier descriptor */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB set device qualifier descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* Super speed configuration descriptor */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB set configuration descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* High speed configuration descriptor */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB Set Other Speed Descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* Full speed configuration descriptor */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* String descriptor 0 */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB set string descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* String descriptor 1 */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB set string descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* String descriptor 2 */
  retval = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB set string descriptor failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }

  /* Connect the USB Pins with super speed operation enabled. */
  retval = CyU3PConnectState(CyTrue, CyTrue);
  if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint (4, "USB Connect failed: Error code: %d", retval);
      CyFxAppErrorHandler(retval);
  }
}

