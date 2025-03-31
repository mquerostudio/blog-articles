/**
 * @file lv_port_disp.c
 * @brief Display interface configuration file
 * @author MQuero
 * @see mquero.com
 */

/**********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "lvgl.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"

/**********************
 *      DEFINES
 *********************/
#define CONFIG_LCD_HOST SPI3_HOST

#define BOARD_LCD_MOSI 39
#define BOARD_LCD_SCK 38
#define BOARD_LCD_CS 40
#define BOARD_LCD_RST 41
#define BOARD_LCD_DC 42

#define CONFIG_LCD_H_RES 240
#define CONFIG_LCD_V_RES 320
#define CONFIG_LCD_FREQ (80 * 1000 * 1000)
#define CONFIF_LCD_CMD_BITS 8
#define CONFIG_LCD_PARAM_BITS 8

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */

/**********************
 *      VARIABLES
 **********************/
esp_lcd_panel_handle_t panel_handle = NULL;

/**********************
 * STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

/**********************
 * GLOBAL FUNCTIONS
 **********************/
void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-------------------------
     * Create a display and set a flush_cb
     * -----------------------*/
    lv_display_t *disp = lv_display_create(CONFIG_LCD_H_RES, CONFIG_LCD_V_RES);
    lv_display_set_flush_cb(disp, disp_flush);

    /* Allocate buffers in SPIRAM */
    LV_ATTRIBUTE_MEM_ALIGN
    uint8_t *buf1 = (uint8_t *)heap_caps_malloc(CONFIG_LCD_H_RES * CONFIG_LCD_V_RES * BYTE_PER_PIXEL, MALLOC_CAP_SPIRAM);
    LV_ATTRIBUTE_MEM_ALIGN
    uint8_t *buf2 = (uint8_t *)heap_caps_malloc(CONFIG_LCD_H_RES * CONFIG_LCD_V_RES * BYTE_PER_PIXEL, MALLOC_CAP_SPIRAM);

    /* Set the display buffers for LVGL */
    lv_display_set_buffers(disp, buf1, buf2, (CONFIG_LCD_H_RES * CONFIG_LCD_V_RES * BYTE_PER_PIXEL), LV_DISPLAY_RENDER_MODE_FULL);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = BOARD_LCD_SCK,
        .mosi_io_num = BOARD_LCD_MOSI,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t)CONFIG_LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = BOARD_LCD_DC,
        .cs_gpio_num = BOARD_LCD_CS,
        .pclk_hz = CONFIG_LCD_FREQ,
        .lcd_cmd_bits = CONFIF_LCD_CMD_BITS,
        .lcd_param_bits = CONFIG_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)CONFIG_LCD_HOST, &io_cfg, &io_handle));

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BOARD_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
}

/*
 *Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map
 *'lv_display_flush_ready()' has to be called when it's finished.
 */
static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{

    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    // Draw the bitmap on the specified area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, (void *)px_map);

    // Inform LVGL that the flushing is done
    lv_display_flush_ready(disp_drv);
}
