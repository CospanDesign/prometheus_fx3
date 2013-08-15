//Pins
#ifndef __PROMETHEUS_DEFINES__
#define __PROMETHEUS_DEFINES__

//Pin Name:GPIO
#define PROC_BUTTON        30
#define UART_EN            24
#define OTG_5V_EN          32

#define POWER_SELECT_0     27
#define POWER_SELECT_1     45

#define FMC_POWER_GOOD_OUT 25
#define FMC_POWER_GOOD_IN  26
#define FMC_DETECT_N       57

#define FPGA_SOFT_RESET    51
#define INIT_N             52
#define DONE               50

//Pin Direction
#define IN                  0
#define OUT                 1

#define PROC_BUTTON_DIR         IN
#define UART_EN_DIR             OUT
//XXX: THIS OTG 5V EN PIN IS AN INPUT ONLY BECAUSE THERE ISN"T THE CORRECT CONTROL TO CONTROL THIS CORRECTLY
#define OTG_5V_EN_DIR           IN

#define POWER_SELECT_0_DIR      OUT
#define POWER_SELECT_1_DIR      OUT

#define FMC_POWER_GOOD_OUT_DIR  OUT
#define FMC_POWER_GOOD_IN_DIR   IN
#define FMC_DETECT_N_DIR        IN

#define FPGA_SOFT_RESET_DIR     OUT
#define INIT_N_DIR              IN
#define DONE_DIR                IN

//Define the GPIO Dir Bitmap
#define LOW_GPIO_DIR  (uint32_t) (PROC_BUTTON_DIR         <<  PROC_BUTTON               | \
                                  UART_EN_DIR             <<  UART_EN                   | \
                                  POWER_SELECT_0_DIR      <<  POWER_SELECT_0            | \
                                  FMC_POWER_GOOD_OUT_DIR  <<  FMC_POWER_GOOD_OUT        | \
                                  FMC_POWER_GOOD_IN_DIR   <<  FMC_POWER_GOOD_IN)

#define HIGH_GPIO_DIR (uint32_t) (OTG_5V_EN_DIR           <<  (OTG_5V_EN - 32)          | \
                                  POWER_SELECT_1_DIR      <<  (POWER_SELECT_1 - 32)     | \
                                  FMC_DETECT_N_DIR        <<  (FMC_DETECT_N - 32)       | \
                                  FPGA_SOFT_RESET_DIR     <<  (FPGA_SOFT_RESET - 32)    | \
                                  INIT_N_DIR              <<  (INIT_N - 32)             | \
                                  DONE_DIR                <<  (DONE - 32))

#endif  //__PROMETHEUS_DEFINES__
