#include "crowpanel.h"

// Define the global crowpanel_config variable
crowpanel_config_t crowpanel_config;
crowpanel_wifi_scan_t crowpanel_wifi_scan;

// Default values for Wi-Fi and configuration
void crowpanel_init(void) {
    // *** IMPORTANT: Replace with your actual WiFi name (SSID) and password ***
    strcpy(crowpanel_config.sta_ssid, "****");    // <-- CHANGE THIS to your WiFi name
    strcpy(crowpanel_config.sta_pwd, "****");    // <-- CHANGE THIS to your WiFi password
    strcpy(crowpanel_config.hostname, "crowpanel");
    
    // *** IMPORTANT: Replace with your actual printer IP address ***
    strcpy(crowpanel_config.moonraker_ip, "****");  // <-- CHANGE THIS to your printer's IP address
    strcpy(crowpanel_config.moonraker_port, "7125");         // <-- Verify this is your Moonraker port
    strcpy(crowpanel_config.moonraker_tool, "tool0");        // Default extruder name
    strcpy(crowpanel_config.mode, "sta");                    // WiFi in station mode (client)
    
    // Initialize Wi-Fi scan data
    crowpanel_wifi_scan.count = 0;
    for (int i = 0; i < SCAN_SSIDS_NUM; i++) {
        crowpanel_wifi_scan.rssi[i] = 0;
        memset(crowpanel_wifi_scan.ssid[i], 0, sizeof(crowpanel_wifi_scan.ssid[i]));
        crowpanel_wifi_scan.authmode[i] = WIFI_AUTH_OPEN;
        crowpanel_wifi_scan.connected[i] = 0;
    }
}