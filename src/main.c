
//Author: dave.mccoy@cospandesign.com

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"

//#include "cypress_usb_defines.h"

#include "prometheus.h"
#include "gpio_controller.h"
#include "usb_controller.h"
#include "fpga_config.h"


extern CyBool_t COMM_APP_ACTIVE;              /* Comm Mode Enabled */
extern CyBool_t FPGA_CONFIG_APP_ACTIVE;       /* FPGA Config Mode */
extern CyBool_t GPIO_INITIALIZED;

extern uint32_t       file_length;

CyU3PThread     gpio_out_thread;              /* GPIO thread structure */
CyU3PThread     gpio_in_thread;               /* GPIO thread structure */
CyU3PEvent      gpio_event;                   /* GPIO input event group. */
CyU3PEvent      main_event;                   /* Events that change */

CyU3PThread     main_thread;	                /* Main application thread structure */

/* Application Error Handler */
void CyFxAppErrorHandler (CyU3PReturnStatus_t status){
  CyU3PDebugPrint (4, "Error Handler: Error code: %d\n", status);
  for (;;){
    CyU3PThreadSleep (100);
  }
}

/* Entry function for the main_thread. */
void main_thread_entry (uint32_t input) {
	CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;
  uint32_t  event_flag;

  /* Initialize the debug module */
  //debug_init();

  /* Initialize GPIO module. */
  gpio_init ();

  /* Initialize the bulk loop application */
  usb_init();

  for (;;){
    //CyU3PThreadSleep (100);
    //This Function will block and wait for events to happen
    retval = CyU3PEventGet (&main_event,
    	                        (RESET_PROC_BOOT_EVENT |
                               ENTER_FPGA_CONFIG_MODE_EVENT |
                               ENTER_FPGA_COMM_MODE_EVENT |
                               EVT_SET_REG_EN_TO_OUTPUT |
                               EVT_DISABLE_REGULATOR |
                               EVT_ENABLE_REGULATOR |
                               EVT_USB_CONNECT |
                               EVT_USB_DISCONNECT),

    	                      CYU3P_EVENT_OR_CLEAR,
                            &event_flag,
                            CYU3P_WAIT_FOREVER);

    //CyU3PDebugPrint (2, "main_thread: event: 0x%08X", event_flag);
    if (retval == CY_U3P_SUCCESS){
      if (event_flag & RESET_PROC_BOOT_EVENT) {
        CyU3PDebugPrint (2, "Reset To Boot Mode in 1 second");
        CyU3PThreadSleep (1000);
        /* Disconnect from the USB host and reset the device. */
        CyU3PConnectState (CyFalse, CyTrue);
        CyU3PThreadSleep (1000);
        CyU3PDeviceReset (CyFalse);
        for (;;);
      }
      if (event_flag & EVT_USB_CONNECT){
      }
      //CyU3PDebugPrint (2, "main_thread: Received an event: 0x%08X", event_flag);
      if (event_flag & ENTER_FPGA_CONFIG_MODE_EVENT){
        //Configure the MCU to program the FPGA
        //if (COMM_APP_ACTIVE) {
        //  CyU3PDebugPrint(3, "COMM mode active, deactivating it");
        //  comm_config_stop();
        //}
        if (GPIO_INITIALIZED) {
          //CyU3PDebugPrint (2, "main_thread: Deinitializing the GPIOs");
          gpio_deinit();
        }
        if (!FPGA_CONFIG_APP_ACTIVE) {
          fpga_config_init();
          fpga_config_setup();
          debug_init();
          //CyU3PDebugPrint (2, "main_thread: Setup FPGA Config Mode");
        }
        retval = config_fpga(file_length);
        if (retval != CY_U3P_SUCCESS){
          CyU3PDebugPrint(4, "Failed to program FPGA");
        }

      }
      if (event_flag & ENTER_FPGA_COMM_MODE_EVENT){
        //Configure the MCU to use the FIFO mode
        //CyU3PDebugPrint (2, "Switch to Parallel FIFO mode");
        if (FPGA_CONFIG_APP_ACTIVE) {
          fpga_config_stop();
        }

        comm_config_init();
        comm_config_start();

        //Setup the GPIO to be an output
        retval = CyU3PGpioSetValue(ADJ_REG_EN, CyFalse);
        CyU3PThreadSleep (200);
        //Re-enable the Regulator
        retval = CyU3PGpioSetValue(ADJ_REG_EN, CyTrue);

      }
      if (event_flag & EVT_SET_REG_EN_TO_OUTPUT){
        //CyU3PDebugPrint (2, "Set Voltage Regulator Pin to an output");
        //Disable the voltage regulator enable pin
        //Enable the pin as an output
        gpio_release(ADJ_REG_EN);
        //Enable the regulator
        gpio_setup_output(ADJ_REG_EN,         CyFalse,  CyFalse);
        //CyU3PDebugPrint (2, "Voltage Regulator Pin set to an output successful!");
      }
      if (event_flag & EVT_DISABLE_REGULATOR){
        //CyU3PDebugPrint (2, "Disable Regulator");
        //Set the GPIO to high
        //is high right?
        retval = CyU3PGpioSetValue(ADJ_REG_EN, CyFalse);
      }
      if (event_flag & EVT_ENABLE_REGULATOR){
        //CyU3PDebugPrint (2, "Enable Regulator");
        //Set the GPIO to low
        //is low right?
        retval = CyU3PGpioSetValue(ADJ_REG_EN, CyTrue);
      }
    }
  }
}

/* Application define function which creates the threads. */
void CyFxApplicationDefine (void){
    void *ptr = NULL;
    uint32_t retval = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_COMM_THREAD_STACK);

    /* Create the thread for the application */
    retval = CyU3PThreadCreate (
        &main_thread,                 /* Bulk loop App Thread structure */
        "21:Main_Thread",             /* Thread ID and Thread name */
        main_thread_entry,            /* Bulk loop App Thread Entry function */
        0,                            /* No input parameter to thread */
        ptr,                          /* Pointer to the allocated thread stack */
        CY_FX_COMM_THREAD_STACK,      /* Bulk loop App Thread stack size */
        CY_FX_COMM_THREAD_PRIORITY,   /* Bulk loop App Thread priority */
        CY_FX_COMM_THREAD_PRIORITY,   /* Bulk loop App Thread priority */
        CYU3P_NO_TIME_SLICE,          /* No time slice for the application thread */
        CYU3P_AUTO_START              /* Start the Thread immediately */
        );

    /* Check the return code */
    if (retval != 0){
        while(1);
    }

    //I'm not sure if I really need this
    //Create GPIO Ouput Thread
    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retval = CyU3PThreadCreate (
        &gpio_out_thread,             /* GPIO Example App Thread structure */
        "22:GPIO_simple_output",      /* Thread ID and Thread name */
        gpio_out_thread_entry,        /* GPIO Example App Thread Entry function */
        0,                            /* No input parameter to thread */
        ptr,                          /* Pointer to the allocated thread stack */
        CY_FX_GPIOAPP_THREAD_STACK,   /* Thread stack size */
        CY_FX_GPIOAPP_THREAD_PRIORITY,/* Thread priority */
        CY_FX_GPIOAPP_THREAD_PRIORITY,/* Pre-emption threshold for the thread. */
        CYU3P_NO_TIME_SLICE,          /* No time slice for the application thread */
        CYU3P_AUTO_START              /* Start the Thread immediately */
        );

    /* Check the return code */
    if (retval != 0){
        while(1);
    }

    //Create GPIO Input Thread
    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retval = CyU3PThreadCreate (
        &gpio_in_thread,              /* GPIO Example App Thread structure */
        "23:GPIO_simple_input",       /* Thread ID and Thread name */
        gpio_in_thread_entry,         /* GPIO Example App Thread entry function */
        0,                            /* No input parameter to thread */
        ptr,                          /* Pointer to the allocated thread stack */
        CY_FX_GPIOAPP_THREAD_STACK,   /* Thread stack size */
        CY_FX_GPIOAPP_THREAD_PRIORITY,/* Thread priority */
        CY_FX_GPIOAPP_THREAD_PRIORITY,/* Pre-emption threshold for the thread */
        CYU3P_NO_TIME_SLICE,          /* No time slice for the application thread */
        CYU3P_AUTO_START              /* Start the Thread immediately */
        );
    /* Check the return code */
    if (retval != 0){
        while(1);
    }
    /* Create GPIO application event group */
    retval = CyU3PEventCreate(&gpio_event);
    retval = CyU3PEventCreate(&main_event);
    if (retval != 0){
        while(1);
    }

}

/*
 * main (pregame) This sets up the RTOS before the RTOS is an RTOS (deep)
 * Sets up:
 *
 * Clocks
 * IO Matrix
 * Threads
 *
 */

int main (void){
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the device */
    status = CyU3PDeviceInit (NULL);
    if (status != CY_U3P_SUCCESS){
        goto handle_fatal_error;
    }

    /* Initialize the caches. Enable both Instruction and Data caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyTrue, CyTrue);
    if (status != CY_U3P_SUCCESS){
        goto handle_fatal_error;
    }

    io_cfg.isDQ32Bit        = CyFalse;
    io_cfg.useUart          = CyTrue;
    io_cfg.useI2C           = CyTrue;
    io_cfg.useI2S           = CyFalse;
    io_cfg.useSpi           = CyFalse;
    io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_UART_ONLY;

    /*Enable GPIO*/
    //These are taken from prometheus.h
    io_cfg.gpioSimpleEn[0]  = LOW_GPIO_DIR;
    io_cfg.gpioSimpleEn[1]  = HIGH_GPIO_DIR;

    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;

    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS){
        goto handle_fatal_error;
    }

    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry ();
    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:
    /* Cannot recover from this error. */
    while (1);
}

/* [ ] */

