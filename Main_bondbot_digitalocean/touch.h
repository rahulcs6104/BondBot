// touch.h - Face Animation Functions for BondBot
// Contains all the detailed face expressions and drawing functions

#ifndef TOUCH_H
#define TOUCH_H

#include <Adafruit_SSD1306.h>

// External references to display and eye parameters
extern Adafruit_SSD1306 display;
extern int baseLeftEyeX;
extern int baseRightEyeX;
extern int eyeY;
extern int eyeWidth;
extern int eyeHeight;
extern int mouthY;

// Particle structure for confetti
struct Particle {
  int x, y;
  int vx, vy;
  bool active;
};

// ===== HELPER FUNCTIONS =====

void drawEyeAtPosition(int x, int y, int width, int height, int pupilOffsetX, int pupilOffsetY, bool isOpen) {
  if (!isOpen) {
    display.fillRect(x - width/2, y - 1, width, 3, SSD1306_WHITE);
    return;
  }
  
  display.fillRoundRect(x - width/2, y - height/2, width, height, 4, SSD1306_WHITE);
  
  int pupilWidth = width / 2;
  int pupilHeight = height - 4;
  
  int maxOffsetX = (width - pupilWidth) / 2 - 2;
  int maxOffsetY = (height - pupilHeight) / 2 - 2;
  
  int px = x + constrain(pupilOffsetX, -maxOffsetX, maxOffsetX);
  int py = y + constrain(pupilOffsetY, -maxOffsetY, maxOffsetY);
  
  display.fillRoundRect(px - pupilWidth/2, py - pupilHeight/2, pupilWidth, pupilHeight, 2, SSD1306_BLACK);
}

void drawSolidEye(int x, int y, int width, int height) {
  display.fillRoundRect(x - width/2, y - height/2, width, height, 4, SSD1306_WHITE);
}

void drawSolidCircleEye(int x, int y, int radius) {
  display.fillCircle(x, y, radius, SSD1306_WHITE);
}

void drawHeart(int x, int y, int size) {
  int r = size / 2;
  display.fillCircle(x - r/2, y - r/2, r, SSD1306_WHITE);
  display.fillCircle(x + r/2, y - r/2, r, SSD1306_WHITE);
  
  for (int dy = 0; dy < size; dy++) {
    int w = size - (dy * 2) / 3;
    display.drawLine(x - w/2, y + dy/2, x + w/2, y + dy/2, SSD1306_WHITE);
  }
  
  display.fillRect(x - size/2, y - r/2, size, size/2, SSD1306_WHITE);
}

void drawAngryEyebrows(int leftX, int rightX) {
  display.drawLine(leftX - 12, eyeY - 16, leftX + 8, eyeY - 13, SSD1306_WHITE);
  display.drawLine(leftX - 12, eyeY - 15, leftX + 8, eyeY - 12, SSD1306_WHITE);
  display.drawLine(rightX - 8, eyeY - 13, rightX + 12, eyeY - 16, SSD1306_WHITE);
  display.drawLine(rightX - 8, eyeY - 12, rightX + 12, eyeY - 15, SSD1306_WHITE);
}

// MOUTH FUNCTIONS

void drawSmileMouth() {
  for (int i = -12; i <= 12; i++) {
    int y = mouthY + (i * i) / 22;
    display.drawPixel(64 + i, y, SSD1306_WHITE);
    display.drawPixel(64 + i, y + 1, SSD1306_WHITE);
  }
}

void drawBigSmileMouth() {
  for (int i = -16; i <= 16; i++) {
    int y = mouthY + (i * i) / 30;
    display.drawPixel(64 + i, y, SSD1306_WHITE);
    display.drawPixel(64 + i, y + 1, SSD1306_WHITE);
    display.drawPixel(64 + i, y + 2, SSD1306_WHITE);
  }
}

void drawCircleMouth() {
  display.fillCircle(64, mouthY + 2, 6, SSD1306_WHITE);
  display.fillCircle(64, mouthY + 1, 4, SSD1306_BLACK);
}

void drawSmallCircleMouth() {
  display.fillCircle(64, mouthY, 3, SSD1306_WHITE);
  display.fillCircle(64, mouthY, 2, SSD1306_BLACK);
}

void drawGentleSmileMouth() {
  for (int i = -10; i <= 10; i++) {
    int y = mouthY + (i * i) / 35;
    display.drawPixel(64 + i, y, SSD1306_WHITE);
  }
}

void drawNeutralMouth() {
  display.fillRect(56, mouthY, 16, 2, SSD1306_WHITE);
}

void drawKissLips(int frame) {
  int pucker = (frame % 20 < 10) ? frame % 10 : 10 - (frame % 10);
  int lipWidth = 16 - pucker;
  int lipHeight = 6 + pucker;
  
  for (int i = -lipWidth/2; i <= lipWidth/2; i++) {
    int topCurve = abs(i) / 3;
    display.drawPixel(64 + i, mouthY - topCurve, SSD1306_WHITE);
    display.drawPixel(64 + i, mouthY - topCurve + 1, SSD1306_WHITE);
  }
  
  for (int i = -lipWidth/2; i <= lipWidth/2; i++) {
    int bottomCurve = abs(i) / 4;
    display.drawPixel(64 + i, mouthY + lipHeight/2 + bottomCurve, SSD1306_WHITE);
    display.drawPixel(64 + i, mouthY + lipHeight/2 + bottomCurve + 1, SSD1306_WHITE);
  }
}

// ===== EXPRESSION FUNCTIONS =====

void drawSleeping(int frame) {
  int breathOffset = (frame % 60 < 30) ? 0 : 1;
  int y = eyeY + breathOffset;
  
  for (int i = 0; i < eyeWidth; i++) {
    int droop = abs(i - eyeWidth/2) / 4;
    display.drawPixel(baseLeftEyeX - eyeWidth/2 + i, y + droop, SSD1306_WHITE);
    display.drawPixel(baseLeftEyeX - eyeWidth/2 + i, y + droop + 1, SSD1306_WHITE);
    display.drawPixel(baseLeftEyeX - eyeWidth/2 + i, y + droop + 2, SSD1306_WHITE);
    
    display.drawPixel(baseRightEyeX - eyeWidth/2 + i, y + droop, SSD1306_WHITE);
    display.drawPixel(baseRightEyeX - eyeWidth/2 + i, y + droop + 1, SSD1306_WHITE);
    display.drawPixel(baseRightEyeX - eyeWidth/2 + i, y + droop + 2, SSD1306_WHITE);
  }
  
  drawNeutralMouth();
  
  if (frame % 70 < 55) {
    int zOffset = (frame % 70) / 18;
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(100, 8 - zOffset * 5);
    display.print("z");
    if (frame % 70 > 18) {
      display.setCursor(107, 3 - zOffset * 5);
      display.print("Z");
    }
  }
}

void drawConfettiBlast(int frame, Particle particles[]) {
  for (int i = 0; i < 15; i++) {
    if (particles[i].active) {
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].vy += 1;
      
      if (particles[i].x >= 0 && particles[i].x < 128 && 
          particles[i].y >= 0 && particles[i].y < 64) {
        if (i % 3 == 0) {
          display.fillRect(particles[i].x, particles[i].y, 2, 2, SSD1306_WHITE);
        } else if (i % 3 == 1) {
          display.drawPixel(particles[i].x, particles[i].y, SSD1306_WHITE);
          display.drawPixel(particles[i].x + 1, particles[i].y, SSD1306_WHITE);
        } else {
          display.drawPixel(particles[i].x, particles[i].y, SSD1306_WHITE);
          display.drawPixel(particles[i].x, particles[i].y + 1, SSD1306_WHITE);
        }
      }
      
      if (particles[i].y > 64) particles[i].active = false;
    }
  }
  
  int growth = min(frame * 2, eyeHeight);
  if (growth < eyeHeight) {
    display.fillRoundRect(baseLeftEyeX - eyeWidth/2, eyeY - growth/2, eyeWidth, growth, 4, SSD1306_WHITE);
    display.fillRoundRect(baseRightEyeX - eyeWidth/2, eyeY - growth/2, eyeWidth, growth, 4, SSD1306_WHITE);
  } else {
    drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, true);
    drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, true);
  }
  
  drawCircleMouth();
}

void drawHappy(int frame) {
  int happyHeight = eyeHeight - 6;
  display.fillRoundRect(baseLeftEyeX - eyeWidth/2, eyeY - happyHeight/2, eyeWidth, happyHeight, 4, SSD1306_WHITE);
  display.fillRoundRect(baseRightEyeX - eyeWidth/2, eyeY - happyHeight/2, eyeWidth, happyHeight, 4, SSD1306_WHITE);
  
  int pupilH = happyHeight - 2;
  display.fillRoundRect(baseLeftEyeX - eyeWidth/4, eyeY - pupilH/2, eyeWidth/2, pupilH, 2, SSD1306_BLACK);
  display.fillRoundRect(baseRightEyeX - eyeWidth/4, eyeY - pupilH/2, eyeWidth/2, pupilH, 2, SSD1306_BLACK);
  
  drawSmileMouth();
}

void drawExcited(int frame) {
  int shake = (frame % 6 < 3) ? -1 : 1;
  int bigWidth = eyeWidth + 3;
  int bigHeight = eyeHeight + 3;
  
  drawEyeAtPosition(baseLeftEyeX + shake, eyeY, bigWidth, bigHeight, 0, -2, true);
  drawEyeAtPosition(baseRightEyeX + shake, eyeY, bigWidth, bigHeight, 0, -2, true);
  
  drawBigSmileMouth();
  
  if (frame % 8 < 4) {
    display.fillCircle(15, 15, 2, SSD1306_WHITE);
    display.fillCircle(113, 15, 2, SSD1306_WHITE);
  }
}

void drawStarryEyes(int frame) {
  drawSolidEye(baseLeftEyeX, eyeY, eyeWidth + 2, eyeHeight + 2);
  drawSolidEye(baseRightEyeX, eyeY, eyeWidth + 2, eyeHeight + 2);
  
  if (frame % 10 < 5) {
    display.fillCircle(20, 15, 2, SSD1306_WHITE);
    display.fillCircle(108, 15, 2, SSD1306_WHITE);
  }
  
  drawBigSmileMouth();
}

void drawHeartEyes(int frame) {
  drawHeart(baseLeftEyeX, eyeY, 14);
  drawHeart(baseRightEyeX, eyeY, 14);
  
  drawBigSmileMouth();
  
  if (frame % 10 < 5) {
    display.fillCircle(20, 12, 2, SSD1306_WHITE);
    display.fillCircle(108, 12, 2, SSD1306_WHITE);
  }
}

void drawWinkLeft(int frame) {
  bool winking = (frame % 25 < 8) || (frame % 25 > 17 && frame % 25 < 25);
  
  if (!winking) {
    drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, false);
  } else {
    drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, true);
  }
  
  drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 2, 0, true);
  
  drawGentleSmileMouth();
}

void drawWinkRight(int frame) {
  bool winking = (frame % 25 < 8) || (frame % 25 > 17 && frame % 25 < 25);
  
  drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, -2, 0, true);
  
  if (!winking) {
    drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, false);
  } else {
    drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, true);
  }
  
  drawGentleSmileMouth();
}

void drawKissing(int frame) {
  int happyHeight = eyeHeight - 8;
  display.fillRoundRect(baseLeftEyeX - eyeWidth/2, eyeY - happyHeight/2, eyeWidth, happyHeight, 4, SSD1306_WHITE);
  display.fillRoundRect(baseRightEyeX - eyeWidth/2, eyeY - happyHeight/2, eyeWidth, happyHeight, 4, SSD1306_WHITE);
  
  int pupilH = happyHeight - 2;
  display.fillRoundRect(baseLeftEyeX - eyeWidth/4, eyeY - pupilH/2, eyeWidth/2, pupilH, 2, SSD1306_BLACK);
  display.fillRoundRect(baseRightEyeX - eyeWidth/4, eyeY - pupilH/2, eyeWidth/2, pupilH, 2, SSD1306_BLACK);
  
  drawKissLips(frame);
  
  if (frame % 20 < 16) {
    int heartOffset = (frame % 20) * 3;
    drawHeart(50, mouthY - 5 - heartOffset, 4);
    
    if (frame % 20 > 8) {
      int heartOffset2 = ((frame % 20) - 8) * 3;
      drawHeart(78, mouthY - 5 - heartOffset2, 4);
    }
  }
}

void drawLoveStruck(int frame) {
  drawHeart(baseLeftEyeX, eyeY, 14);
  
  bool rightWinking = (frame % 30) > 20;
  drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, !rightWinking);
  
  drawBigSmileMouth();
  
  if (frame % 12 < 10) {
    int offset = (frame % 12) * 3;
    drawHeart(90, 15 - offset, 4);
  }
}

void drawCurious(int frame) {
  int cycle = frame % 35;
  int lookX = (cycle < 18) ? cycle - 9 : 9 - (cycle - 18);
  
  drawEyeAtPosition(baseLeftEyeX, eyeY - 1, eyeWidth, eyeHeight, lookX / 2, -2, true);
  drawEyeAtPosition(baseRightEyeX, eyeY - 1, eyeWidth, eyeHeight, lookX / 2, -2, true);
  
  drawGentleSmileMouth();
}

void drawSurprised(int frame) {
  int bigWidth = eyeWidth + 4;
  int bigHeight = eyeHeight + 5;
  
  drawEyeAtPosition(baseLeftEyeX, eyeY - 2, bigWidth, bigHeight, 0, -3, true);
  drawEyeAtPosition(baseRightEyeX, eyeY - 2, bigWidth, bigHeight, 0, -3, true);
  
  drawCircleMouth();
}

void drawScared(int frame) {
  int shiftProgress = min(frame / 3, 18);
  int centerX = 64 + shiftProgress;
  
  display.fillRect(centerX - 3, eyeY - 8, 3, 16, SSD1306_WHITE);
  display.fillRect(centerX + 3, eyeY - 8, 3, 16, SSD1306_WHITE);
  
  drawSmallCircleMouth();
  
  if (frame % 4 < 2) {
    display.drawPixel(10, 35, SSD1306_WHITE);
    display.drawPixel(118, 35, SSD1306_WHITE);
  }
}

void drawFearSmall(int frame) {
  int wobble = (frame % 6 < 3) ? -1 : 1;
  
  drawSolidCircleEye(baseLeftEyeX + wobble, eyeY, 5);
  drawSolidCircleEye(baseRightEyeX + wobble, eyeY, 5);
  
  drawSmallCircleMouth();
  
  if (frame % 4 < 2) {
    display.drawPixel(10, 35, SSD1306_WHITE);
    display.drawPixel(118, 35, SSD1306_WHITE);
  }
}

void drawAnxious(int frame) {
  int wobble = (frame % 8 < 4) ? -1 : 1;
  
  drawEyeAtPosition(baseLeftEyeX + wobble, eyeY, eyeWidth - 2, eyeHeight - 3, 0, 2, true);
  drawEyeAtPosition(baseRightEyeX + wobble, eyeY, eyeWidth - 2, eyeHeight - 3, 0, 2, true);
  
  drawGentleSmileMouth();
}

void drawAngry(int frame) {
  int shake = (frame % 6 < 3) ? -2 : 2;
  
  display.fillRoundRect(baseLeftEyeX - eyeWidth/2, eyeY - (eyeHeight/2) + 4, eyeWidth, eyeHeight - 8, 3, SSD1306_WHITE);
  display.fillRoundRect(baseRightEyeX - eyeWidth/2, eyeY - (eyeHeight/2) + 4, eyeWidth, eyeHeight - 8, 3, SSD1306_WHITE);
  
  int pupilH = eyeHeight - 10;
  display.fillRoundRect(baseLeftEyeX - eyeWidth/4, eyeY - pupilH/2, eyeWidth/2, pupilH, 2, SSD1306_BLACK);
  display.fillRoundRect(baseRightEyeX - eyeWidth/4, eyeY - pupilH/2, eyeWidth/2, pupilH, 2, SSD1306_BLACK);
  
  drawAngryEyebrows(baseLeftEyeX + shake, baseRightEyeX + shake);
  drawNeutralMouth();
}

void drawSilly(int frame) {
  int cycle = frame % 30;
  
  if (cycle < 15) {
    drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, 4, -3, true);
    drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth + 2, eyeHeight + 2, -4, 3, true);
  } else {
    drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth + 2, eyeHeight + 2, -4, 3, true);
    drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 4, -3, true);
  }
  
  drawBigSmileMouth();
}

void drawPlayful(int frame) {
  int cycle = frame % 25;
  bool leftClosed = cycle < 8 || (cycle > 16 && cycle < 24);
  
  drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, -2, 0, !leftClosed);
  drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 2, 0, leftClosed);
  
  drawSmileMouth();
}

void drawCuteIdle(int frame) {
  int blinkCycle = frame % 150;
  bool isBlinking = blinkCycle > 140;
  
  int lookCycle = frame % 80;
  int lookX = 0;
  if (lookCycle < 40) {
    lookX = -3 + (lookCycle * 6) / 40;
  } else {
    lookX = 3 - ((lookCycle - 40) * 6) / 40;
  }
  
  if (isBlinking) {
    drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, false);
    drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, false);
  } else {
    drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, lookX, 0, true);
    drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, lookX, 0, true);
  }
  
  drawGentleSmileMouth();
}

void drawGettingDrowsy(int frame) {
  int droopAmount = min(frame / 3, 12);
  int sleepyHeight = eyeHeight - droopAmount;
  
  bool slowBlink = (frame % 40) > 35;
  
  if (slowBlink) {
    drawEyeAtPosition(baseLeftEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, false);
    drawEyeAtPosition(baseRightEyeX, eyeY, eyeWidth, eyeHeight, 0, 0, false);
  } else {
    display.fillRoundRect(baseLeftEyeX - eyeWidth/2, eyeY - eyeHeight/2, eyeWidth, sleepyHeight, 4, SSD1306_WHITE);
    display.fillRoundRect(baseRightEyeX - eyeWidth/2, eyeY - eyeHeight/2, eyeWidth, sleepyHeight, 4, SSD1306_WHITE);
    
    if (sleepyHeight > 4) {
      int pupilH = sleepyHeight - 4;
      display.fillRoundRect(baseLeftEyeX - eyeWidth/4, eyeY - pupilH/2, eyeWidth/2, pupilH, 2, SSD1306_BLACK);
      display.fillRoundRect(baseRightEyeX - eyeWidth/4, eyeY - pupilH/2, eyeWidth/2, pupilH, 2, SSD1306_BLACK);
    }
  }
  
  drawNeutralMouth();
}

void drawBreathing(int frame, float breathPhase) {
  int r = 15 + (int)(10 * (0.5 + 0.5*sin(breathPhase)));
  display.drawCircle(64, 32, r, SSD1306_WHITE);
  display.drawCircle(64, 32, r-2, SSD1306_WHITE);
  display.setTextSize(1);
  if(breathPhase < PI) { 
    display.setCursor(40,55); 
    display.print("Breathe In"); 
  } else { 
    display.setCursor(38,55); 
    display.print("Breathe Out"); 
  }
}

#endif // TOUCH_H
