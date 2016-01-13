

#ifndef __FPGA_CONFIG_H__
#define __FPGA_CONFIG_H__

CyU3PReturnStatus_t config_fpga (void);
void fpga_configure_usb (void);
void fpga_config_stop(void);
CyU3PReturnStatus_t fpga_confiure_mcu(void);
CyBool_t is_fpga_config_enabled(void);
void fpga_config_set_file_length(uint32_t in_file_length);
uint32_t fpga_config_get_file_length(void);
void fpga_flush_outputs(void);

#endif
