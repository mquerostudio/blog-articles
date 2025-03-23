/**
 * @file i2c_driver.c
 * @brief I2C driver implementation file
 * @author MQuero
 * @see mquero.com
 */

/**********************
 *      INCLUDES
 *********************/
#include "i2c_driver.h"

/**********************
 *      DEFINES
 *********************/
#define BOARD_I2C_PORT I2C_NUM_0
#define BOARD_I2C_SDA 47
#define BOARD_I2C_SCL 21

/**********************
 *      VARIABLES
 **********************/
i2c_master_bus_handle_t i2c_bus_handle;

/**********************
 * GLOBAL FUNCTIONS
 **********************/
void i2c_init(void)
{
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA,
        .scl_io_num = BOARD_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .flags.enable_internal_pullup = 0};

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));    
}

void i2c_deinit(void)
{
    ESP_ERROR_CHECK(i2c_del_master_bus(i2c_bus_handle));
}
