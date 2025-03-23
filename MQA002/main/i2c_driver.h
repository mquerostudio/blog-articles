/**
 * @file i2c_driver.h
 * @brief I2C driver interface configuration header file
 * @author MQuero
 * @see mquero.com
 */

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

/**********************
*      INCLUDES
*********************/
#include "driver/i2c_master.h"

/**********************
 *      VARIABLES
 **********************/
extern i2c_master_bus_handle_t i2c_bus_handle;

/**********************
* GLOBAL PROTOTYPES
**********************/
void i2c_init(void);
void i2c_deinit(void);

#endif /*I2C_DRIVER_H*/
