#ifndef FEATURE_CHECKIN_H
#define FEATURE_CHECKIN_H

#include "mqtt_handler.h"

void featureCheckIn() {
  sendMessage("CHECKIN_REQUEST");
}

#endif
