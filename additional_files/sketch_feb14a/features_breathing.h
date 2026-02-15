#ifndef FEATURE_BREATHING_H
#define FEATURE_BREATHING_H

#include "display.h"

void featureBreathingMode() {
  for (int i = 0; i < 200; i++) {
    float phase = sin(i * 0.05);
    displayBreathingFrame(phase);
    delay(50);
  }
}

#endif
