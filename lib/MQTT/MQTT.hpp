#ifndef MQTT_HPP
#define MQTT_HPP

#include <Arduino.h>
#include <iostream>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <global_var.hpp>


#ifdef __cplusplus
extern "C" {
#endif
//



void init_Wifi_and_MQTT(void);
void receiver_data(char* topic, byte* payload, unsigned int length);
void Transmit_receiver_data(void);
void reconnect(void);
///
#ifdef __cplusplus
}
#endif

#endif