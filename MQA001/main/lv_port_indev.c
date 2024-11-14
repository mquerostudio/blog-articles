/**
 * @file lv_port_indev.h
 * @brief Input device interface configuration header file
 * @author MQuero
 * @see mquero.com
 */

/**********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "i2c_driver.h"
#include "esp_lcd_touch_cst816s.h"
#include "lvgl.h"

/**********************
 *      DEFINES
 *********************/
#define CONFIG_LCD_H_RES 240
#define CONFIG_LCD_V_RES 320

#define BOARD_TOUCH_IRQ (-1)
#define BOARD_TOUCH_RST (-1)

/**********************
 *      VARIABLES
 **********************/
esp_lcd_touch_handle_t tp_handle = NULL;

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t *indev_touchpad;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void touchpad_init(void);
static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_indev_init(void)
{
    /*Initialize touchpad*/
    touchpad_init();

    /*Register a touchpad input device*/
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
/*Initialize your touchpad*/
static void touchpad_init(void)
{
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config =
        ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    tp_io_config.scl_speed_hz = (400 * 1000);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config,
                                             &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_LCD_H_RES,
        .y_max = CONFIG_LCD_V_RES,
        .rst_gpio_num = BOARD_TOUCH_RST,
        .int_gpio_num = BOARD_TOUCH_IRQ,
        .levels =
            {
                .reset = 0,
                .interrupt = 0,
            },
        .flags =
            {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
        .interrupt_callback = NULL,
        .process_coordinates = NULL,
    };
    esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &tp_handle);
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint8_t touch_cnt = 0;

    esp_lcd_touch_read_data(tp_handle);
    bool touchpad_is_pressed = esp_lcd_touch_get_coordinates(
        tp_handle, touch_x, touch_y, NULL, &touch_cnt, 1);

    /*Save the pressed coordinates and the state*/
    if (touchpad_is_pressed)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_x[0];
        data->point.y = touch_y[0];
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
