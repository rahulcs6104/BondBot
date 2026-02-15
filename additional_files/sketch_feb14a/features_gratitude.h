#ifndef FEATURE_GRATITUDE_H
#define FEATURE_GRATITUDE_H

#include "mqtt_handler.h"
#include "audio.h"

void featureGratitudeMessage() {
  recordAudio();
  sendMessage("AUDIO_MESSAGE");
}

#endif
