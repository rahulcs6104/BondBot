#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 20
#define SCL_PIN 21

#define TOUCH_PIN 1   // TTP223 digital output pin

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  delay(2000); // give USB time to connect
  Serial.println("Touch test starting");

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED NOT FOUND");
    while (true);
  }
  Serial.println("OLED FOUND");

  pinMode(TOUCH_PIN, INPUT);  // digital input from TTP223

  // Initial display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Touch sensor test");
  display.display();
}

void loop() {
  int touchState = digitalRead(TOUCH_PIN);  // read digital HIGH/LOW

  Serial.print("Touch state: ");
  Serial.println(touchState);

  // Only update the dynamic part
  display.fillRect(0, 15, 128, 50, SSD1306_BLACK);  // clear area for dynamic text

  display.setTextSize(1);
  display.setCursor(0, 15);
  display.print("State: ");
  display.println(touchState);

  if (touchState == LOW) {  // pressed
    display.setTextSize(2);
    display.setCursor(10, 35);
    display.println("TOUCH!");
  }

  display.display();
  delay(100);
}
