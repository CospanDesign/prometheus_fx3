
#ifndef __DEBUG_CONTROLLER_H__
#define __DEBUG_CONTROLLER_H__

void debug_setup (void);
void debug_destroy(void);
CyBool_t is_debug_enabled(void);
void debug_flush_inputs(void);
void debug_flush_outputs(void);

#endif //__DEBUG_CONTROLLER_H__
