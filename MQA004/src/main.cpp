#define LGFX_USE_V1
#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include "CST816D.h"
#include "moonraker.h"
#include "crowpanel.h"

// I/O expander definitions
#define PI4IO_I2C_ADDR 0x43
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define TP_INT 0  // Touch panel interrupt pin
#define TP_RST -1 // Touch panel reset pin

// Display size
#define screenWidth 240
#define screenHeight 240

// Screen buffer size
#define buf_size 120

// Display driver
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void)
  {
    { // Set up bus control
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 80000000;
      cfg.freq_read = 20000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 6;
      cfg.pin_mosi = 7;
      cfg.pin_miso = -1;
      cfg.pin_dc = 2;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    { // Set up display panel control
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 10;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.memory_width = screenWidth;
      cfg.memory_height = screenHeight;
      cfg.panel_width = screenWidth;
      cfg.panel_height = screenHeight;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

LGFX tft;

// Touch screen
CST816D touch(I2C_SDA_PIN, I2C_SCL_PIN, TP_RST, TP_INT);

// Display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[2][screenWidth * buf_size];

// UI elements
lv_obj_t *printer_status_label;
lv_obj_t *nozzle_temp_label;
lv_obj_t *bed_temp_label;

// I/O expander functions
void init_IO_extender()
{
  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x01); // test register
  Wire.endTransmission();
  Wire.requestFrom(PI4IO_I2C_ADDR, 1);
  uint8_t rxdata = Wire.read();

  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x03);                                                 // IO direction register
  Wire.write((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)); // set pins 0, 1, 2, 3, 4 as outputs
  Wire.endTransmission();

  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x07);                                                    // Output Hi-Z register
  Wire.write(~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4))); // set pins 0, 1, 2, 3, 4 low
  Wire.endTransmission();
}

void set_pin_io(uint8_t pin_number, bool value)
{
  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x05); // read output register
  Wire.endTransmission();
  Wire.requestFrom(PI4IO_I2C_ADDR, 1);
  uint8_t rxdata = Wire.read();

  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x05); // Output register

  if (!value)
    Wire.write((~(1 << pin_number)) & rxdata); // set pin low
  else
    Wire.write((1 << pin_number) | rxdata); // set pin high
  Wire.endTransmission();
}

// LVGL display flush callback
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  if (tft.getStartCount() == 0)
  {
    tft.endWrite();
  }

  tft.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t *)&color_p->full);

  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

// LVGL touchpad read callback
static void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  bool touched;
  uint8_t gesture;
  uint16_t touchX, touchY;

  touched = touch.getTouch(&touchX, &touchY, &gesture);

  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;
    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;
  }
}

// Function prototypes
void create_ui(void);
void update_ui(void);

// Timer for UI updates
lv_timer_t *ui_timer;

void update_ui_cb(lv_timer_t *timer)
{
  update_ui();
}

void update_ui()
{
  static unsigned long lastStatusCheck = 0;
  static int lastStatus = -1;

  // Check WiFi and Moonraker status periodically (every 5 seconds)
  if (millis() - lastStatusCheck > 5000)
  {
    wifi_status_t status = wifi_get_connect_status();
    bool ready = !moonraker.unready;
    lastStatus = (int)status;
    lastStatusCheck = millis();
  }

  if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED && !moonraker.unready)
  {
    // Update printer status text
    const char *status_text = "IDLE";

    if (moonraker.data.printing)
    {
      status_text = "PRINTING";
    }
    else if (moonraker.data.homing)
    {
      status_text = "HOMING";
    }
    else if (moonraker.data.probing)
    {
      status_text = "PROBING";
    }
    else if (moonraker.data.qgling)
    {
      status_text = "QGL";
    }
    else if (moonraker.data.heating_nozzle)
    {
      status_text = "HEATING NOZZLE";
    }
    else if (moonraker.data.heating_bed)
    {
      status_text = "HEATING BED";
    }

    // Update status label with format "STATE"
    char status_buf[30];
    snprintf(status_buf, sizeof(status_buf), "%s", status_text);
    lv_label_set_text(printer_status_label, status_buf);

    // Update temperature displays
    char nozzle_text[20];
    char bed_text[20];

    // Format temperatures as shown in the image: "150 °C" and "40 °C"
    snprintf(nozzle_text, sizeof(nozzle_text), "%d °C", moonraker.data.nozzle_actual);
    snprintf(bed_text, sizeof(bed_text), "%d °C", moonraker.data.bed_actual);

    // Update temperature labels directly
    lv_label_set_text(nozzle_temp_label, nozzle_text);
    lv_label_set_text(bed_temp_label, bed_text);
  }
  else
  {
    // Not connected
    if (wifi_get_connect_status() != WIFI_STATUS_CONNECTED)
    {
      lv_label_set_text(printer_status_label, "Connecting...");
    }
    else
    {
      lv_label_set_text(printer_status_label, "Disconnected");
    }
    // Set default temperatures when disconnected
    lv_label_set_text(nozzle_temp_label, "0 °C");
    lv_label_set_text(bed_temp_label, "0 °C");
  }
}

// Button event handlers
static void home_btn_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED && !moonraker.unready)
    {
      moonraker.post_gcode_to_queue("G28"); // Home all axes
      lv_label_set_text(printer_status_label, "Homing...");
    }
  }
}

static void qgl_btn_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED && !moonraker.unready)
    {
      moonraker.post_gcode_to_queue("QUAD_GANTRY_LEVEL"); // Run QGL
      lv_label_set_text(printer_status_label, "QGL Running...");
    }
  }
}

static void pla_btn_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED && !moonraker.unready)
    {
      moonraker.post_gcode_to_queue("M104 S220 T0"); // Set nozzle to 220°C for PLA
      moonraker.post_gcode_to_queue("M140 S40");     // Set bed to 40°C for PLA
      lv_label_set_text(printer_status_label, "Heating for PLA...");
    }
  }
}

static void abs_btn_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED && !moonraker.unready)
    {
      moonraker.post_gcode_to_queue("M104 S250 T0"); // Set nozzle to 250°C for ABS
      moonraker.post_gcode_to_queue("M140 S100");    // Set bed to 100°C for ABS
      lv_label_set_text(printer_status_label, "Heating for ABS...");
    }
  }
}

void create_ui()
{
  // Set up a display with black background
  lv_obj_t *main_screen = lv_scr_act();
  lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);

  // Create temperature display at the top
  lv_obj_t *temp_container = lv_obj_create(main_screen);
  lv_obj_remove_style_all(temp_container);
  lv_obj_set_size(temp_container, 240, 90);
  lv_obj_set_pos(temp_container, 0, 0);
  lv_obj_set_style_bg_color(temp_container, lv_color_black(), 0);
  lv_obj_set_style_border_width(temp_container, 0, 0);

  // Nozzle temperature section
  lv_obj_t *nozzle_title = lv_label_create(temp_container);
  lv_label_set_text(nozzle_title, "Nozzle");
  lv_obj_set_style_text_color(nozzle_title, lv_color_white(), 0);
  lv_obj_set_style_text_font(nozzle_title, &lv_font_montserrat_16, 0);
  lv_obj_align(nozzle_title, LV_ALIGN_TOP_LEFT, 40, 45);

  nozzle_temp_label = lv_label_create(temp_container);
  lv_label_set_text(nozzle_temp_label, "150 °C");
  lv_obj_set_style_text_color(nozzle_temp_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(nozzle_temp_label, &lv_font_montserrat_20, 0);
  lv_obj_align(nozzle_temp_label, LV_ALIGN_TOP_LEFT, 40, 70);

  // Bed temperature section
  lv_obj_t *bed_title = lv_label_create(temp_container);
  lv_label_set_text(bed_title, "Bed");
  lv_obj_set_style_text_color(bed_title, lv_color_white(), 0);
  lv_obj_set_style_text_font(bed_title, &lv_font_montserrat_16, 0);
  lv_obj_align(bed_title, LV_ALIGN_TOP_RIGHT, -60, 45);

  bed_temp_label = lv_label_create(temp_container);
  lv_label_set_text(bed_temp_label, "40 °C");
  lv_obj_set_style_text_color(bed_temp_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(bed_temp_label, &lv_font_montserrat_20, 0);
  lv_obj_align(bed_temp_label, LV_ALIGN_TOP_RIGHT, -40, 70);

  // First row of buttons - Material presets
  lv_obj_t *pla_btn = lv_btn_create(main_screen);
  lv_obj_set_size(pla_btn, 90, 36);
  lv_obj_set_pos(pla_btn, 25, 100);
  lv_obj_set_style_radius(pla_btn, 20, 0);
  lv_obj_set_style_bg_color(pla_btn, lv_color_make(200, 200, 200), 0);                // Light gray
  lv_obj_set_style_bg_color(pla_btn, lv_color_make(150, 150, 150), LV_STATE_PRESSED); // Darker when pressed
  lv_obj_add_event_cb(pla_btn, pla_btn_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *pla_label = lv_label_create(pla_btn);
  lv_label_set_text(pla_label, "PLA");
  lv_obj_set_style_text_color(pla_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(pla_label, &lv_font_montserrat_20, 0);
  lv_obj_center(pla_label);

  lv_obj_t *abs_btn = lv_btn_create(main_screen);
  lv_obj_set_size(abs_btn, 90, 36);
  lv_obj_set_pos(abs_btn, 125, 100);
  lv_obj_set_style_radius(abs_btn, 20, 0);
  lv_obj_set_style_bg_color(abs_btn, lv_color_make(200, 200, 200), 0);                // Light gray
  lv_obj_set_style_bg_color(abs_btn, lv_color_make(150, 150, 150), LV_STATE_PRESSED); // Darker when pressed
  lv_obj_add_event_cb(abs_btn, abs_btn_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *abs_label = lv_label_create(abs_btn);
  lv_label_set_text(abs_label, "ABS");
  lv_obj_set_style_text_color(abs_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(abs_label, &lv_font_montserrat_20, 0);
  lv_obj_center(abs_label);

  // Second row of buttons - Control operations
  lv_obj_t *home_btn = lv_btn_create(main_screen);
  lv_obj_set_size(home_btn, 90, 36);
  lv_obj_set_pos(home_btn, 25, 150);
  lv_obj_set_style_radius(home_btn, 20, 0);
  lv_obj_set_style_bg_color(home_btn, lv_color_make(200, 200, 200), 0);                // Light gray
  lv_obj_set_style_bg_color(home_btn, lv_color_make(150, 150, 150), LV_STATE_PRESSED); // Darker when pressed
  lv_obj_add_event_cb(home_btn, home_btn_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *home_label = lv_label_create(home_btn);
  lv_label_set_text(home_label, "HOME");
  lv_obj_set_style_text_color(home_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(home_label, &lv_font_montserrat_20, 0);
  lv_obj_center(home_label);

  lv_obj_t *qgl_btn = lv_btn_create(main_screen);
  lv_obj_set_size(qgl_btn, 90, 36);
  lv_obj_set_pos(qgl_btn, 125, 150);
  lv_obj_set_style_radius(qgl_btn, 20, 0);
  lv_obj_set_style_bg_color(qgl_btn, lv_color_make(200, 200, 200), 0);                // Light gray
  lv_obj_set_style_bg_color(qgl_btn, lv_color_make(150, 150, 150), LV_STATE_PRESSED); // Darker when pressed
  lv_obj_add_event_cb(qgl_btn, qgl_btn_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *qgl_label = lv_label_create(qgl_btn);
  lv_label_set_text(qgl_label, "QGL");
  lv_obj_set_style_text_color(qgl_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(qgl_label, &lv_font_montserrat_20, 0);
  lv_obj_center(qgl_label);

  // Status label at the bottom
  lv_obj_t *status_label = lv_label_create(main_screen);
  lv_label_set_text(status_label, "IDLE");
  lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_16, 0);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);

  // Store for updating
  printer_status_label = status_label;
}

void setup()
{
  // Initialize crowpanel settings
  crowpanel_init();

  // Initialize I2C and I/O expander
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  init_IO_extender();
  delay(100);

  // Reset touch controller and display, turn on backlight
  set_pin_io(3, true);  // Reset Touch
  set_pin_io(4, true);  // Reset Display
  set_pin_io(2, true);  // Turn on Display Backlight
  set_pin_io(0, false); // Set Motor off (if applicable)
  delay(200);

  // Initialize display
  tft.init();
  tft.initDMA();
  tft.startWrite();
  tft.setColor(0, 0, 0);
  tft.fillScreen(TFT_BLACK);
  delay(100);

  // Initialize touch
  touch.begin();
  delay(100);

  // Initialize LVGL
  lv_init();

  // Setup display buffer
  lv_disp_draw_buf_init(&draw_buf, buf[0], buf[1], screenWidth * buf_size);

  // Setup display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Setup input device
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // Initialize WiFi
  wifi_connect();

  // Initialize Moonraker
  moonraker_setup();

  // Create the UI
  create_ui();

  // Create a timer for UI updates
  ui_timer = lv_timer_create(update_ui_cb, 1000, NULL);

  // Create WiFi task
  xTaskCreatePinnedToCore(
      wifi_task,   // Function to implement the task
      "WiFi Task", // Name of the task
      4096,        // Stack size in words
      NULL,        // Task input parameter
      1,           // Priority of the task
      NULL,        // Task handle
      0            // Core where the task should run
  );

  // Create Moonraker task
  xTaskCreatePinnedToCore(
      moonraker_task,   // Function to implement the task
      "Moonraker Task", // Name of the task
      8192,             // Stack size in words
      NULL,             // Task input parameter
      1,                // Priority of the task
      NULL,             // Task handle
      1                 // Core where the task should run
  );
}

void loop()
{
  // Call LVGL task handler
  lv_timer_handler();
  delay(5);
}