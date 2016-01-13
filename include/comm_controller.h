#ifndef __COMM_CONTROLLER_H__
#define __COMM_CONTROLLER_H__

void comm_configure_mcu(void);
  //setup the MCU to use the comm controller configuration
  //  sets up GPIO, UART, I2C
  //  sets up the P-Port
  //    Clock
  //    Sockets
  //    GPIF
  //    Starts the GPIF state machine

void comm_config_start(void);
  //setup the USB communication
  //  Gets the USB Speed
  //  Configures the endpoints
  //  Creates the DMA Channels
  //  Attach one side of DMA to USB
  //  Attach other side of DMA to P-Port

void comm_config_stop(void);
  //tear down the USB communication
  //  Clear out DMA channels
  //  Remove DMA channels
  //  Disable endpoints


CyBool_t is_comm_enabled(void);

void comm_gpio_configure_standard();
void comm_flush_outputs(void);
void comm_flush_inputs(void);
#endif //__COMM_CONTROLLER_H__

