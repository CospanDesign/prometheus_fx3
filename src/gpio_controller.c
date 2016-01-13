#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3gpio.h>
#include <cyu3uart.h>

#include "prometheus.h"
#include "gpio_controller.h"

#define CY_FX_GPIOAPP_GPIO_HIGH_EVENT    (1 << 0)   /* GPIO high event */
#define CY_FX_GPIOAPP_GPIO_LOW_EVENT     (1 << 1)   /* GPIO low event */
CyBool_t GPIO_INITIALIZED = CyFalse;

void gpio_configure_standard(void);

extern CyU3PEvent gpio_event;    /* GPIO input event group. */

/*
 * GPIO Interrupt
 *
 * Debug Print is not available in this context
 *
 */
void gpio_interrupt( uint8_t gpio_id){
  CyBool_t gpio_value = CyFalse;
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

  //Get the status of the pin
  retval = CyU3PGpioGetValue(gpio_id, &gpio_value);

  if (retval == CY_U3P_SUCCESS){
    if (gpio_value == CyTrue){
      CyU3PEventSet(&gpio_event, CY_FX_GPIOAPP_GPIO_HIGH_EVENT, CYU3P_EVENT_OR);
    }
    else {
      CyU3PEventSet(&gpio_event, CY_FX_GPIOAPP_GPIO_LOW_EVENT, CYU3P_EVENT_OR);
    }
  }
}


//Setup an input pin
void gpio_setup_input(uint32_t pinnum,
                      CyBool_t pull_up,
                      CyBool_t pull_down,
                      CyBool_t override){

  CyU3PGpioSimpleConfig_t gpio_config;
  CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

  if (override){
    apiRetStatus = CyU3PDeviceGpioOverride (pinnum, CyTrue);
    if (apiRetStatus != 0)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "CyU3PDeviceGpioOverride failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }
  }


  gpio_config.outValue    = CyFalse;
  gpio_config.inputEn     = CyTrue;
  gpio_config.driveLowEn  = pull_down;
  gpio_config.driveHighEn = pull_up;
  gpio_config.intrMode    = CY_U3P_GPIO_INTR_BOTH_EDGE;

  apiRetStatus = CyU3PGpioSetSimpleConfig(pinnum, &gpio_config);
  if (apiRetStatus != 0){
      /* Error Handling */
      CyU3PDebugPrint (4, "CyU3PGpioInit failed, error code = %d\n", apiRetStatus);
      CyFxAppErrorHandler(apiRetStatus);
  }
}
//Setup an output pin
void gpio_setup_output(uint32_t pinnum,
                       CyBool_t default_value,
                       CyBool_t override){

  CyU3PGpioSimpleConfig_t gpio_config;
  CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

  if (override){
    apiRetStatus = CyU3PDeviceGpioOverride (pinnum, CyTrue);
    if (apiRetStatus != 0)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "CyU3PDeviceGpioOverride failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }
  }

  gpio_config.outValue    = default_value;
  gpio_config.inputEn     = CyFalse;
  gpio_config.driveLowEn  = CyTrue;
  gpio_config.driveHighEn = CyTrue;
  gpio_config.intrMode    = CY_U3P_GPIO_NO_INTR;

  apiRetStatus = CyU3PGpioSetSimpleConfig(pinnum, &gpio_config);
  if (apiRetStatus != 0){
    /* Error Handling */
    CyU3PDebugPrint (4, "CyU3PGpioInit failed, error code = %d\n", apiRetStatus);
    CyFxAppErrorHandler(apiRetStatus);
  }
}

/* Entry function for the gpioOutputThread */
void gpio_out_thread_entry (uint32_t input){
    CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

    retval = CyU3PGpioSetValue (FPGA_SOFT_RESET, CyTrue);
    if (retval != CY_U3P_SUCCESS){
        CyU3PDebugPrint (4, "set value failed, error code = %d\n", retval);
        CyFxAppErrorHandler(retval);
    }

    for (;;){
        CyU3PThreadSleep(2000);
        //CyU3PDebugPrint (1, "Tick");
        /*
        retval = CyU3PGpioSetValue (FPGA_SOFT_RESET, CyTrue);
        if (retval != CY_U3P_SUCCESS){
            CyU3PDebugPrint (4, "set value failed, error code = %d\n", retval);
            CyFxAppErrorHandler(retval);
        }
        CyU3PDebugPrint (0, "LED went on");
        //Wait for 2 seconds
        CyU3PThreadSleep(2000);

        retval = CyU3PGpioSetValue (FPGA_SOFT_RESET, CyFalse);
        if (retval != CY_U3P_SUCCESS){
            CyU3PDebugPrint (4, "set value failed, error code = %d\n", retval);
            CyFxAppErrorHandler(retval);
        }
        CyU3PDebugPrint (0, "LED went off");
        //Wait for 2 seconds
        CyU3PThreadSleep(2000);
        */
    }
}

/* Entry function for the gpioInputThread */
void gpio_in_thread_entry (uint32_t input){
    uint32_t event_flag;
    CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;

    for (;;){
        /* Wait for a GPIO event */
        retval = CyU3PEventGet (&gpio_event,
                                    (CY_FX_GPIOAPP_GPIO_HIGH_EVENT |
                                     CY_FX_GPIOAPP_GPIO_LOW_EVENT),
                                CYU3P_EVENT_OR_CLEAR,
                                &event_flag,
                                CYU3P_WAIT_FOREVER);

        if (retval == CY_U3P_SUCCESS){
            if (event_flag & CY_FX_GPIOAPP_GPIO_HIGH_EVENT){
                /* Print the status of the pin */
                //CyU3PDebugPrint (2, "GPIO Went High");
            }
            else{
                /* Print the status of the pin */
                //CyU3PDebugPrint (2, "GPIO Went Low");
            }
        }
    }
}

void gpio_deinit(){
  CyU3PGpioDeInit();
  GPIO_INITIALIZED = CyFalse;
};

void gpio_configure_standard(){
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
    CyU3PDebugPrint(4, "gpio_configure_standard: Failed to initialize the GPIO: Error Code: %d", retval);
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

  GPIO_INITIALIZED = CyTrue;
}

void gpio_release(uint32_t gpio_id){
  CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;
  if (CyU3PIsGpioSimpleIOConfigured(gpio_id) ||
      CyU3PIsGpioComplexIOConfigured(gpio_id)){
    retval = CyU3PGpioDisable(gpio_id);
    if (retval != CY_U3P_SUCCESS){
      CyU3PDebugPrint(4, "gpio_release: Failed to release the GPIO: Error Code: %d", retval);
    }
  }
}

CyBool_t is_gpio_enabled(void){
    return GPIO_INITIALIZED;
}
