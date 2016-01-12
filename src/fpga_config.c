#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3gpio.h"
#include "cyu3spi.h"
#include "cyu3uart.h"
#include "cyu3gpif.h"
#include "cyu3pib.h"
#include "pib_regs.h"


#include "fpga_config.h"
#include "gpio_controller.h"
#include "prometheus.h"
#include "usb_controller.h"
//#include "cypress_usb_defines.h"  //Defines USB Endpoints/DMA Stuff


//Globals
CyU3PDmaChannel FPGA_CONFIG_CHANNEL;
CyBool_t CONFIG_DONE = CyTrue;
CyBool_t FPGA_CONFIG_APP_ACTIVE = CyFalse;


//File Globals
uint32_t  file_length = 0;
uint16_t  ui_packet_size = 0;

CyU3PReturnStatus_t config_fpga (uint32_t ui_len){
  //Conifgure an FPGA
  uint32_t ui_idx;
  uint32_t retval = CY_U3P_SUCCESS;
  uint32_t read_count = 0;
  uint32_t write_count = 0;
  CyU3PDmaBuffer_t in_buffer;
  CyBool_t fpga_done;
  CyBool_t fpga_init;

  CyU3PDebugPrint(1, "config_fpga: File Length: %d\n", ui_len);

  retval = CyU3PSpiSetSsnLine(CyFalse);
  CyU3PThreadSleep(10);
  CyU3PGpioGetValue(INIT_N, &fpga_init);
  CyU3PGpioGetValue(INIT_N, &fpga_init);
  //In the example they got the value of fpga_init twice
  if (fpga_init) {
    //Something went wrong with configuration
    CyU3PDebugPrint(4, "config_fpga: Program is low and INIT_B is high!");
    CONFIG_DONE = CyFalse;
    return retval;
  }

  CyU3PThreadSleep(10);
  //Bring the Program Line high
  retval |= CyU3PSpiSetSsnLine(CyTrue);
  CyU3PThreadSleep(10);

  //Check if the FPGA is now ready by testing the FPGA Init Signal
  retval |= CyU3PGpioSimpleGetValue(INIT_N, &fpga_init);
  if ( (fpga_init != CyTrue) || (retval != CY_U3P_SUCCESS)) {
    if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint(4, "Failed to get the value of INIT_B: Error code: %d", retval);
    }
    CyU3PDebugPrint(4, "Init line is low even when the Done pin is released");

    return retval;
  }

  //1. Read data from the manual DMA Channel
  //2. Send it to the FPGA through SPI bus
  //3. Discard the buffer in the DMA

  retval = CY_U3P_SUCCESS;
  for (ui_idx = 0; (ui_idx < ui_len) && FPGA_CONFIG_APP_ACTIVE; ui_idx += ui_packet_size){
    //Wait 2 seconds to receive all data from the Transfer from the host
    retval = CyU3PDmaChannelGetBuffer (&FPGA_CONFIG_CHANNEL, &in_buffer, 2000);  //Wait 2 seconds
    if (retval != CY_U3P_SUCCESS){
      CONFIG_DONE  = CyFalse;
      CyU3PDebugPrint(4, "config_fpga: CyU3PDmaChannelGetBuffer fail: Error code: %d", retval);
      break;
    }
    read_count++;

    //Transmit the data over the SPI bus
    retval = CyU3PSpiTransmitWords(in_buffer.buffer, ui_packet_size);
    if (retval != CY_U3P_SUCCESS){
      CONFIG_DONE = CyFalse;
      CyU3PDebugPrint(4, "config_fpga: CyU3PSpiTransmitWords fail: Error code: %d", retval);
      break;
    }
    write_count++;

    //Get rid of the data within the DMA Channel
    retval = CyU3PDmaChannelDiscardBuffer(&FPGA_CONFIG_CHANNEL);
    if (retval != CY_U3P_SUCCESS){
      CONFIG_DONE = CyFalse;
      CyU3PDebugPrint(4, "config_fpga: CyU3PDmaChannelDiscardBuffer fail: Error code: %d", retval);
      break;
    }
  }

  if (retval != CY_U3P_SUCCESS) {
    //Sent all the configuration data to the FPGA
    CyU3PDebugPrint(1, "config_data: Received %d packets (512 byte packets) from DMA", read_count);
    CyU3PDebugPrint(1, "config_data: Wrote %d packets (512 byte packets) to SPI", write_count);
    return retval;
  }

  //Give other threads a chance to do something
  CyU3PThreadSleep(10);

  //Check if FPGA Done signal is asserted
  retval |= CyU3PGpioSimpleGetValue(DONE, &fpga_done);
  if (fpga_done != CyTrue){
    CyU3PDebugPrint(4, "config_fpga: Done pin wasn't asserted, program failed");
    CONFIG_DONE  = CyFalse;
    retval = CY_U3P_ERROR_FAILURE;
    return retval;
  }
  CyU3PDebugPrint(2, "config_fpga: Done pin asserted, FPGA Programmed");
  return retval;
}

void fpga_config_setup (void){
  uint16_t size = 0;
  CyU3PEpConfig_t ep_config;
  CyU3PDmaChannelConfig_t dma_config;
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;
  CyU3PUSBSpeed_t usb_speed = CyU3PUsbGetSpeed();

  CyU3PDebugPrint (2, "fpga_config_setup: Set up FX3 to program FPGA");
  //Get the USB Speed
  switch (usb_speed){
    case CY_U3P_FULL_SPEED:
      size = 64;
      break;
    case CY_U3P_HIGH_SPEED:
      size = 512;
      break;
    case CY_U3P_SUPER_SPEED:
      size = 1024;
      break;
    default:
      CyU3PDebugPrint(4, "fpga_config_setup: Error! Invalid USB speed.");
      CyFxAppErrorHandler(CY_U3P_ERROR_FAILURE);
      break;
  }

  //Setup the endpoint configuration
  CyU3PMemSet((uint8_t *) &ep_config, 0, sizeof (ep_config)); //Clear out the ep_config mem space to 0
  ep_config.enable   = CyTrue;
  ep_config.epType   = CY_U3P_USB_EP_BULK;
  ep_config.burstLen = 1;
  ep_config.streams  = 0;
  ep_config.pcktSize = size;

  //Set the global variable of the packet size
  ui_packet_size = size;

  //Producer Endpoint (incomming relative to the MCU)
  retval = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &ep_config);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "fpga_config_setup: Failed to setup endpoint for FPGA Config: Error Code: %d", retval);
    CyFxAppErrorHandler(retval);
  }

  //Flush the endpoint memory
  CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);

  //Create a DMA Manual Channel for Host to MCU Transfer, manual because we
  //have to transfer the data to the SPI core in a thread as apposed to
  //automatically transfering it
  dma_config.size           = size;
  dma_config.count          = CY_FX_COMM_DMA_BUF_COUNT;
  dma_config.prodSckId      = PROMETHEUS_PRODUCER_USB_SOCKET;
  dma_config.consSckId      = CY_U3P_CPU_SOCKET_CONS;
  dma_config.dmaMode        = CY_U3P_DMA_MODE_BYTE;

  //Enable the callback for the producer event
  dma_config.notification   = 0;
  dma_config.cb             = NULL;
  dma_config.prodHeader     = 0;
  dma_config.prodFooter     = 0;
  dma_config.consHeader     = 0;
  dma_config.prodAvailCount = 0;

  retval = CyU3PDmaChannelCreate(&FPGA_CONFIG_CHANNEL, CY_U3P_DMA_TYPE_MANUAL_IN, &dma_config);
  if (retval != CY_U3P_SUCCESS) {
    CyU3PDebugPrint(4, "fpga_config_setup: failed to create DMA Channel: Error Code: %d", retval);
    CyFxAppErrorHandler(retval);
  }

  //setup the DMA Channel transfer size
  retval = CyU3PDmaChannelSetXfer (&FPGA_CONFIG_CHANNEL, CY_FX_COMM_DMA_TX_SIZE);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "fpga_config_setup: Failed to setup DMA Channel transfer size, Error code: %d", retval);
  }
  CyU3PDebugPrint (2, "fpga_config_setup: Initialized FPGA config mode");
  FPGA_CONFIG_APP_ACTIVE = CyTrue;
}

void fpga_config_stop(void){
  CyU3PGpioDeInit();
  CyU3PSpiDeInit();
  //Update the flag
  FPGA_CONFIG_APP_ACTIVE = CyFalse;
  //Flush the Endpoint
  CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
  //Destroy the channel
  CyU3PDmaChannelDestroy(&FPGA_CONFIG_CHANNEL);
}

CyU3PReturnStatus_t fpga_config_init(void) {
  CyU3PIoMatrixConfig_t io_cfg;
  CyU3PSpiConfig_t  spi_config;
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

  //Setup the io ports: Disable the 32nd bit to allow usage of the SPI controller
  io_cfg.isDQ32Bit        = CyFalse;
  io_cfg.useUart          = CyTrue;
  io_cfg.useI2C           = CyTrue;
  io_cfg.useI2S           = CyFalse;
  io_cfg.useSpi           = CyTrue;
  io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_DEFAULT;

  //These are taken from prometheus.h
  io_cfg.gpioSimpleEn[0]  = LOW_GPIO_DIR;
  io_cfg.gpioSimpleEn[1]  = HIGH_GPIO_DIR;

  io_cfg.gpioComplexEn[0] = 0;
  io_cfg.gpioComplexEn[1] = 0;

  //Reconfigure the IO matrix
  retval = CyU3PDeviceConfigureIOMatrix (&io_cfg);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "fpga_config_init: Failed to setup IOMatrix: Error Code: %d", retval);
  	while (1);		/* Cannot recover from this error. */
  }
  gpio_init();

  //Start the SPI Module
  retval = CyU3PSpiInit();
  if (retval != CY_U3P_SUCCESS) {
    CyU3PDebugPrint(4, "fpga_config_init: SPI Init Failed: Error Code: %d", retval);
    return retval;
  }

  //Setup the SPI Master Block
  CyU3PMemSet((uint8_t *) &spi_config, 0, sizeof(spi_config));
  spi_config.isLsbFirst = CyFalse;    //MSB First
  spi_config.cpol       = CyTrue;     //Positive polarity (Type 1 SPI, also look at ssnPol)
  spi_config.ssnPol     = CyFalse;
  spi_config.cpha       = CyTrue;
  spi_config.leadTime   = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK;
  spi_config.lagTime    = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK;
  spi_config.clock      = 40000000;   //25MHz
  spi_config.wordLen    = 8;          //8-bits per word

  retval = CyU3PSpiSetConfig(&spi_config, NULL);
  if (retval != CY_U3P_SUCCESS) {
    CyU3PDebugPrint(4, "fpga_config_init: SPI config failed: Error Code: %d", retval);
  }

  //Maybe I should change the GPIO so that I can use the reset as a data indicator
  return retval;
}

CyBool_t is_fpga_config_enabled(void){
  return FPGA_CONFIG_APP_ACTIVE;
}
