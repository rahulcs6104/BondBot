#ifndef FEATURE_PRESENCE_H
#define FEATURE_PRESENCE_H

#include "mqtt_handler.h"
#include "display.h"
#include "audio.h"

void featurePresencePing() {
  sendMessage("PING");
  displayFace(random(1,4));
  playSoftTone();
}

#endif
