#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// ===== USER CONFIG =====
#define WIFI_SSID   "YOUR_WIFI"
#define WIFI_PASS   "YOUR_PASS"

// Use domain if possible (recommended): mqtt.yourdomain.com
#define MQTT_SERVER "YOUR_DIGITALOCEAN_IP_OR_DOMAIN"
#define MQTT_PORT   1883   // use 8883 for TLS later

#define PAIR_ID   "pair01"
#define DEVICE_ID "A"      // "A" or "B"

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== TOUCH =====
#define TOUCH_PIN 2

// ===== GLOBALS =====
WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long touchStart = 0;
bool touching = false;

// ----- Topics (matches your architecture doc) -----
String topicSub;   // what THIS device listens to
String topicPub;   // what THIS device sends to partner

// ===== helpers =====
void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);

void detectGesture();
void featurePresencePing();

void displayFace(int id);
void playSoftTone();

void setupTopics() {
  // If I'm A: publish A_to_B, subscribe B_to_A
  // If I'm B: publish B_to_A, subscribe A_to_B
  if (String(DEVICE_ID) == "A") {
    topicPub = "couple/" + String(PAIR_ID) + "/A_to_B";
    topicSub = "couple/" + String(PAIR_ID) + "/B_to_A";
  } else {
    topicPub = "couple/" + String(PAIR_ID) + "/B_to_A";
    topicSub = "couple/" + String(PAIR_ID) + "/A_to_B";
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("CoupleBot Boot...");
  display.display();

  setupTopics();
  connectWiFi();

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
}

// ================= LOOP =================
void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  detectGesture();
}

// ================= WIFI =================
void connectWiFi() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi connecting...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  display.println("WiFi OK");
  display.display();
}

// ================= MQTT =================
void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.println("Connecting MQTT...");

    // Use unique clientId so A and B don't kick each other off
    String clientId = String("CoupleBot-") + DEVICE_ID + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    if (mqtt.connect(clientId.c_str())) {
      Serial.println("MQTT connected");

      mqtt.subscribe(topicSub.c_str());
      Serial.print("Subscribed: ");
      Serial.println(topicSub);

      display.clearDisplay();
      display.setCursor(0,0);
      display.println("MQTT OK");
      display.println("Sub:");
      display.println(topicSub);
      display.display();

    } else {
      Serial.print("MQTT fail rc=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("IN ["); Serial.print(topic); Serial.print("] ");
  Serial.println(msg);

  // Super simple parsing by checking "type"
  if (msg.indexOf("\"type\":\"PING\"") >= 0) {
    displayFace(random(1,4));
    playSoftTone();
  }

  if (msg.indexOf("\"type\":\"AUDIO_MESSAGE\"") >= 0) {
    // later: actually receive audio bytes or fetch from API
    displayFace(2);
  }

  if (msg.indexOf("\"type\":\"EMOTION_RESULT\"") >= 0) {
    displayFace(3);
  }
}

// ================= GESTURE ENGINE =================
void detectGesture() {
  bool state = digitalRead(TOUCH_PIN);

  if (state && !touching) {
    touching = true;
    touchStart = millis();
  }

  if (!state && touching) {
    touching = false;
    unsigned long duration = millis() - touchStart;

    if (duration < 400) {
      featurePresencePing();
    }
    // You can add double-tap and other gestures after this is stable
  }
}

// ================= FEATURE =================
void featurePresencePing() {
  String msg = "{\"type\":\"PING\",\"timestamp\":";
  msg += String(millis());
  msg += ",\"payload\":{\"from\":\"";
  msg += DEVICE_ID;
  msg += "\"}}";

  mqtt.publish(topicPub.c_str(), msg.c_str());

  Serial.print("OUT -> ");
  Serial.print(topicPub);
  Serial.print(" ");
  Serial.println(msg);

  displayFace(random(1,4));
}

// ================= UI PLACEHOLDERS =================
void displayFace(int id) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Face: ");
  display.println(id);
  display.println("From partner!");
  display.display();
}

void playSoftTone() {
  // placeholder (you can hook to MAX98357A later)
  Serial.println("tone()");
}
