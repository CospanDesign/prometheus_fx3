#ifndef __COMM_CONTROLLER_H__
#define __COMM_CONTROLLER_H__

void comm_config_init(void);
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


void comm_gpio_init();

#endif //__COMM_CONTROLLER_H__
