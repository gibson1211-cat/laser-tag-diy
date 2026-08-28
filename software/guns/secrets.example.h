#pragma once
// Copy to secrets.h in the same sketch folder (guns/ or turret/).
// secrets.h is gitignored. Do not commit it. Do not print these values.

// This week: house Wi-Fi (visible 2.4 GHz). Arena_Net (hidden VLAN) is later.
#define WIFI_SSID "ORBI75"
#define WIFI_PASSWORD "fill-me-in-locally"

// Home Assistant mosquitto (aihapi). Not 192.168.1.30.
#define MQTT_HOST "192.168.20.249"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD "fill-me-in-locally"

// ArduinoOTA on port 3232. Empty = no OTA password (LAN only).
#define OTA_PASSWORD ""
