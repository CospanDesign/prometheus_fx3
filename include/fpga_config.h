

#ifndef __FPGA_CONFIG_H__
#define __FPGA_CONFIG_H__

CyU3PReturnStatus_t config_fpga (uint32_t ui_len);
void fpga_config_setup (void);
void fpga_config_stop(void);
CyU3PReturnStatus_t fpga_config_init(void);
CyBool_t is_fpga_config_enabled(void);

#endif
