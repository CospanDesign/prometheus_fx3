#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3gpio.h>
#include <cyu3uart.h>

#include "prometheus.h"
#include "gpio_controller.h"

#define CY_FX_GPIOAPP_GPIO_HIGH_EVENT    (1 << 0)   /* GPIO high event */
#define CY_FX_GPIOAPP_GPIO_LOW_EVENT     (1 << 1)   /* GPIO low event */


void gpio_init(void);
void gpio_interrupt( uint8_t gpio_id);

void setup_input_gpio(uint32_t pinnum,
                      CyBool_t pull_up,
                      CyBool_t pull_down,
                      CyBool_t override);
void setup_output_gpio(uint32_t pinnum,
                       CyBool_t default_value,
                       CyBool_t override);



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
void setup_input_gpio(uint32_t pinnum,
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
void setup_output_gpio(uint32_t pinnum,
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
                (CY_FX_GPIOAPP_GPIO_HIGH_EVENT | CY_FX_GPIOAPP_GPIO_LOW_EVENT),
                CYU3P_EVENT_OR_CLEAR, &event_flag, CYU3P_WAIT_FOREVER);
        if (retval == CY_U3P_SUCCESS){
            if (event_flag & CY_FX_GPIOAPP_GPIO_HIGH_EVENT){
                /* Print the status of the pin */
                CyU3PDebugPrint (2, "GPIO Went High\n");
            }
            else{
                /* Print the status of the pin */
                CyU3PDebugPrint (2, "GPIO Went Low\n");
            }
        }
    }
}

void gpio_init(){
    CyU3PGpioClock_t gpio_clock;
    CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;


    gpio_clock.fastClkDiv = 2;
    gpio_clock.slowClkDiv = 0;
    gpio_clock.simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpio_clock.clkSrc     = CY_U3P_SYS_CLK;
    gpio_clock.halfDiv    = 0;

    retval = CyU3PGpioInit(&gpio_clock, gpio_interrupt);
    if (retval != 0) {
      CyU3PDebugPrint(4, "gpio_init: Failed to initialize the GPIO: Error Code: %d", retval);
      CyFxAppErrorHandler(retval);
    }

    //Configure Input Pins
    //               Name                 Pull Up  Pull Down  Override
    setup_input_gpio(PROC_BUTTON,         CyFalse, CyFalse,   CyTrue);
    setup_input_gpio(DONE,                CyFalse, CyFalse,   CyFalse);
    setup_input_gpio(INIT_N,              CyFalse, CyFalse,   CyFalse);
    //setup_input_gpio(FMC_DETECT_N,        CyFalse, CyFalse,   CyFalse);
    //setup_input_gpio(FMC_POWER_GOOD_IN,   CyFalse, CyFalse,   CyFalse);

    //Configure Output Pins
    //                Name                Default   Override
    setup_output_gpio(FPGA_SOFT_RESET,    CyFalse,  CyFalse);
    setup_output_gpio(UART_EN,            CyFalse,  CyFalse);
    //setup_output_gpio(OTG_5V_EN,          CyFalse,  CyTrue);
    //setup_output_gpio(POWER_SELECT_0,     CyFalse,  CyTrue);
    //setup_output_gpio(POWER_SELECT_1,     CyFalse,  CyTrue);
    //setup_output_gpio(FMC_POWER_GOOD_OUT, CyFalse,  CyTrue);
}
