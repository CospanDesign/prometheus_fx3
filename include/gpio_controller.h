#ifndef __GPIO_CONTROLLER_H__
#define __GPIO_CONTROLLER_H__

//Prototypes
void gpio_in_thread_entry (uint32_t input);
void gpio_out_thread_entry (uint32_t input);

void gpio_deinit();
void gpio_init();

#endif //__GPIO_CONTROLLER_H__
