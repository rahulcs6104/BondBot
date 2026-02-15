#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <PubSubClient.h>
#include <Arduino.h>

inline void sendMessage(PubSubClient &mqtt, const String &topic, const char* type) {
  String msg = "{\"type\":\"";
  msg += type;
  msg += "\",\"timestamp\":";
  msg += String(millis());
  msg += "}";
  mqtt.publish(topic.c_str(), msg.c_str());
}

#endif
