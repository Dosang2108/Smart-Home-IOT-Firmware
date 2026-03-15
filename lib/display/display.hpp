#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include "global_var.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

extern LiquidCrystal_I2C lcd;

void initLCD();
void initNTP();
void scanI2CBus();
void lcdDisplaySensors();
void lcdDisplayRow1();
void lcdDisplayRow2();
void lcdUpdateColon();

#ifdef __cplusplus
}
#endif
#endif