#ifndef CONFIG_H
#define CONFIG_H

// ===== WIFI =====
#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASS "YOUR_PASS"

// ===== MQTT =====
#define MQTT_SERVER "YOUR_DIGITALOCEAN_IP"
#define MQTT_PORT 1883
#define PAIR_ID "pair01"
#define DEVICE_ID "A"   // change per device

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// ===== PINS =====
#define TOUCH_PIN 2

// ===== TOPICS =====
#define TOPIC_SEND "couple/" PAIR_ID "/" DEVICE_ID "_to_other"
#define TOPIC_RECV "couple/" PAIR_ID "/other_to_" DEVICE_ID

#endif
