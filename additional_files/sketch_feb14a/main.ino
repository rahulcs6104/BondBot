// #include <WiFi.h>
// #include <PubSubClient.h>
// #include <Wire.h>
// #include <Adafruit_SSD1306.h>

// // ===== USER CONFIG =====
// #define WIFI_SSID "YOUR_WIFI"
// #define WIFI_PASS "YOUR_PASS"

// #define MQTT_SERVER "YOUR_DIGITALOCEAN_IP"
// #define MQTT_PORT 1883
// #define PAIR_ID "pair01"
// #define DEVICE_ID "A"   // or "B"

// // ===== OLED =====
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// // ===== TOUCH =====
// #define TOUCH_PIN 2

// // ===== GLOBALS =====
// WiFiClient espClient;
// PubSubClient mqtt(espClient);

// unsigned long touchStart = 0;
// bool touching = false;

// // ===== FUNCTION DECLARATIONS =====
// void connectWiFi();
// void connectMQTT();
// void mqttCallback(char* topic, byte* payload, unsigned int length);

// void detectGesture();
// void handleSingleTap();
// void handleDoubleTap();
// void handleHold3s();
// void handleHold5s();
// void handleTapHold();

// void featurePresencePing();
// void featureBreathingMode();
// void featureCheckIn();
// void featureGratitudeMessage();
// void featureHabitUpdate();

// void displayFace(int id);
// void displayBreathingFrame(float phase);
// void playSoftTone();
// void recordAudio();
// void playAudio();


// // ================= SETUP =================
// void setup() {
//   Serial.begin(115200);
//   pinMode(TOUCH_PIN, INPUT);

//   Wire.begin();
//   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
//   display.clearDisplay();
//   display.display();

//   connectWiFi();

//   mqtt.setServer(MQTT_SERVER, MQTT_PORT);
//   mqtt.setCallback(mqttCallback);
// }

// // ================= LOOP =================
// void loop() {
//   if (!mqtt.connected()) connectMQTT();
//   mqtt.loop();

//   detectGesture();
// }


// // ================= WIFI =================
// void connectWiFi() {
//   WiFi.begin(WIFI_SSID, WIFI_PASS);
//   while (WiFi.status() != WL_CONNECTED) delay(500);
// }

// // ================= MQTT =================
// void connectMQTT() {
//   while (!mqtt.connected()) {
//     if (mqtt.connect(DEVICE_ID)) {
//       String topic = "couple/" PAIR_ID "/" DEVICE_ID "_to_" "other";
//       mqtt.subscribe(topic.c_str());
//     } else {
//       delay(1000);
//     }
//   }
// }

// void mqttCallback(char* topic, byte* payload, unsigned int length) {
//   String msg;
//   for (int i = 0; i < length; i++) msg += (char)payload[i];

//   if (msg.indexOf("PING") >= 0) {
//     displayFace(random(1,4));
//     playSoftTone();
//   }

//   if (msg.indexOf("AUDIO_MESSAGE") >= 0) {
//     playAudio();
//   }

//   if (msg.indexOf("EMOTION_RESULT") >= 0) {
//     displayFace(3);
//   }
// }


// // ================= GESTURE ENGINE =================
// void detectGesture() {
//   bool state = digitalRead(TOUCH_PIN);

//   if (state && !touching) {
//     touching = true;
//     touchStart = millis();
//   }

//   if (!state && touching) {
//     touching = false;
//     unsigned long duration = millis() - touchStart;

//     if (duration < 400) handleSingleTap();
//     else if (duration >= 3000 && duration < 5000) handleHold3s();
//     else if (duration >= 5000) handleHold5s();
//   }
// }


// // ================= FEATURE HANDLERS =================
// void handleSingleTap() {
//   featurePresencePing();
// }

// void handleDoubleTap() {
//   featureBreathingMode();
// }

// void handleHold3s() {
//   featureCheckIn();
// }

// void handleHold5s() {
//   featureGratitudeMessage();
// }

// void handleTapHold() {
//   featureHabitUpdate();
// }


// // ================= FEATURE IMPLEMENTATIONS =================

// // ❤️ Presence Ping
// void featurePresencePing() {
//   String msg = "{\"type\":\"PING\"}";
//   mqtt.publish("couple/" PAIR_ID "/" DEVICE_ID "_to_other", msg.c_str());
//   displayFace(random(1,4));
// }

// // 🌬️ Breathing Mode
// void featureBreathingMode() {
//   for (int i=0;i<200;i++) {
//     float phase = sin(i * 0.05);
//     displayBreathingFrame(phase);
//     delay(50);
//   }
// }

// // 🧠 Mental Health Check-in
// void featureCheckIn() {
//   String msg = "{\"type\":\"CHECKIN_REQUEST\"}";
//   mqtt.publish("couple/" PAIR_ID "/" DEVICE_ID "_to_other", msg.c_str());
// }

// // 💌 Gratitude Voice Message
// void featureGratitudeMessage() {
//   recordAudio();
//   String msg = "{\"type\":\"AUDIO_MESSAGE\"}";
//   mqtt.publish("couple/" PAIR_ID "/" DEVICE_ID "_to_other", msg.c_str());
// }

// // 💑 Habit Tracking
// void featureHabitUpdate() {
//   String msg = "{\"type\":\"HABIT_UPDATE\"}";
//   mqtt.publish("couple/" PAIR_ID "/" DEVICE_ID "_to_other", msg.c_str());
// }
