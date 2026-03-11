#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "global_var.hpp"
#include "DHT20.h"


#ifdef __cplusplus
extern "C"
{
#endif

extern LiquidCrystal_I2C lcd;
extern DHT20 DHT;
void lcdDisplay();

#ifdef __cplusplus
}
#endif
#endif