/**
 * @file lv_port_disp.h
 * @brief Display interface configuration header file
 * @author MQuero
 * @see mquero.com
 */

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

/**********************
 *      INCLUDES
 *********************/
#include "esp_lcd_types.h"

/**********************
 *      VARIABLES
 **********************/
extern esp_lcd_panel_handle_t panel_handle;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Initialize low level display driver */
void lv_port_disp_init(void);

#endif /*LV_PORT_DISP_H*/
