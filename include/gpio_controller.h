#ifndef __GPIO_CONTROLLER_H__
#define __GPIO_CONTROLLER_H__

//Prototypes
void gpio_in_thread_entry (uint32_t input);
void gpio_out_thread_entry (uint32_t input);
void gpio_interrupt( uint8_t gpio_id);

void gpio_deinit();
void gpio_init();

void gpio_release(uint32_t gpio_id);

void gpio_setup_output(uint32_t pinnum,
                       CyBool_t default_value,
                       CyBool_t override);

void gpio_setup_input(uint32_t pinnum,
                      CyBool_t pull_up,
                      CyBool_t pull_down,
                      CyBool_t override);

CyBool_t is_gpio_enabled(void);



#endif //__GPIO_CONTROLLER_H__
