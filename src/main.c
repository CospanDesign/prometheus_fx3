/* This file illustrates the bulkloop application example using the DMA AUTO mode */

/*
   This example illustrates a loopback mechanism between two USB bulk endpoints. The example comprises of
   vendor class USB enumeration descriptors with 2 bulk endpoints. A bulk OUT endpoint acts as the producer
   of data from the host. A bulk IN endpint acts as the consumer of data to the host.

   The loopback is achieved with the help of a DMA AUTO channel. DMA AUTO channel is created between the
   producer USB bulk endpoint and the consumer USB bulk endpoint. Data is transferred from the host into
   the producer endpoint which is then directly transferred to the consumer endpoint by the DMA engine.
   CPU is not involved in the data transfer.

   The DMA buffer size is defined based on the USB speed. 64 for full speed, 512 for high speed and 1024
   for super speed. CY_FX_BULKLP_DMA_BUF_COUNT in the header file defines the number of DMA buffers.
 */

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cypress_usb_defines.h"
#include "cyu3usb.h"

#include "prometheus.h"
#include "gpio_controller.h"
#include "usb_controller.h"


CyU3PThread     gpio_out_thread;    /* GPIO thread structure */
CyU3PThread     gpio_in_thread;     /* GPIO thread structure */
CyU3PEvent      gpio_event;         /* GPIO input event group. */

CyU3PThread     main_thread;	      /* Bulk loop application thread structure */

CyBool_t reset_device = CyFalse;    /* Request to reset the FX3 device. */

/* Application Error Handler */
void CyFxAppErrorHandler (CyU3PReturnStatus_t apiRetStatus){
    for (;;){
        CyU3PThreadSleep (100);
    }
}

/* Entry function for the main_thread. */
void main_thread_entry (uint32_t input) {
    /* Initialize the debug module */
    debug_init();

    /* Initialize the bulk loop application */
    bulk_app_init();

    for (;;){
        CyU3PThreadSleep (1000);

        if (reset_device){
            /* Disconnect from the USB host and reset the device. */
            reset_device = CyFalse;
            CyU3PThreadSleep (1000);
            CyU3PConnectState (CyFalse, CyTrue);
            CyU3PThreadSleep (1000);
            CyU3PDeviceReset (CyFalse);
            for (;;);
        }
    }
}

/* Application define function which creates the threads. */
void CyFxApplicationDefine (void){
    void *ptr = NULL;
    uint32_t retvalue = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_BULKLP_THREAD_STACK);

    /* Create the thread for the application */
    retvalue = CyU3PThreadCreate (
        &main_thread,                 /* Bulk loop App Thread structure */
        "21:Bulk_loop_AUTO",          /* Thread ID and Thread name */
        main_thread_entry,            /* Bulk loop App Thread Entry function */
        0,                            /* No input parameter to thread */
        ptr,                          /* Pointer to the allocated thread stack */
        CY_FX_BULKLP_THREAD_STACK,    /* Bulk loop App Thread stack size */
        CY_FX_BULKLP_THREAD_PRIORITY, /* Bulk loop App Thread priority */
        CY_FX_BULKLP_THREAD_PRIORITY, /* Bulk loop App Thread priority */
        CYU3P_NO_TIME_SLICE,          /* No time slice for the application thread */
        CYU3P_AUTO_START              /* Start the Thread immediately */
        );

    /* Check the return code */
    if (retvalue != 0){
        while(1);
    }

    //Create GPIO Ouput Thread
    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retvalue = CyU3PThreadCreate (
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
    if (retvalue != 0){
        while(1);
    }

    //Create GPIO Input Thread
    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retvalue = CyU3PThreadCreate (
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
    if (retvalue != 0){
        while(1);
    }
    /* Create GPIO application event group */
    retvalue = CyU3PEventCreate(&gpio_event);
    if (retvalue != 0){ 
        while(1);
    }
}
/*
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

    /* Configure the IO matrix for the device. On the FX3 DVK board, the COM port
     * is connected to the IO(53:56). This means that either DQ32 mode should be
     * selected or lppMode should be set to UART_ONLY. Here we are choosing
     * UART_ONLY configuration. */
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;

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

