/**
* @file lv_port_indev.h
* @brief Input device interface configuration header file
* @author MQuero
* @see mquero.com
*/

#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

/**********************
*      INCLUDES
*********************/
#include "esp_lcd_touch.h"

/**********************
*      VARIABLES
**********************/
extern esp_lcd_touch_handle_t tp_handle;

/**********************
* GLOBAL PROTOTYPES
**********************/
void lv_port_indev_init(void);

#endif /*LV_PORT_INDEV_H*/
