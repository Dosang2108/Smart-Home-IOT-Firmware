#ifndef SENSOR_HPP
#define SENSOR_HPP
#include <Arduino.h>
#include "sensors.hpp"
#include <Arduino.h>
#include "DHT20.h"
#include "global_var.hpp"


#ifdef __cplusplus
extern "C"
{
#endif

extern DHT20 DHT; 
extern int soilMoistureSensor;
extern int lightSensor;
extern int temperatureSensor;
extern int humiditySensor;

void readDHT20();
void readSoilMoisture();
void readLight();
void valueSensor();

#ifdef __cplusplus
}
#endif
#endif