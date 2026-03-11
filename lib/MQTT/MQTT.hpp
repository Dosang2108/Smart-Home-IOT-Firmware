#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "global_var.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void init_Wifi_and_MQTT(void);
void registerChannels(void);
void mqttLoop(void);
void publishSensorData(void);
void publishFeedback(const char* message);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnect(void);

#ifdef __cplusplus
}
#endif

#endif