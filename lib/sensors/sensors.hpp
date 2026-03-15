#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <Arduino.h>
#include "DHT20.h"
#include "global_var.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

extern DHT20 DHT;

bool initDHT20();
void readDHT20();
void readSoilMoisture();
void readLight();
void readPIR();
void valueSensor();

#ifdef __cplusplus
}
#endif
#endif