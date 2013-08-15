#ifndef __USB_CONTROLLER_H__
#define __USB_CONTROLLER_H__

//Prototypes
void bulk_app_init (void);
void bulk_app_start(void);
void bulk_app_stop (void);

CyBool_t usb_set_feature (uint8_t bTarget, uint16_t wValue, uint16_t wIndex);
CyBool_t usb_clear_feature (uint8_t bTarget, uint16_t wValue, uint16_t wIndex);

CyU3PReturnStatus_t usb_send_descriptor (uint16_t wValue, uint16_t wIndex, uint16_t wLength);

CyBool_t usb_setup_cb( uint32_t setupdat0, uint32_t setupdat1);

CyBool_t lpm_request_cb (CyU3PUsbLinkPowerMode link_mode);


#endif //__USB_CONTROLLER_H__
