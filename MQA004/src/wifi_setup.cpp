#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "crowpanel.h"

// Global variables
static bool connect_busy = false;
wifi_status_t wifi_status = WIFI_STATUS_DISCONNECTED;

// Connect to WiFi
void wifi_connect(void) {
  if (connect_busy) return;
  connect_busy = true;
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(crowpanel_config.hostname);
  WiFi.begin(crowpanel_config.sta_ssid, crowpanel_config.sta_pwd);
  
  connect_busy = false;
}

// WiFi connection status
wifi_status_t wifi_get_connect_status(void) {
  return wifi_status;
}

// WiFi task
void wifi_task(void* parameter) {
  unsigned long lastTime = millis();
  const unsigned long delayTime = 1000; // Check every second
  
  // Allow time for WiFi connection
  delay(3000);
  
  // Loop forever
  for(;;) {
    if (millis() - lastTime > delayTime) {
      lastTime = millis();
      
      // Check WiFi status
      switch (WiFi.status()) {
        case WL_CONNECTED:
          wifi_status = WIFI_STATUS_CONNECTED;
          break;
        case WL_DISCONNECTED:
        case WL_CONNECTION_LOST:
        case WL_NO_SSID_AVAIL:
          if (wifi_status == WIFI_STATUS_CONNECTED) {
            // Try to reconnect
            WiFi.disconnect();
            delay(500);
            WiFi.begin(crowpanel_config.sta_ssid, crowpanel_config.sta_pwd);
            wifi_status = WIFI_STATUS_CONNECTING;
          } else if (wifi_status == WIFI_STATUS_DISCONNECTED) {
            // Try to connect if we're supposed to
            if (strlen(crowpanel_config.sta_ssid) > 0) {
              WiFi.begin(crowpanel_config.sta_ssid, crowpanel_config.sta_pwd);
              wifi_status = WIFI_STATUS_CONNECTING;
            }
          }
          break;
        case WL_CONNECT_FAILED:
          wifi_status = WIFI_STATUS_ERROR;
          delay(5000);
          wifi_status = WIFI_STATUS_DISCONNECTED;
          break;
        default:
          if (wifi_status != WIFI_STATUS_CONNECTING) {
            wifi_status = WIFI_STATUS_DISCONNECTED;
          }
          break;
      }
    }
    
    delay(100);
  }
}
