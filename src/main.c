
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

void return_to_base(void){
  if (is_fpga_config_enabled()){
    fpga_config_stop();
  }
  if (is_comm_enabled()) {
    comm_config_stop();
  }
}

/* Entry function for the main_thread. */
void main_thread_func (uint32_t input) {
	CyU3PReturnStatus_t retval = CY_U3P_SUCCESS;
  uint32_t  event_flag;

  gpio_configure_standard ();
  usb_setup_mcu();

  for (;;){
    //CyU3PThreadSleep (100);
    //This Function will block and wait for events to happen
    retval = CyU3PEventGet (&main_event,
    	                        (RESET_PROC_BOOT_EVENT        |
                               ENTER_FPGA_CONFIG_MODE_EVENT   |
                               ENTER_FPGA_COMM_MODE_EVENT     |
                               ENTER_BASE_MODE                |
                               EVT_USB_CONNECT                |
                               EVT_USB_DISCONNECT),

    	                      CYU3P_EVENT_OR_CLEAR,
                            &event_flag,
                            CYU3P_WAIT_FOREVER);

    if (retval == CY_U3P_SUCCESS){
      if (event_flag & RESET_PROC_BOOT_EVENT) {
        return_to_base();
        CyU3PDebugPrint (2, "Reset To Boot Mode in 1 second");
        CyU3PThreadSleep (1000);
        /* Disconnect from the USB host and reset the device. */
        CyU3PConnectState (CyFalse, CyTrue);
        CyU3PThreadSleep (1000);
        CyU3PDeviceReset (CyFalse);
        for (;;);
      }
      if (event_flag & EVT_USB_CONNECT){
        CyU3PDebugPrint (2, "USB Connected");
        usb_start();
        return_to_base();
      }
      if (event_flag & ENTER_FPGA_CONFIG_MODE_EVENT){
        //Return to base mode
        return_to_base();
        if (is_gpio_enabled()) {
          gpio_deinit();
        }

        //Setup the chip to handle FPGA Config USB Transactions
        fpga_confiure_mcu();
        fpga_configure_usb();
        //Perform the actual configuration
        retval = config_fpga();
        if (retval != CY_U3P_SUCCESS)
          CyU3PDebugPrint(4, "Failed to program FPGA");
        else
          CyU3PDebugPrint(4, "Programed FPGA");

        //Put the chip back to how we found it
        return_to_base();
        //Set the GPIOs up in their normal configuration
        gpio_configure_standard();
      }
      if (event_flag & ENTER_FPGA_COMM_MODE_EVENT){
        return_to_base();
        CyU3PDebugPrint(4, "Return to base");
        comm_configure_mcu();
        CyU3PDebugPrint(4, "Configure COMM MCU");
        comm_config_start();
        CyU3PDebugPrint(4, "Enter Comm Mode");
      }
      if (event_flag & EVT_ENTER_BASE_MODE){
        return_to_base();
        //gpio_configure_standard();
        CyU3PDebugPrint(4, "Enter Base Mode");
      }
      if (event_flag & EVT_USB_DISCONNECT){
        CyU3PDebugPrint (2, "USB Disconnect");
        return_to_base();
        usb_stop();
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
        main_thread_func,            /* Bulk loop App Thread Entry function */
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

