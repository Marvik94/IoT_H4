#pragma once

// ========== WiFi ==========
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"

// ========== MQTT ==========
#define MQTT_SERVER "SERVER IP" 
#define MQTT_PORT 1884
#define MQTT_CLIENT_ID "esp32_sniffer"
#define MQTT_TOPIC "sniffer/data"

// ========== Sensor ==========
#define DHTPIN 4
#define DHTTYPE DHT11

// ========== LED Pins ==========
#define GREEN_LED 17
#define RED_LED 16

// ========== Sampling ==========
#define SAMPLE_INTERVAL_MS 10000
#define BUFFER_LIMIT 5
#define BT_SCAN_DURATION_SEC 1

// ========== Debug ==========
#define ENABLE_SERIAL true
