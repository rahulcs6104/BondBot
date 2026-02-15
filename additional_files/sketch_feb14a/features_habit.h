#ifndef FEATURE_HABIT_H
#define FEATURE_HABIT_H

#include "mqtt_handler.h"

void featureHabitUpdate() {
  sendMessage("HABIT_UPDATE");
}

#endif
