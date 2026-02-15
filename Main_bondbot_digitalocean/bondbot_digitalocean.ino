/*
 * BondBot - DigitalOcean MQTT Version
 * Features:
 * 1. Single Tap  -> Ping partner (random faces)
 * 2. Double Tap  -> Breathing exercise
 * 3. Hold 1s     -> Activity tracking (shared hearts)
 * 4. Hold 3s     -> Mood Check-in (record audio -> Gemini -> mood face)
 * 5. Hold 5s     -> Gratitude message
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include "base64.h"

// Eye parameters - REQUIRED by touch.h (must be global!)
int baseLeftEyeX = 38;
int baseRightEyeX = 90;
int eyeY = 28;
int eyeWidth = 22;
int eyeHeight = 24;
int mouthY = 52;

#include "touch.h"

// Forward declarations
void playNote(int freq, int ms, float vol=0.5);
void playHappySound();
void playLoveSound();
void playKissSound();
void playCuriousSound();
void playExcitedSound();
void showStatus(const char* status);
void initConfetti();

// ========================================
// CONFIGURATION - EDIT WIFI ONLY!
// ========================================

// WiFi Settings
#define WIFI_SSID "WhiteSky-TheFlats"
#define WIFI_PASS "3g524bz6"

// MQTT Settings (YOUR DigitalOcean Droplet!)
#define MQTT_SERVER   "159.89.46.71"
#define MQTT_PORT     1883
#define MQTT_USER     "couplebot"
#define MQTT_PASSWD   "CoupleBot2026!"

// Backend HTTP endpoint for audio analysis
#define BACKEND_URL "http://159.89.46.71:3000/analyze-mood"

// Device Settings
#define PAIR_ID "pair01"                  // Same for both devices
#define DEVICE_ID "A"                     // Use "A" for first device, "B" for second

// ========================================
// HARDWARE CONFIGURATION
// ========================================

#define I2C_SDA 20
#define I2C_SCL 21
#define TOUCH_PIN 1

// I2S pins - speaker output
#define I2S_BCLK 6
#define I2S_LRC 5
#define I2S_DIN 7      // Speaker data out

// I2S pins - microphone input (INMP441 or similar)
// Connect your I2S mic: SCK->pin 2, WS->pin 3, SD->pin 4
#define I2S_MIC_SCK 2
#define I2S_MIC_WS  3
#define I2S_MIC_SD  4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define SAMPLE_RATE 16000
#define I2S_NUM_SPK I2S_NUM_0

// Recording settings
#define RECORD_DURATION_MS 1500
#define RECORD_SAMPLE_RATE 8000
#define RECORD_BUFFER_SIZE (RECORD_SAMPLE_RATE * RECORD_DURATION_MS / 1000)

// Gesture timings
#define TAP_MAX_DURATION 400
#define DOUBLE_TAP_WINDOW 500
#define HOLD_1S_THRESHOLD 1000
#define HOLD_3S_THRESHOLD 3000
#define HOLD_5S_THRESHOLD 5000
#define FRAME_DELAY 60

// ========================================
// GLOBAL OBJECTS
// ========================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ========================================
// STATE VARIABLES
// ========================================

enum GestureType { NONE, SINGLE_TAP, DOUBLE_TAP, TRIPLE_TAP, HOLD_1S, HOLD_3S, HOLD_5S };
enum Expression { SLEEPING, CONFETTI_BLAST, HAPPY, EXCITED, HEART_EYES, KISSING, 
                  NEUTRAL, CURIOUS, SURPRISED, BREATHING, ACTIVITY,
                  MOOD_HAPPY, MOOD_NEUTRAL, MOOD_SAD, RECORDING, ANALYZING };

Expression currentExpression = SLEEPING;
int animationFrame = 0;
unsigned long lastFrameTime = 0;

// Gesture detection
bool touching = false;
unsigned long touchStartTime = 0;
unsigned long lastTapTime = 0;
int tapCount = 0;
bool waitingForTap = false;
bool hold1sReturned = false;
bool hold3sReturned = false;

// Animation variables - Particle struct is defined in touch.h
Particle particles[15];
float breathPhase = 0;

// Activity tracking
struct DotPosition { int x, y; };
DotPosition activityDots[7];
bool myActivityDone[7] = {false, false, false, false, false, false, false};
bool partnerActivityDone[7] = {false, false, false, false, false, false, false};
int currentDay = 0;

// Mood check-in state
bool waitingForMoodResult = false;
String checkinRequester = "";

// MQTT topics
String topicPub;
String topicSub;

bool audioEnabled = false;

// ========================================
// SETUP
// ========================================

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n=================================");
  Serial.println("   BondBot - DigitalOcean MQTT");
  Serial.println("   With Mood Check-in!");
  Serial.println("=================================");
  
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Display failed!");
    for(;;);
  }
  
  showBootScreen();
  
  pinMode(TOUCH_PIN, INPUT);
  setupI2S_Speaker();
  initActivityDots();
  setupTopics();
  connectWiFi();
  
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);
  connectMQTT();
  
  Serial.println("\nBondBot Ready!");
  Serial.println("Gestures:");
  Serial.println("  Single Tap  -> Ping partner!");
  Serial.println("  Double Tap  -> Breathing mode");
  Serial.println("  Hold 1s     -> Activity tracking");
  Serial.println("  Hold 3s     -> Mood check-in");
  Serial.println("  Hold 5s     -> Gratitude message");
  
  currentExpression = SLEEPING;
  animationFrame = 0;
}

// ========================================
// MAIN LOOP
// ========================================

void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();
  
  GestureType gesture = detectGesture();
  if (gesture != NONE) handleGesture(gesture);
  
  if (millis() - lastFrameTime >= FRAME_DELAY) {
    lastFrameTime = millis();
    updateAnimation();
  }
  
  delay(10);
}

// ========================================
// MQTT FUNCTIONS
// ========================================

void setupTopics() {
  if (String(DEVICE_ID) == "A") {
    topicPub = "bondbot/" + String(PAIR_ID) + "/A_to_B";
    topicSub = "bondbot/" + String(PAIR_ID) + "/B_to_A";
  } else {
    topicPub = "bondbot/" + String(PAIR_ID) + "/B_to_A";
    topicSub = "bondbot/" + String(PAIR_ID) + "/A_to_B";
  }
  Serial.print("Publishing to: "); Serial.println(topicPub);
  Serial.print("Subscribed to: "); Serial.println(topicSub);
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: "); Serial.println(WIFI_SSID);
  showStatus("Connecting WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) { delay(500); Serial.print("."); attempts++; }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!"); Serial.print("   IP: "); Serial.println(WiFi.localIP());
    showStatus("WiFi OK!"); delay(1000);
  } else { Serial.println("\nWiFi failed!"); showStatus("WiFi FAILED!"); delay(3000); }
}

void connectMQTT() {
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    showStatus("Connecting MQTT...");
    String clientId = "BondBot-" + String(DEVICE_ID) + "-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWD)) {
      Serial.println("MQTT Connected!");
      mqtt.subscribe(topicSub.c_str());
      showStatus("MQTT OK!"); delay(1000); return;
    } else {
      Serial.print("MQTT Failed, rc="); Serial.println(mqtt.state());
      attempts++; if (attempts < 5) delay(3000);
    }
  }
  if (!mqtt.connected()) { showStatus("MQTT Failed!"); delay(2000); }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  Serial.print("MQTT: "); Serial.println(message);
  
  // === PRESENCE PING ===
  if (message.indexOf("PRESENCE_PING") >= 0) {
    animationFrame = 0;
    int r = random(0, 5);
    switch(r) {
      case 0: initConfetti(); currentExpression = HAPPY; playHappySound(); break;
      case 1: currentExpression = EXCITED; playExcitedSound(); break;
      case 2: currentExpression = HEART_EYES; playLoveSound(); break;
      case 3: currentExpression = KISSING; playKissSound(); break;
      case 4: currentExpression = SURPRISED; playExcitedSound(); break;
    }
  }
  // === ACTIVITY UPDATE ===
  else if (message.indexOf("ACTIVITY_UPDATE") >= 0) {
    int dayIdx = message.indexOf("\"day\":");
    if (dayIdx >= 0) {
      int day = message.substring(dayIdx + 6).toInt();
      if (day >= 0 && day < 7) partnerActivityDone[day] = true;
    }
    currentExpression = ACTIVITY; animationFrame = 0; playHappySound();
  }
  // === CHECKIN REQUEST (partner wants to check our mood) ===
  else if (message.indexOf("CHECKIN_REQUEST") >= 0) {
    Serial.println("   -> Partner wants to check our mood!");
    
    // Extract who requested it
    int fromIdx = message.indexOf("\"from\":\"");
    String requester = "A";
    if (fromIdx >= 0) requester = message.substring(fromIdx + 8, fromIdx + 9);
    
    // Show recording face, then record and send
    currentExpression = RECORDING;
    animationFrame = 0;
    
    Serial.println("   -> Recording 2s audio...");
    recordAndAnalyzeMood(requester);
  }
  // === MOOD RESULT (backend analyzed the audio) ===
  else if (message.indexOf("MOOD_RESULT") >= 0) {
    Serial.println("   -> Got partner's mood result!");
    
    String mood = "neutral";
    if (message.indexOf("\"mood\":\"happy\"") >= 0) mood = "happy";
    else if (message.indexOf("\"mood\":\"sad\"") >= 0) mood = "sad";
    
    Serial.print("   -> Partner's mood: "); Serial.println(mood);
    
    animationFrame = 0;
    if (mood == "happy") {
      currentExpression = MOOD_HAPPY;
      playHappySound();
    } else if (mood == "sad") {
      currentExpression = MOOD_SAD;
      playNote(330, 200, 0.4);
      playNote(262, 300, 0.3);
    } else {
      currentExpression = MOOD_NEUTRAL;
      playCuriousSound();
    }
    waitingForMoodResult = false;
  }
  // === AUDIO MESSAGE (gratitude) ===
  else if (message.indexOf("AUDIO_MESSAGE") >= 0) {
    currentExpression = HEART_EYES; animationFrame = 0; playLoveSound();
  }
}

// ========================================
// AUDIO RECORDING & MOOD ANALYSIS
// ========================================

void recordAndAnalyzeMood(String requester) {
  showStatus("Recording...");
  display.display();
  
  // Allocate recording buffer (2 seconds at 16kHz = 32000 samples)
  int16_t* audioBuffer = (int16_t*)malloc(RECORD_BUFFER_SIZE * sizeof(int16_t));
  if (!audioBuffer) {
    Serial.println("   Failed to allocate audio buffer!");
    showStatus("Memory error!");
    delay(1000);
    currentExpression = SLEEPING; return;
  }
  
  // ESP32-C3 only has ONE I2S port (I2S_NUM_0)
  // So we must uninstall speaker, install mic, record, then reinstall speaker
  i2s_driver_uninstall(I2S_NUM_SPK);
  
  // Configure I2S for microphone input
  i2s_config_t rxCfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = RECORD_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4, .dma_buf_len = 512,
    .use_apll = false, .tx_desc_auto_clear = true, .fixed_mclk = 0
  };
  
  i2s_pin_config_t rxPins = {
    .bck_io_num = I2S_MIC_SCK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD
  };
  
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &rxCfg, 0, NULL);
  if (err != ESP_OK) {
    Serial.print("   I2S mic failed: "); Serial.println(err);
    memset(audioBuffer, 0, RECORD_BUFFER_SIZE * sizeof(int16_t));
  } else {
    i2s_set_pin(I2S_NUM_0, &rxPins);
    i2s_zero_dma_buffer(I2S_NUM_0);
    delay(100); // Let mic stabilize
    
    // Record audio
    size_t bytesRead = 0;
    size_t totalRead = 0;
    size_t targetBytes = RECORD_BUFFER_SIZE * sizeof(int16_t);
    
    Serial.println("   Recording 2 seconds...");
    unsigned long recordStart = millis();
    
    while (totalRead < targetBytes && (millis() - recordStart) < (RECORD_DURATION_MS + 500)) {
      i2s_read(I2S_NUM_0, ((uint8_t*)audioBuffer) + totalRead,
               min((size_t)1024, targetBytes - totalRead), &bytesRead, 100);
      totalRead += bytesRead;
      
      // Update recording animation on screen
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(22, 8);
      display.print("RECORDING...");
      
      int pulse = (millis() / 200) % 4;
      display.fillCircle(64, 35, 8 + pulse * 2, SSD1306_WHITE);
      display.fillCircle(64, 35, 6 + pulse * 2, SSD1306_BLACK);
      display.fillCircle(64, 35, 4, SSD1306_WHITE);
      
      int progress = map(totalRead, 0, targetBytes, 0, 100);
      display.drawRect(14, 53, 100, 6, SSD1306_WHITE);
      display.fillRect(14, 53, progress, 6, SSD1306_WHITE);
      display.display();
    }
    
    Serial.print("   Recorded "); Serial.print(totalRead); Serial.println(" bytes");
    
    // Uninstall mic driver
    i2s_driver_uninstall(I2S_NUM_0);
  }
  
  // Reinstall speaker driver
  setupI2S_Speaker();
  
  // Show analyzing screen
  showStatus("Analyzing mood...");
  display.display();
  
  // Create WAV in the SAME buffer to save memory
  // We need 44 extra bytes for the header, so shift audio data
  int audioDataSize = RECORD_BUFFER_SIZE * sizeof(int16_t);
  int wavSize = 44 + audioDataSize;
  
  // Allocate WAV buffer (header + audio)
  uint8_t* wavBuffer = (uint8_t*)malloc(wavSize);
  
  if (!wavBuffer) {
    Serial.println("   WAV alloc failed, trying smaller...");
    // Try with half the data if full fails
    audioDataSize = audioDataSize / 2;
    wavSize = 44 + audioDataSize;
    wavBuffer = (uint8_t*)malloc(wavSize);
    if (!wavBuffer) {
      Serial.println("   Still failed! Skipping...");
      free(audioBuffer);
      showStatus("Memory error!");
      delay(1000);
      currentExpression = SLEEPING; return;
    }
  }
  
  // Write WAV header
  writeWavHeader(wavBuffer, audioDataSize, RECORD_SAMPLE_RATE, 1, 16);
  memcpy(wavBuffer + 44, audioBuffer, audioDataSize);
  
  // Free audio buffer BEFORE base64 encoding to free up RAM
  free(audioBuffer);
  audioBuffer = NULL;
  
  // Base64 encode
  String base64Audio = base64::encode(wavBuffer, wavSize);
  
  // Free WAV buffer BEFORE HTTP send
  free(wavBuffer);
  wavBuffer = NULL;
  
  Serial.print("   Base64 size: "); Serial.println(base64Audio.length());
  
  // Send to backend via HTTP
  sendAudioToBackend(base64Audio, String(DEVICE_ID), String(PAIR_ID), requester);
}

void writeWavHeader(uint8_t* h, int dataSize, int sr, int ch, int bps) {
  int byteRate = sr * ch * bps / 8;
  int blockAlign = ch * bps / 8;
  int fileSize = 36 + dataSize;
  
  h[0]='R'; h[1]='I'; h[2]='F'; h[3]='F';
  h[4]=fileSize&0xFF; h[5]=(fileSize>>8)&0xFF; h[6]=(fileSize>>16)&0xFF; h[7]=(fileSize>>24)&0xFF;
  h[8]='W'; h[9]='A'; h[10]='V'; h[11]='E';
  h[12]='f'; h[13]='m'; h[14]='t'; h[15]=' ';
  h[16]=16; h[17]=0; h[18]=0; h[19]=0;
  h[20]=1; h[21]=0;
  h[22]=ch; h[23]=0;
  h[24]=sr&0xFF; h[25]=(sr>>8)&0xFF; h[26]=(sr>>16)&0xFF; h[27]=(sr>>24)&0xFF;
  h[28]=byteRate&0xFF; h[29]=(byteRate>>8)&0xFF; h[30]=(byteRate>>16)&0xFF; h[31]=(byteRate>>24)&0xFF;
  h[32]=blockAlign; h[33]=0;
  h[34]=bps; h[35]=0;
  h[36]='d'; h[37]='a'; h[38]='t'; h[39]='a';
  h[40]=dataSize&0xFF; h[41]=(dataSize>>8)&0xFF; h[42]=(dataSize>>16)&0xFF; h[43]=(dataSize>>24)&0xFF;
}

void sendAudioToBackend(String base64Audio, String fromDevice, String pairId, String requester) {
  Serial.println("   Sending audio to backend...");
  showStatus("Sending to server...");
  display.display();
  
  HTTPClient http;
  http.begin(BACKEND_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000);
  
  String payload = "{\"audio\":\"" + base64Audio + 
                   "\",\"from\":\"" + fromDevice + 
                   "\",\"pair_id\":\"" + pairId + 
                   "\",\"requester\":\"" + requester + "\"}";
  
  Serial.print("   Payload size: "); Serial.println(payload.length());
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("   Response ("); Serial.print(httpCode); Serial.print("): "); Serial.println(response);
    if (httpCode == 200) showStatus("Mood sent!");
    else showStatus("Server error!");
  } else {
    Serial.print("   HTTP error: "); Serial.println(http.errorToString(httpCode));
    showStatus("Send failed!");
  }
  
  http.end();
  delay(1000);
  currentExpression = SLEEPING;
  animationFrame = 0;
}

// ========================================
// GESTURE DETECTION
// ========================================

GestureType detectGesture() {
  bool currentTouch = digitalRead(TOUCH_PIN);
  unsigned long now = millis();
  
  if (currentTouch && !touching) {
    touching = true; touchStartTime = now;
    hold1sReturned = false; hold3sReturned = false;
    return NONE;
  }
  
  if (currentTouch && touching) {
    if (now - touchStartTime >= HOLD_5S_THRESHOLD) {
      touching = false; return HOLD_5S;
    }
  }
  
  if (!currentTouch && touching) {
    touching = false;
    unsigned long duration = now - touchStartTime;
    if (duration >= HOLD_3S_THRESHOLD) return HOLD_3S;
    else if (duration >= HOLD_1S_THRESHOLD) return HOLD_1S;
    else if (duration < TAP_MAX_DURATION) {
      if (!waitingForTap) { waitingForTap = true; lastTapTime = now; tapCount = 1; }
      else if (now - lastTapTime < DOUBLE_TAP_WINDOW) { tapCount++; lastTapTime = now; }
      else { tapCount = 1; lastTapTime = now; }
      return NONE;
    }
  }
  
  if (waitingForTap && (now - lastTapTime >= DOUBLE_TAP_WINDOW)) {
    waitingForTap = false;
    if (tapCount == 1) { tapCount = 0; return SINGLE_TAP; }
    else if (tapCount == 2) { tapCount = 0; return DOUBLE_TAP; }
    else if (tapCount >= 3) { tapCount = 0; return TRIPLE_TAP; }
  }
  
  return NONE;
}

// ========================================
// GESTURE HANDLERS
// ========================================

void handleGesture(GestureType gesture) {
  String msg;
  
  switch(gesture) {
    case SINGLE_TAP:
      Serial.println("Single Tap - Ping!");
      { int r = random(0,4);
        switch(r) {
          case 0: initConfetti(); currentExpression = CONFETTI_BLAST; playExcitedSound(); break;
          case 1: currentExpression = HAPPY; playHappySound(); break;
          case 2: currentExpression = HEART_EYES; playLoveSound(); break;
          case 3: currentExpression = KISSING; playKissSound(); break;
        }
      }
      animationFrame = 0;
      msg = "{\"type\":\"PRESENCE_PING\",\"from\":\"" + String(DEVICE_ID) + "\",\"pair_id\":\"" + String(PAIR_ID) + "\"}";
      mqtt.publish(topicPub.c_str(), msg.c_str());
      break;
      
    case DOUBLE_TAP:
      currentExpression = BREATHING; animationFrame = 0; break;
      
    case TRIPLE_TAP:
      currentExpression = HEART_EYES; animationFrame = 0; playLoveSound(); break;
      
    case HOLD_1S:
      myActivityDone[currentDay] = true;
      currentExpression = ACTIVITY; animationFrame = 0; playHappySound();
      msg = "{\"type\":\"ACTIVITY_UPDATE\",\"from\":\"" + String(DEVICE_ID) + "\",\"pair_id\":\"" + String(PAIR_ID) + "\",\"day\":" + String(currentDay) + "}";
      mqtt.publish(topicPub.c_str(), msg.c_str());
      currentDay = (currentDay + 1) % 7;
      break;
      
    case HOLD_3S:
      Serial.println("Hold 3s - Mood Check-in!");
      currentExpression = CURIOUS; animationFrame = 0; playCuriousSound();
      waitingForMoodResult = true;
      msg = "{\"type\":\"CHECKIN_REQUEST\",\"from\":\"" + String(DEVICE_ID) + "\",\"pair_id\":\"" + String(PAIR_ID) + "\"}";
      mqtt.publish(topicPub.c_str(), msg.c_str());
      Serial.println("   -> Sent check-in request, waiting for partner...");
      showStatus("Checking partner...");
      break;
      
    case HOLD_5S:
      msg = "{\"type\":\"AUDIO_MESSAGE\",\"from\":\"" + String(DEVICE_ID) + "\",\"pair_id\":\"" + String(PAIR_ID) + "\"}";
      mqtt.publish(topicPub.c_str(), msg.c_str());
      currentExpression = KISSING; animationFrame = 0; playKissSound();
      break;
      
    default: break;
  }
}

// ========================================
// DISPLAY FUNCTIONS
// ========================================

void showBootScreen() {
  display.clearDisplay();
  display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 15); display.println("BondBot");
  display.setTextSize(1); display.setCursor(20, 40);
  display.print("Device "); display.println(DEVICE_ID);
  display.display(); delay(2000);
}

void showStatus(const char* status) {
  display.clearDisplay();
  display.setTextSize(1); display.setCursor(10, 25); display.println(status);
  display.display();
}

void updateAnimation() {
  display.clearDisplay();
  switch(currentExpression) {
    case SLEEPING:       drawSleeping(animationFrame); break;
    case CONFETTI_BLAST: drawConfettiBlast(animationFrame, particles); break;
    case HAPPY:          drawHappy(animationFrame); break;
    case EXCITED:        drawExcited(animationFrame); break;
    case HEART_EYES:     drawHeartEyes(animationFrame); break;
    case KISSING:        drawKissing(animationFrame); break;
    case NEUTRAL:        drawNeutralFace(animationFrame); break;
    case CURIOUS:        drawCurious(animationFrame); break;
    case SURPRISED:      drawSurprised(animationFrame); break;
    case BREATHING:      drawBreathingAnim(animationFrame); break;
    case ACTIVITY:       drawActivityScreen(); break;
    case MOOD_HAPPY:     drawMoodHappy(animationFrame); break;
    case MOOD_NEUTRAL:   drawMoodNeutral(animationFrame); break;
    case MOOD_SAD:       drawMoodSad(animationFrame); break;
    case RECORDING:      drawRecordingScreen(animationFrame); break;
    case ANALYZING:      drawAnalyzingScreen(animationFrame); break;
  }
  display.display();
  animationFrame++;
}

// ========================================
// MOOD FACE DRAWINGS
// ========================================

void drawMoodHappy(int f) {
  int bounce = (f % 20 < 10) ? 0 : -1;
  drawEyeAtPosition(baseLeftEyeX, eyeY + bounce, eyeWidth, eyeHeight, 0, -2, true);
  drawEyeAtPosition(baseRightEyeX, eyeY + bounce, eyeWidth, eyeHeight, 0, -2, true);
  
  // Big smile
  for (int i = -16; i <= 16; i++) {
    int y = mouthY + (i * i) / 30;
    display.drawPixel(64 + i, y, SSD1306_WHITE);
    display.drawPixel(64 + i, y + 1, SSD1306_WHITE);
  }
  
  // Rosy cheeks
  display.fillCircle(20, 42, 3, SSD1306_WHITE);
  display.fillCircle(108, 42, 3, SSD1306_WHITE);
  
  // Sparkles
  if (f % 12 < 6) {
    display.drawPixel(15, 12, SSD1306_WHITE); display.drawPixel(113, 12, SSD1306_WHITE);
    display.drawPixel(10, 18, SSD1306_WHITE); display.drawPixel(118, 18, SSD1306_WHITE);
  }
  
  display.setTextSize(1);
  display.setCursor(35, 0); display.print("PARTNER:");
  display.setCursor(44, 57); display.print("HAPPY!");
}

void drawMoodNeutral(int f) {
  int lookCycle = f % 40;
  int lookX = (lookCycle < 20) ? (lookCycle - 10) / 5 : (10 - (lookCycle - 20)) / 5;
  drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, lookX, 0, true);
  drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, lookX, 0, true);
  display.fillRect(52, mouthY, 24, 2, SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(35, 0); display.print("PARTNER:");
  display.setCursor(38, 57); display.print("NEUTRAL");
}

void drawMoodSad(int f) {
  int droop = 2;
  int sadH = eyeHeight - 4;
  display.fillRoundRect(baseLeftEyeX - eyeWidth/2, eyeY - sadH/2 + droop, eyeWidth, sadH, 4, SSD1306_WHITE);
  display.fillRoundRect(baseRightEyeX - eyeWidth/2, eyeY - sadH/2 + droop, eyeWidth, sadH, 4, SSD1306_WHITE);
  int pH = sadH - 4;
  display.fillRoundRect(baseLeftEyeX - eyeWidth/4, eyeY - pH/2 + droop + 2, eyeWidth/2, pH, 2, SSD1306_BLACK);
  display.fillRoundRect(baseRightEyeX - eyeWidth/4, eyeY - pH/2 + droop + 2, eyeWidth/2, pH, 2, SSD1306_BLACK);
  
  // Frown
  for (int i = -12; i <= 12; i++) {
    int y = mouthY + 4 - (i * i) / 22;
    display.drawPixel(64 + i, y, SSD1306_WHITE);
    display.drawPixel(64 + i, y + 1, SSD1306_WHITE);
  }
  
  // Tear
  if (f % 30 < 20) {
    int tearY = eyeY + sadH/2 + droop + (f % 30);
    if (tearY < mouthY - 5) display.fillCircle(baseLeftEyeX + eyeWidth/2 - 2, tearY, 1, SSD1306_WHITE);
  }
  
  display.setTextSize(1);
  display.setCursor(35, 0); display.print("PARTNER:");
  display.setCursor(50, 57); display.print("SAD");
}

void drawRecordingScreen(int f) {
  display.setTextSize(1); display.setCursor(22, 8); display.print("RECORDING...");
  int pulse = (f % 15);
  int r = 6 + (pulse < 8 ? pulse : 15 - pulse);
  display.fillCircle(64, 35, r, SSD1306_WHITE);
  display.fillCircle(64, 35, r - 2, SSD1306_BLACK);
  display.fillCircle(64, 35, 3, SSD1306_WHITE);
  if (f % 8 < 4) display.drawCircle(64, 35, r + 4, SSD1306_WHITE);
  display.setCursor(30, 55); display.print("Speak now...");
}

void drawAnalyzingScreen(int f) {
  display.setTextSize(1); display.setCursor(25, 10); display.print("ANALYZING...");
  for (int i = 0; i < 3; i++) {
    float angle = (f * 0.15) + (i * TWO_PI / 3);
    int dx = 64 + (int)(15 * cos(angle));
    int dy = 38 + (int)(10 * sin(angle));
    display.fillCircle(dx, dy, 2, SSD1306_WHITE);
  }
  display.setCursor(22, 55); display.print("Please wait...");
}

// ========================================
// OTHER DRAWING FUNCTIONS
// ========================================

void drawNeutralFace(int f) {
  drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, true);
  drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, true);
  drawNeutralMouth();
}

void drawBreathingAnim(int f) {
  breathPhase += 0.05;
  if (breathPhase > TWO_PI) breathPhase -= TWO_PI;
  drawBreathing(f, breathPhase);
}

void drawHappyWithConfetti(int f) {
  for (int i = 0; i < 15; i++) {
    if (particles[i].active) {
      particles[i].x += particles[i].vx; particles[i].y += particles[i].vy; particles[i].vy += 1;
      if (particles[i].x>=0 && particles[i].x<128 && particles[i].y>=0 && particles[i].y<64)
        display.fillRect(particles[i].x, particles[i].y, 2, 2, SSD1306_WHITE);
      if (particles[i].y > 64) particles[i].active = false;
    }
  }
  drawHappy(f);
}

void drawActivityScreen() {
  display.setTextSize(1); display.setCursor(35, 5); display.println("ACTIVITY");
  const char* dayLabels[] = {"M","T","W","T","F","S","S"};
  for (int i = 0; i < 7; i++) {
    int x = activityDots[i].x, y = activityDots[i].y;
    bool showLeft, showRight;
    if (String(DEVICE_ID) == "A") { showLeft = myActivityDone[i]; showRight = partnerActivityDone[i]; }
    else { showLeft = partnerActivityDone[i]; showRight = myActivityDone[i]; }
    if (showLeft && showRight) { drawLeftHalfHeart(x, y); drawRightHalfHeart(x, y); }
    else if (showLeft) drawLeftHalfHeart(x, y);
    else if (showRight) drawRightHalfHeart(x, y);
    else display.drawCircle(x, y, 4, SSD1306_WHITE);
    display.setTextSize(1); display.setCursor(x-2, y+8); display.print(dayLabels[i]);
  }
  int cx = activityDots[currentDay].x, cy = activityDots[currentDay].y;
  display.drawPixel(cx, cy-8, SSD1306_WHITE);
  display.drawPixel(cx-1, cy-9, SSD1306_WHITE);
  display.drawPixel(cx+1, cy-9, SSD1306_WHITE);
}

void drawLeftHalfHeart(int cx, int cy) {
  for(int y=-4;y<=4;y++) display.drawPixel(cx,cy+y,SSD1306_WHITE);
  for(int y=-4;y<=3;y++) display.drawPixel(cx-1,cy+y,SSD1306_WHITE);
  for(int y=-3;y<=2;y++) display.drawPixel(cx-2,cy+y,SSD1306_WHITE);
  for(int y=-3;y<=1;y++) display.drawPixel(cx-3,cy+y,SSD1306_WHITE);
  display.drawPixel(cx-4,cy-1,SSD1306_WHITE); display.drawPixel(cx-4,cy,SSD1306_WHITE);
}

void drawRightHalfHeart(int cx, int cy) {
  for(int y=-4;y<=4;y++) display.drawPixel(cx,cy+y,SSD1306_WHITE);
  for(int y=-4;y<=3;y++) display.drawPixel(cx+1,cy+y,SSD1306_WHITE);
  for(int y=-3;y<=2;y++) display.drawPixel(cx+2,cy+y,SSD1306_WHITE);
  for(int y=-3;y<=1;y++) display.drawPixel(cx+3,cy+y,SSD1306_WHITE);
  display.drawPixel(cx+4,cy-1,SSD1306_WHITE); display.drawPixel(cx+4,cy,SSD1306_WHITE);
}

void initConfetti() {
  for(int i=0;i<15;i++) {
    particles[i].x=64; particles[i].y=32;
    particles[i].vx=random(-6,7); particles[i].vy=random(-8,-2);
    particles[i].active=true;
  }
}

void initActivityDots() {
  for(int i=0;i<4;i++) { activityDots[i].x=16+i*28; activityDots[i].y=25; }
  for(int i=4;i<7;i++) { activityDots[i].x=30+(i-4)*28; activityDots[i].y=48; }
}

// ========================================
// AUDIO FUNCTIONS
// ========================================

void setupI2S_Speaker() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4, .dma_buf_len = 512,
    .use_apll = false, .tx_desc_auto_clear = true, .fixed_mclk = 0
  };
  i2s_pin_config_t pins = { .bck_io_num=I2S_BCLK, .ws_io_num=I2S_LRC,
    .data_out_num=I2S_DIN, .data_in_num=I2S_PIN_NO_CHANGE };
  i2s_driver_install(I2S_NUM_SPK, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_SPK, &pins);
  i2s_zero_dma_buffer(I2S_NUM_SPK);
  audioEnabled = true;
  Serial.println("Speaker initialized");
}

void playNote(int freq, int ms, float vol) {
  if(!audioEnabled) return;
  int n = (SAMPLE_RATE*ms)/1000;
  int16_t *buf = (int16_t*)malloc(n*sizeof(int16_t));
  if(!buf) return;
  for(int i=0;i<n;i++) buf[i]=(int16_t)(sin(2.0*PI*freq*i/SAMPLE_RATE)*32767*vol);
  size_t w; i2s_write(I2S_NUM_SPK,buf,n*sizeof(int16_t),&w,portMAX_DELAY); free(buf);
}

void playHappySound() { playNote(659,80,0.5); playNote(784,80,0.5); }
void playLoveSound() { playNote(523,90,0.5); playNote(659,90,0.5); playNote(784,100,0.5); }
void playKissSound() { playNote(880,50,0.5); playNote(659,50,0.5); }
void playCuriousSound() { playNote(659,60,0.5); playNote(784,60,0.5); }
void playExcitedSound() { playNote(784,60,0.55); playNote(988,60,0.55); playNote(1175,70,0.55); }
