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
#include "gpio_controller.h"

#include "prometheus.h"
//#include "cypress_usb_defines.h"
#include "cyfxgpif2config.h"
#include "comm_controller.h"

extern CyBool_t GPIO_INITIALIZED;

CyBool_t COMM_APP_ACTIVE = CyFalse;      /* Whether the loopback application is active or not. */
CyU3PDmaChannel COMM_CHANNEL_USB_TO_GPIF;
CyU3PDmaChannel COMM_CHANNEL_GPIF_TO_USB;

void comm_config_start(void){
  uint16_t size = 0;
  CyU3PEpConfig_t               ep_config;
  CyU3PDmaChannelConfig_t       dma_config;
  CyU3PReturnStatus_t           retval        = CY_U3P_SUCCESS;
  CyU3PUSBSpeed_t               usb_speed     = CyU3PUsbGetSpeed();

  //Identify the USB Speed
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
      CyU3PDebugPrint(4, "Error! Invalid USB Speed.");
      CyFxAppErrorHandler(CY_U3P_ERROR_FAILURE);
      break;
  }


  //Setup the endpoint
  CyU3PMemSet ((uint8_t *) &ep_config, 0, sizeof(ep_config));
  ep_config.enable    = CyTrue;
  ep_config.epType    = CY_U3P_USB_EP_BULK;
  //ep_config.burstLen  = BURST_LEN;      //XXX: This is defined within the promtheus.h, Not sure how to set this
  ep_config.burstLen  = 1;
    //Burst length is 1 so that only a packet size is read in
  ep_config.streams   = 0;              //XXX: I think this is related to USB 3.0 Super Speed
  ep_config.pcktSize  = size;

  //Configure Producer
  retval = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &ep_config);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "com_config_start: Error setting up producer endpoint: Error code: %d", retval);
    CyFxAppErrorHandler (retval);
  }

  //Configure DMA Channels

  //Create a DMA Auto Channel that will interleave output from USB to the PPORT
  CyU3PMemSet ((uint8_t *) &dma_config, 0, sizeof(dma_config));
  //dma_config.size           = DMA_BUF_SIZE * size; //Increase bufer size for higher performance
  dma_config.size           = size; //Increase bufer size for higher performance
  dma_config.count          = CY_FX_EP_COMM_DMA_BUF_COUNT_U_2_P;
  dma_config.prodSckId      = PROMETHEUS_PRODUCER_USB_SOCKET;
  dma_config.consSckId      = PROMETHEUS_CONSUMER_PPORT_0;
  dma_config.dmaMode        = CY_U3P_DMA_MODE_BYTE;

  //Enable a callback for the producer event
  dma_config.notification   = 0;
  dma_config.cb             = NULL;
  dma_config.prodHeader     = 0;
  dma_config.prodFooter     = 0;
  dma_config.consHeader     = 0;
  dma_config.prodAvailCount = 0;

  retval = CyU3PDmaChannelCreate (  &COMM_CHANNEL_USB_TO_GPIF,
                                    CY_U3P_DMA_TYPE_AUTO_SIGNAL, &dma_config);
  if (retval != CY_U3P_SUCCESS) {
    CyU3PDebugPrint(4, "com_config_start: Error creating DMA USB to GPIF DMA Channel: Error code: %d", retval);
    CyFxAppErrorHandler(retval);
  }

  //Clear out the endpoints
  CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);

  //Set DMA USB to GPIF Channel Transfer Size
  retval = CyU3PDmaChannelSetXfer ( &COMM_CHANNEL_USB_TO_GPIF,
                                    CY_FX_COMM_DMA_TX_SIZE);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "com_config_start: Error setting DMA USB to GPIF DMA Channel Transfer Size: Error code: %d", retval);
    CyFxAppErrorHandler(retval);

  }

  //Configure the Consumer
  //Setup the endpoint
  CyU3PMemSet ((uint8_t *) &ep_config, 0, sizeof(ep_config));
  ep_config.enable    = CyTrue;
  ep_config.epType    = CY_U3P_USB_EP_BULK;
  //ep_config.burstLen  = BURST_LEN;      //XXX: This is defined within the promtheus.h, Not sure how to set this
  ep_config.burstLen  = 1;
    //Burst length is 1 so that only a packet size is read in
  ep_config.streams   = 0;              //XXX: I think this is related to USB 3.0 Super Speed
  ep_config.pcktSize  = size;

  //Configure Consumer
  retval = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &ep_config);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "com_config_start: Error setting up consumer endpoint: Error code: %d", retval);
    CyFxAppErrorHandler (retval);
  }



  //Create a DMA Auto Channel that will interleave output from PPORT to the USB
  CyU3PMemSet ((uint8_t *) &dma_config, 0, sizeof(dma_config));
  //dma_config.size           = DMA_BUF_SIZE * size; //Increase buffer size for higher performance
  dma_config.size           = size; //Increase buffer size for higher performance
  dma_config.count          = CY_FX_EP_COMM_DMA_BUF_COUNT_P_2_U;  //Increace buffer count for higher performacne
  dma_config.prodSckId      = PROMETHEUS_PRODUCER_PPORT_0;
  dma_config.consSckId      = PROMETHEUS_CONSUMER_USB_SOCKET;
  dma_config.dmaMode        = CY_U3P_DMA_MODE_BYTE;

  //Enable a callback for the producer event
  dma_config.notification   = 0;
  dma_config.cb             = NULL;
  dma_config.prodHeader     = 0;
  dma_config.prodFooter     = 0;
  dma_config.consHeader     = 0;
  dma_config.prodAvailCount = 0;

  //Create Consumer Endpoint
  dma_config.cb = NULL;

  retval = CyU3PDmaChannelCreate(   &COMM_CHANNEL_GPIF_TO_USB,
                                    CY_U3P_DMA_TYPE_AUTO_SIGNAL,
                                    &dma_config);
  if (retval != CY_U3P_SUCCESS) {
    CyU3PDebugPrint(4, "com_config_start: Error creating DMA GPIF to USB DMA Channel: Error code: %d", retval);
    CyFxAppErrorHandler(retval);
  }

  CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

  //Set DMA GPIF to USB Channel Transfer Size
  retval = CyU3PDmaChannelSetXfer ( &COMM_CHANNEL_GPIF_TO_USB,
                                    CY_FX_COMM_DMA_RX_SIZE);

  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "com_config_start: Error setting DMA GPIF to USB DMA Channel Transfer Size: Error code: %d", retval);
    CyFxAppErrorHandler(retval);

  }

  COMM_APP_ACTIVE = CyTrue;
}

void comm_config_stop(void){
  CyU3PEpConfig_t ep_config;
  CyU3PReturnStatus_t retval  = CY_U3P_SUCCESS;

  //Update the flag
  COMM_APP_ACTIVE = CyFalse;
  //Flush the Endpoints
  CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
  CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

  //Destroy the channel
  CyU3PDmaChannelDestroy (&COMM_CHANNEL_USB_TO_GPIF);
  CyU3PDmaChannelDestroy (&COMM_CHANNEL_GPIF_TO_USB);

  //Disable the endpoints
  CyU3PMemSet((uint8_t *) &ep_config, 0, sizeof(ep_config));

  if (retval != CY_U3P_SUCCESS) {
    CyU3PDebugPrint(4, "comm_config_stop: Failed to clear the endpoint structure: Error code: %d", retval);
    CyFxAppErrorHandler (retval);
  }

  ep_config.enable = CyFalse;
  retval = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &ep_config);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "comm_config_stop: Failed to disable the comm producer endpoint: Error code: %d", retval);
  }

  retval = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &ep_config);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "comm_config_stop: Failed to disable the comm consumer endpoint: Error code: %d", retval);
  }
}

void comm_configure_mcu(void){

  CyU3PIoMatrixConfig_t io_cfg;
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

  CyU3PPibClock_t pib_clock;

  io_cfg.useUart          = CyTrue;
  io_cfg.useI2C           = CyTrue;
  io_cfg.useI2S           = CyFalse;
  io_cfg.useSpi           = CyFalse;
  io_cfg.isDQ32Bit        = CyTrue;
  io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_DEFAULT;

  io_cfg.gpioSimpleEn[0]  = LOW_GPIO_DIR;
  io_cfg.gpioSimpleEn[1]  = HIGH_GPIO_DIR;
  io_cfg.gpioComplexEn[0] = 0;
  io_cfg.gpioComplexEn[1] = 0;

  retval = CyU3PDeviceConfigureIOMatrix (&io_cfg);
  if (retval != CY_U3P_SUCCESS){
    while (1);    /* Cannot recover from this error. */
  }
  if (GPIO_INITIALIZED) {
    gpio_deinit();
  }
  comm_gpio_configure_standard();

  //setup the P-Block
  pib_clock.clkDiv      = 2; //XXX: I need to figure out how to setup 100MHz clock (input from the FPGA)
  pib_clock.clkSrc      = CY_U3P_SYS_CLK; //XXX: How to use the external clock?
  pib_clock.isHalfDiv   = CyFalse;
  pib_clock.isDllEnable = CyFalse;
  //Initializes the PPort
  retval = CyU3PPibInit(CyTrue, &pib_clock);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "comm_configure_mcu: P-Port Initialization Failed: Error code: %d", retval);
    CyFxAppErrorHandler (retval);
  }

  //Load the GPIF Configuration generated from the GPIF II Designer
  retval = CyU3PGpifLoad(&CyFxGpifConfig);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "comm_configure_mcu: Failed to load GPIF configuration: Error code: %d", retval);
    CyFxAppErrorHandler(retval);
  }

  //Confiugre the sockets
  //                      Thread, Associated Socket, Watermark, Set the
  //                      Partial Flag, Min Number of words to transfer before sending flag
  CyU3PGpifSocketConfigure(0,                            //Thread
                           PROMETHEUS_CONSUMER_PPORT_0,  //Socket  (0)
                           6,                            //Watermark
                           CyFalse,                      //Emit a notification
                           1);                           //Threshold of DMA Flags

  CyU3PGpifSocketConfigure(1,                            //Thread
                           PROMETHEUS_PRODUCER_PPORT_0,  //Socket  (2)
                           6,                            //Watermark
                           CyFalse,                      //Emit a notification
                           1);                           //Threshold of DMA Flags
  //Start the state machine
  retval = CyU3PGpifSMStart (RESET, ALPHA_RESET);
  if (retval != CY_U3P_SUCCESS){
    CyU3PDebugPrint(4, "comm_configure_mcu: Failed to start GPIF state machine: Error code: %d", retval);
    CyFxAppErrorHandler(retval);
  }

}

void comm_gpio_configure_standard(){
  CyU3PGpioClock_t gpio_clock;
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;
  GPIO_INITIALIZED = CyFalse;

  gpio_clock.fastClkDiv = 2;
  gpio_clock.slowClkDiv = 0;
  gpio_clock.simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
  gpio_clock.clkSrc     = CY_U3P_SYS_CLK;
  gpio_clock.halfDiv    = 0;

  retval = CyU3PGpioInit(&gpio_clock, gpio_interrupt);
  if (retval != 0) {
    CyU3PDebugPrint(4, "comm_gpio_configure_standard: Failed to initialize the GPIO: Error Code: %d", retval);
    CyFxAppErrorHandler(retval);
  }

  //Configure Input Pins
  //               Name                 Pull Up  Pull Down  Override
  gpio_setup_input(ADJ_REG_EN,          CyFalse, CyTrue,    CyTrue);
  gpio_setup_input(DONE,                CyFalse, CyFalse,   CyTrue);
  gpio_setup_input(INIT_N,              CyFalse, CyFalse,   CyTrue);
  gpio_setup_input(FMC_DETECT_N,        CyFalse, CyFalse,   CyTrue);
  gpio_setup_input(FMC_POWER_GOOD_IN,   CyFalse, CyFalse,   CyTrue);

  //Configure Output Pins
  //                Name                Default   Override
  gpio_setup_output(FPGA_SOFT_RESET,    CyTrue,   CyFalse);
  gpio_setup_output(UART_EN,            CyFalse,  CyFalse);
  //gpio_setup_output(OTG_5V_EN,          CyFalse,  CyTrue);
  gpio_setup_output(POWER_SELECT_0,     CyFalse,  CyTrue);
  gpio_setup_output(POWER_SELECT_1,     CyTrue,  CyTrue);
  gpio_setup_output(FMC_POWER_GOOD_OUT, CyFalse,  CyTrue);

  gpio_release(ADJ_REG_EN);
  gpio_setup_output(ADJ_REG_EN,         CyFalse,  CyFalse);
  retval = CyU3PGpioSetValue(ADJ_REG_EN, CyTrue);
  CyU3PThreadSleep (200);
  //Re-enable the Regulator
  retval = CyU3PGpioSetValue(ADJ_REG_EN, CyFalse);


  GPIO_INITIALIZED = CyTrue;
}

CyBool_t is_comm_enabled(void){
  return COMM_APP_ACTIVE;
}

void comm_flush_outputs(void){
  CyU3PDmaChannelReset    (&COMM_CHANNEL_USB_TO_GPIF);
  CyU3PUsbFlushEp         (CY_FX_EP_PRODUCER);
  CyU3PUsbResetEp         (CY_FX_EP_PRODUCER);
  CyU3PDmaChannelSetXfer  (&COMM_CHANNEL_USB_TO_GPIF, CY_FX_COMM_DMA_TX_SIZE);
}
void comm_flush_inputs(void){
  CyU3PDmaChannelReset    (&COMM_CHANNEL_GPIF_TO_USB);
  CyU3PUsbFlushEp         (CY_FX_EP_CONSUMER);
  CyU3PUsbResetEp         (CY_FX_EP_CONSUMER);
  CyU3PDmaChannelSetXfer  (&COMM_CHANNEL_GPIF_TO_USB, CY_FX_COMM_DMA_RX_SIZE);
}
