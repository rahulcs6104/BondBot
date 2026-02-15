#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

void displayFace(int id) {
  display.clearDisplay();

  if (id == 1) {
    display.setCursor(40, 20);
    display.print(":)");
  }
  else if (id == 2) {
    display.setCursor(40, 20);
    display.print(":|");
  }
  else {
    display.setCursor(40, 20);
    display.print(":(");
  }

  display.display();
}

void displayBreathingFrame(float phase) {
  display.clearDisplay();
  int radius = 20 + (int)(10 * phase);
  display.drawCircle(64, 32, radius, SSD1306_WHITE);
  display.display();
}

#endif
