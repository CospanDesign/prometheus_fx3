#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cypress_usb_defines.h"
#include "cyu3usb.h"
#include "cyu3uart.h"

#include "usb_controller.h"
#include "prometheus.h"

//Found in the main file
extern CyU3PEvent     main_event;                       /* Events that change the behavior of the system*/
extern CyBool_t       CONFIG_DONE;                      /* FPGA Configuration Finished */
extern uint32_t       file_length;

        CyBool_t BASE_APP_ACTIVE = CyFalse;
extern  CyBool_t FPGA_CONFIG_APP_ACTIVE;
extern  CyBool_t COMM_APP_ACTIVE;


CyU3PDmaChannel bulk_dma_channel_handle;                /* DMA Channel handle */
uint8_t ep0_buffer[32] __attribute__ ((aligned (32)));  /* Buffer used for sending EP0 data.    */
uint8_t device_status = 0;                              /* USB device status. Bus powered.      */
uint8_t usb_configuration = 0;                          /* Active USB configuration.            */
uint8_t usb_interface = 0;                              /* Active USB interface.                */
uint8_t *select_buffer = 0;                             /* Buffer to hold SEL values.           */



/* This is the callback function to handle the USB events. */
void usb_event_cb (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    ){
  switch (evtype){
    case CY_U3P_USB_EVENT_RESET:
    case CY_U3P_USB_EVENT_DISCONNECT:
      /* Stop the loop back function. */
      usb_stop ();
      /* Drop current U1/U2 enable state values. */
      device_status = 0;
      break;
    default:
      break;
  }
}


/*
 *  This Function starts the bulk application
 *
 *  When a SET_CONF event is received from the USB host the endpoints are
 *  configured  and the DMA pipe is setup in this function
 */

void usb_start(void){
  uint16_t size = 0;
  //Debug Size is for the Debug interface
  uint16_t debug_size = 0;
  CyU3PEpConfig_t epCfg;
  CyU3PDmaChannelConfig_t dmaCfg;
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;
  CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

  /* First identify the usb speed. Once that is identified,
   * create a DMA channel and start the transfer on this. */

  /* Based on the Bus Speed configure the endpoint packet size */
  debug_init();
  /*
  switch (usbSpeed){
    case CY_U3P_FULL_SPEED:
      size = 64;
      debug_size = 64;
      break;

    case CY_U3P_HIGH_SPEED:
      size = 512;
      debug_size = 128;
      break;

    case  CY_U3P_SUPER_SPEED:
      size = 1024;
      debug_size = 128;
      break;

    default:
      CyU3PDebugPrint (4, "Error! Invalid USB speed.\n");
      CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
      break;
  }

  //Setup The Debug Interrupt Endpoitns
  CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
  epCfg.enable = CyTrue;
  epCfg.epType = CY_U3P_USB_EP_INTR;
  epCfg.burstLen = 1;
  epCfg.streams = 0;
  epCfg.pcktSize = size;

  //Consumer Endpoint
  retval = CyU3PSetEpConfig(CY_FX_EP_DEBUG_IN, &epCfg);
  if (retval != CY_U3P_SUCCESS) {
    CyFxAppErrorHandler (retval);
  }

  //Producer Endpoint
  retval = CyU3PSetEpConfig(CY_FX_EP_DEBUG_OUT, &epCfg);
  if (retval != CY_U3P_SUCCESS) {
    CyFxAppErrorHandler (retval);
  }

  //Flush the Endpoint memory
  CyU3PUsbFlushEp(CY_FX_EP_DEBUG_IN);
  CyU3PUsbFlushEp(CY_FX_EP_DEBUG_OUT);

  //Now Setup the Debug
  retval = CyU3PDebugInit(CY_FX_EP_DEBUG_OUT_SOCKET, 8);
  if (retval != CY_U3P_SUCCESS) {
    CyFxAppErrorHandler (retval);
  }

  //Setup The Bulk
  CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
  epCfg.enable = CyTrue;
  epCfg.epType = CY_U3P_USB_EP_BULK;
  epCfg.burstLen = 1;
  epCfg.streams = 0;
  epCfg.pcktSize = size;

  //Producer Endpoint Configuration
  retval = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
  if (retval != CY_U3P_SUCCESS) {
      CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", retval);
      CyFxAppErrorHandler (retval);
  }
  //Consumer Endpoint Configuration
  retval = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
  if (retval != CY_U3P_SUCCESS) {
      CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", retval);
      CyFxAppErrorHandler (retval);
  }

  //Create a DMA Auto Channel between two sockets of the U-port.
  //DMA size is set based on the USB speed
  dmaCfg.size = size;
  dmaCfg.count = CY_FX_COMM_DMA_BUF_COUNT;
  dmaCfg.prodSckId = CY_FX_EP_PRODUCER_PPORT_SOCKET;
  dmaCfg.consSckId = CY_FX_EP_CONSUMER_PPORT_SOCKET;
  dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
  dmaCfg.notification = 0;
  dmaCfg.cb = NULL;

  dmaCfg.prodHeader = 0;
  dmaCfg.prodFooter = 0;
  dmaCfg.consHeader = 0;
  dmaCfg.prodAvailCount = 0;

  retval = CyU3PDmaChannelCreate (&bulk_dma_channel_handle,
          CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
  if (retval != CY_U3P_SUCCESS) {
      CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", retval);
      CyFxAppErrorHandler(retval);
  }

  // Flush the Endpoint memory
  CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
  CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

  //Set DMA Channel Transfer Size
  retval = CyU3PDmaChannelSetXfer (&bulk_dma_channel_handle, CY_FX_COMM_DMA_TX_SIZE);
  if (retval != CY_U3P_SUCCESS) {
      CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n", retval);
      CyFxAppErrorHandler(retval);
  }
*/
  //Update the base app activate flag
  BASE_APP_ACTIVE = CyTrue;

  //Drop current U1/U2 enable state values
  device_status = 0;

}

/* This function stops the bulk loop application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipe is destroyed by this function. */
void usb_stop (void){
  CyU3PEpConfig_t epCfg;
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

  //Update the flag
  if (FPGA_CONFIG_APP_ACTIVE){
    //Disable the FPGA
    fpga_config_stop();
  }
  if (COMM_APP_ACTIVE){
    //Disable the FPGA comm application
    comm_config_stop();
  }
  BASE_APP_ACTIVE = CyFalse;

  CyU3PDebugDeInit ();
  /*
  //Flush the endpoint memory
  CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
  CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);
  CyU3PUsbFlushEp(CY_FX_EP_DEBUG_IN);
  CyU3PUsbFlushEp(CY_FX_EP_DEBUG_OUT);


  //Destroy the channel
  CyU3PDmaChannelDestroy (&bulk_dma_channel_handle);

  //Disable endpoints
  CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
  epCfg.enable = CyFalse;


  //Debug Input
  retval = CyU3PSetEpConfig(CY_FX_EP_DEBUG_IN, &epCfg);
  if (retval != CY_U3P_SUCCESS) {
      CyFxAppErrorHandler (retval);
  }

  //Debug Output
  retval = CyU3PSetEpConfig(CY_FX_EP_DEBUG_OUT, &epCfg);
  if (retval != CY_U3P_SUCCESS) {
      CyFxAppErrorHandler (retval);
  }

  //Producer Endpoint Configuration
  retval = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
  if (retval != CY_U3P_SUCCESS){
      CyFxAppErrorHandler (retval);
  }

  //Consumer endpoint configuration
  retval = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
  if (retval != CY_U3P_SUCCESS){
      CyFxAppErrorHandler (retval);
  }
  */
}


/* Set a USB Feature
 */
CyBool_t usb_set_feature (uint8_t bTarget, uint16_t wValue, uint16_t wIndex){

  CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();
  CyBool_t isHandled = CyFalse;

  if (bTarget == CY_U3P_USB_TARGET_DEVICE){
    switch (wValue & 0xFF){
      case CY_U3P_USB2_FS_REMOTE_WAKE:
        device_status |= 0x02;
        /* Fall-through here. */
      case CY_U3P_USB2_FS_TEST_MODE:
        if (usbSpeed != CY_U3P_SUPER_SPEED){
          isHandled = CyTrue;
        }
        break;

      case CY_U3P_USB3_FS_U1_ENABLE:
        if ((usbSpeed == CY_U3P_SUPER_SPEED) && (BASE_APP_ACTIVE)){
          device_status |= 0x04;
          isHandled = CyTrue;
        }
        break;

      case CY_U3P_USB3_FS_U2_ENABLE:
        if ((usbSpeed == CY_U3P_SUPER_SPEED) && (BASE_APP_ACTIVE)){
          device_status |= 0x08;
          isHandled = CyTrue;
        }
        break;
      default:
        isHandled = CyFalse;
        break;
    }
  }
  else if (bTarget == CY_U3P_USB_TARGET_INTF){
    /* Need to handle SET_FEATURE(FUNCTION_SUSPEND) requests. We allow this request to pass
       if the USB device is in configured state and fail it otherwise.  */
    if ((BASE_APP_ACTIVE) && (wValue == 0))
      CyU3PUsbAckSetup ();
    else
      CyU3PUsbStall (0, CyTrue, CyFalse);

    isHandled = CyTrue;
  }
  else if ((bTarget == CY_U3P_USB_TARGET_ENDPT) &&
          (wValue == CY_U3P_USBX_FS_EP_HALT)) {
    /* Stall the endpoint */
    CyU3PUsbStall (wIndex, CyTrue, CyFalse);
    isHandled = CyTrue;
  }
  else
  {
    isHandled = CyFalse;
  }

  return isHandled;
}

/* Handle the CLEAR_FEATURE request. */
CyBool_t usb_clear_feature (
        uint8_t bTarget,
        uint16_t wValue,
        uint16_t wIndex){
  CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();
  CyBool_t isHandled = CyFalse;

  if (bTarget == CY_U3P_USB_TARGET_DEVICE){
    switch (wValue & 0xFF){
      case CY_U3P_USB2_FS_REMOTE_WAKE:
        device_status &= 0xFD;
        /* Fall-through here. */
      case CY_U3P_USB2_FS_TEST_MODE:
        if (usbSpeed != CY_U3P_SUPER_SPEED){
          isHandled = CyTrue;
        }
        break;

      case CY_U3P_USB3_FS_U1_ENABLE:
        if ((usbSpeed == CY_U3P_SUPER_SPEED) && (BASE_APP_ACTIVE)){
          device_status &= 0xFB;
          isHandled = CyTrue;
        }
        break;

      case CY_U3P_USB3_FS_U2_ENABLE:
        if ((usbSpeed == CY_U3P_SUPER_SPEED) && (BASE_APP_ACTIVE)){
          device_status &= 0xF7;
          isHandled = CyTrue;
        }
        break;

      default:
        isHandled = CyFalse;
        break;
    }
  }
  else if (bTarget == CY_U3P_USB_TARGET_INTF){
    /* Need to handle CLEAR_FEATURE(FUNCTION_SUSPEND) requests. We allow this request to pass
       if the USB device is in configured state and fail it otherwise.  */
    if ((BASE_APP_ACTIVE) && (wValue == 0))
      CyU3PUsbAckSetup ();
    else
      CyU3PUsbStall (0, CyTrue, CyFalse);

    isHandled = CyTrue;
  }
  else if ((bTarget == CY_U3P_USB_TARGET_ENDPT) &&
        (wValue == CY_U3P_USBX_FS_EP_HALT)){
    /* If the application is active make sure that all sockets and
     * DMA buffers are flushed and cleaned. If more than one enpoint
     * is linked to the same DMA channel, then reset all the affected
     * endpoint pipes. */
    if (BASE_APP_ACTIVE){
      //Flush/Reset Debug Endpoints
      CyU3PUsbFlushEp (CY_FX_EP_DEBUG_IN);
      CyU3PUsbFlushEp (CY_FX_EP_DEBUG_OUT);
      CyU3PUsbResetEp (CY_FX_EP_DEBUG_IN);
      CyU3PUsbResetEp (CY_FX_EP_DEBUG_OUT);


      CyU3PDmaChannelReset (&bulk_dma_channel_handle);
      CyU3PUsbFlushEp (CY_FX_EP_PRODUCER);
      CyU3PUsbFlushEp (CY_FX_EP_CONSUMER);


      CyU3PUsbResetEp (CY_FX_EP_PRODUCER);
      CyU3PUsbResetEp (CY_FX_EP_CONSUMER);
      CyU3PDmaChannelSetXfer (&bulk_dma_channel_handle, CY_FX_COMM_DMA_TX_SIZE);
    }

    /* Clear stall on the endpoint. */
    CyU3PUsbStall (wIndex, CyFalse, CyTrue);
    isHandled = CyTrue;
  }
  else{
    isHandled = CyFalse;
  }

  return isHandled;
}

/* Send the requested USB descriptor to the host. */
CyU3PReturnStatus_t usb_send_descriptor (uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
    uint16_t length = 0, index = 0;
    uint8_t *buffer = NULL;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed ();

    /* The descriptor type is in the MS byte and the index is in
     * the LS byte. The index is useful only for string descriptor. */
    switch (wValue >> 8)
    {
        case CY_U3P_USB_DEVICE_DESCR:
            if (usbSpeed == CY_U3P_SUPER_SPEED){
                buffer = (uint8_t *)CyFxUSB30DeviceDscr;
            }
            else{
                buffer = (uint8_t *)CyFxUSB20DeviceDscr;
            }
            length = buffer[0];
            break;

        case CY_U3P_BOS_DESCR:
            buffer = (uint8_t *)CyFxUSBBOSDscr;
            length = (buffer[2] | ((uint16_t)buffer[3] << 8));
            break;

        case CY_U3P_USB_DEVQUAL_DESCR:
            buffer = (uint8_t *)CyFxUSBDeviceQualDscr;
            length = buffer[0];
            break;

        case CY_U3P_USB_CONFIG_DESCR:
            if (usbSpeed == CY_U3P_SUPER_SPEED){
                buffer = (uint8_t *)CyFxUSBSSConfigDscr;
            }
            else if (usbSpeed == CY_U3P_HIGH_SPEED){
                buffer = (uint8_t *)CyFxUSBHSConfigDscr;
            }
            else{ /* CY_U3P_FULL_SPEED */
                buffer = (uint8_t *)CyFxUSBFSConfigDscr;
            }
            buffer[1] = CY_U3P_USB_CONFIG_DESCR;                /* Mark as configuration descriptor. */
            length    = (buffer[2] | ((uint16_t)buffer[3] << 8));
            break;

        case CY_U3P_USB_OTHERSPEED_DESCR:
            if (usbSpeed == CY_U3P_HIGH_SPEED){
                buffer = (uint8_t *)CyFxUSBFSConfigDscr;
            }
            else if (usbSpeed == CY_U3P_FULL_SPEED){
                buffer = (uint8_t *)CyFxUSBHSConfigDscr;
            }
            else{
                /* Do nothing. buffer = NULL. */
            }

            if (buffer != NULL){
                buffer[1] = CY_U3P_USB_OTHERSPEED_DESCR;        /* Mark as other speed configuration descriptor. */
                length    = (buffer[2] | ((uint16_t)buffer[3] << 8));
            }
            break;

        case CY_U3P_USB_STRING_DESCR:
            index = wValue & 0xFF;
            switch (index){
                case 0:
                    buffer = (uint8_t *)CyFxUSBStringLangIDDscr;
                    break;

                case 1:
                    buffer = (uint8_t *)CyFxUSBManufactureDscr;
                    break;

                case 2:
                    buffer = (uint8_t *)CyFxUSBProductDscr;
                    break;

                default:
                    /* Do nothing. buffer = NULL. */
                    break;
            }
            length = buffer[0];
            break;

        default:
            /* Do nothing. buffer = NULL. */
            break;
    }

    if (buffer != NULL){
        /* Send only the minimum of actual descriptor length
         * and the requested length. */
        length = (wLength < length) ? wLength : length;
        status = CyU3PUsbSendEP0Data (length, buffer);
    }
    else{
        status = CY_U3P_ERROR_FAILURE;
    }

    return status;
}

/* Callback to handle the USB setup requests. */
CyBool_t usb_setup_cb (uint32_t setupdat0, uint32_t setupdat1){

    CyBool_t isHandled = CyTrue;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

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


    /*Three commands are available to the user
     * Reset to Boot: Resets the USB Device to a programmable setup
     * FPGA Configurtion mode: Switch to the mode in which you can program the FPGA
     * COMM mode: Switch to a high speed parallel bus mode
     */
    if (bType == CY_U3P_USB_VENDOR_RQT){
      switch (bRequest) {
        case (RESET_TO_BOOTMODE):
          if (wLength > 0){
            break;
          }

          //Reset to boot mode
          CyU3PEventSet(&main_event, RESET_PROC_BOOT_EVENT, CYU3P_EVENT_OR);
          CyU3PUsbAckSetup();
          return CyTrue; //Return that we handled the request


        case (ENTER_FPGA_CONFIG_MODE):
          if ((bReqType & 0x80) != 0) {
            break;
          }
          //What is this flag for?
          //Extract the size from byte array
          CyU3PUsbGetEP0Data (wLength, ep0_buffer, NULL);
          file_length = (uint32_t)  ((ep0_buffer[3] << 24) |
                                     (ep0_buffer[2] << 16) |
                                     (ep0_buffer[1] << 8)  |
                                      ep0_buffer[0]);

          CyU3PDebugPrint (2, "usb_controller: buf packets: %X, %X, %X, %X",
                          ep0_buffer[3],
                          ep0_buffer[2],
                          ep0_buffer[1],
                          ep0_buffer[0]);

          CyU3PDebugPrint (2, "usb_controller: File Length: %X, %d", file_length, file_length);
          CONFIG_DONE = CyTrue;
          //Set CONFIG FPGA APP Start Event to start configurin the FPGA
          CyU3PEventSet (&main_event, ENTER_FPGA_CONFIG_MODE_EVENT, CYU3P_EVENT_OR);
          return CyTrue;  //Return that we handled the request

        case (ENTER_FPGA_COMM_MODE):
          if ((bReqType & 0x80) != 0x80){
            break;
          }
          ep0_buffer[0] == CONFIG_DONE;
          CyU3PUsbSendEP0Data (wLength, ep0_buffer);
          //Switch to Slave FIFO interface when FPGA is configured successfully
          if (CONFIG_DONE) {
            CyU3PEventSet(&main_event, ENTER_FPGA_COMM_MODE_EVENT, CYU3P_EVENT_OR);
            return CyTrue; //Return that we handled the request
          }
          break;
      }
      return CyFalse;
    }


    //Don't need to specify CY_U3P_USB_STANDARD_RQT because the Vendor Request returned

    /* Identify and handle setup request. */
    switch (bRequest){
        /* This is a get status request. The response depends on the target.
         * If this is for the device, then we need to return the status
         * of the device. If this is for the endpoint, then return the stall
         * status of the endpoint. */
        case CY_U3P_USB_SC_GET_STATUS:
            CyU3PMemSet (ep0_buffer, 0, sizeof(ep0_buffer));
            if (bTarget == CY_U3P_USB_TARGET_DEVICE){
                ep0_buffer[0] = device_status;
                status = CyU3PUsbSendEP0Data (wLength, ep0_buffer);
            }
            else if (bTarget == CY_U3P_USB_TARGET_INTF){
                /* Just send zeros. */
                status = CyU3PUsbSendEP0Data (wLength, ep0_buffer);
            }
            else if (bTarget == CY_U3P_USB_TARGET_ENDPT){
                CyBool_t isStall;
                status = CyU3PUsbGetEpCfg (wIndex, NULL, &isStall);
                if (status == CY_U3P_SUCCESS){
                    ep0_buffer[0] = isStall;
                    status = CyU3PUsbSendEP0Data (wLength, ep0_buffer);
                }
            }
            else{
                isHandled = CyFalse;
            }
            break;

        /* This feature behaves differently depending upon the target.
         * If the target is device then, this request is for remote-wakeup,
         * test mode, U1_ENABLE and U2_ENABLE. If this is for the endpoint
         * then this is for clearing the endpoint stall. */
        case CY_U3P_USB_SC_CLEAR_FEATURE:
            isHandled = usb_clear_feature (bTarget, wValue, wIndex);
            break;

        /* This feature behaves differently depending upon the target.
         * If the target is device then, this request is for remote-wakeup,
         * test mode, U1_ENABLE and U2_ENABLE. If this is for the endpoint
         * then this is for clearing the endpoint stall. */
        case CY_U3P_USB_SC_SET_FEATURE:
            isHandled = usb_set_feature (bTarget, wValue, wIndex);
            break;

        /* Return the requested descriptor. */
        case CY_U3P_USB_SC_GET_DESCRIPTOR:
            status = usb_send_descriptor (wValue, wIndex, wLength);
            break;

        case CY_U3P_USB_SC_SET_DESCRIPTOR:
            /* ACK the request and do nothing. */
            break;

        /* Return the current selected configuration. */
        case CY_U3P_USB_SC_GET_CONFIGURATION:
            ep0_buffer[0] = usb_configuration;
            status = CyU3PUsbSendEP0Data (wLength, ep0_buffer);
            break;

        /* Store the value for future use and start the application. */
        case CY_U3P_USB_SC_SET_CONFIGURATION:
            if (wValue == 1){
                /* If the application is already active, then disable
                 * it before re-enabling it. */
                usb_configuration = wValue;
                if (BASE_APP_ACTIVE){
                    usb_stop ();
                }
                /* Start the loop back function. */
                usb_start ();
            }
            else{
            	if (wValue == 0){
                    /* Stop the loop back function. */
                    usb_configuration = wValue;
                    if (BASE_APP_ACTIVE){
                        usb_stop ();
                    }
            	}
            	else{
            		/* Invalid configuration value. Fail the request. */
            		CyU3PUsbStall (0, CyTrue, CyFalse);
            	}
            }
            break;

        /* Return the current selected interface. */
        case CY_U3P_USB_SC_GET_INTERFACE:
            ep0_buffer[0] = usb_interface;
            status = CyU3PUsbSendEP0Data (wLength, ep0_buffer);
            break;

        /* Store the selected interface value. */
        case CY_U3P_USB_SC_SET_INTERFACE:
            usb_interface = wValue;
            break;

        case CY_U3P_USB_SC_SET_SEL:
            {
                if ((CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED) && (wValue == 0) && (wIndex == 0) && (wLength == 6)){
                    status = CyU3PUsbGetEP0Data (32, select_buffer, 0);
                }
                else{
                    isHandled = CyFalse;
                }
            }
            break;

        case CY_U3P_USB_SC_SET_ISOC_DELAY:
            {
                if ((CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED) || (wIndex != 0) || (wLength != 0))
                    isHandled = CyFalse;
            }
            break;

        default:
            isHandled = CyFalse;
            break;
    }

    /* If there has been an error, stall EP0 to fail the transaction. */
    if ((isHandled != CyTrue) || (status != CY_U3P_SUCCESS)){
        /* This is an unhandled setup command. Stall the EP. */
        CyU3PUsbStall (0, CyTrue, CyFalse);
    }
    else{
        CyU3PUsbAckSetup ();
    }

    return CyTrue;
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
void usb_init (void) {
    CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

    /* Start the USB functionality. */
    retval = CyU3PUsbStart();
    if (retval != CY_U3P_SUCCESS){
        CyU3PDebugPrint (4, "CyU3PUsbStart failed to Start, Error code = %d\n", retval);
        CyFxAppErrorHandler(retval);
    }

    /* Allocate a buffer to hold the SEL (System Exit Latency) values set by the host. */
    select_buffer = (uint8_t *)CyU3PDmaBufferAlloc (32);
    CyU3PMemSet (select_buffer, 0, 32);

    /* In this example the application handles all the setup packets.
     * But even in normal mode of enumeration, it is possible to register
     * DMA descriptors with the library and not handle inside the setup
     * callback. */
    CyU3PUsbRegisterSetupCallback(usb_setup_cb, CyFalse);

    /* Setup the callback to handle the USB events. */
    CyU3PUsbRegisterEventCallback(usb_event_cb);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback(lpm_request_cb);

    /* Connect the USB Pins with super speed operation enabled. */
    retval = CyU3PConnectState(CyTrue, CyTrue);
    if (retval != CY_U3P_SUCCESS){
        CyU3PDebugPrint (4, "USB Connect failed, Error code = %d\n", retval);
        CyFxAppErrorHandler(retval);
    }
}




