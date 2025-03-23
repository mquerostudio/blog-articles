/**
 * @file main.c
 * @brief Main file for the project
 * @author MQuero
 * @see mquero.com
 */

/**********************
 *      INCLUDES
 *********************/
#include "i2c_driver.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lvgl.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include "freertos/task.h"

#include "lv_demos.h"

/**********************
 *      VARIABLES
 **********************/
SemaphoreHandle_t xGuiSemaphore;
TaskHandle_t gui_task_handle;

/*********************
 * STATIC PROTOTYPES
 * *******************/
static uint32_t lv_tick_task(void);
static void task_gui(void *arg);

/*********************
 * MAIN FUNCTION
 * *******************/
void app_main(void)
{
    xGuiSemaphore = xSemaphoreCreateMutex();

    i2c_init();

    xTaskCreatePinnedToCore(task_gui, "gui", (1024 * 18), NULL,
                            5, &gui_task_handle, 1);

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static uint32_t lv_tick_task(void)
{
    return (((uint32_t)esp_timer_get_time()) / 1000);
}

static void task_gui(void *arg)
{
    lv_init();
    lv_tick_set_cb((lv_tick_get_cb_t)lv_tick_task);
    lv_port_disp_init();
    lv_port_indev_init();

    lv_demo_music();

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    uint32_t time_till_next = 0;
    while (1)
    {
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, 50 / portTICK_PERIOD_MS))
        { /*Take semaphore to update slider and arc values*/
            time_till_next = lv_timer_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
        vTaskDelay(time_till_next / portTICK_PERIOD_MS);
    }
}
