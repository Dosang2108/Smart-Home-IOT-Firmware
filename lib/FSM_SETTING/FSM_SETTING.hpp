#ifndef FSM_SETTING_HPP
#define FSM_SETTING_HPP

#include <Arduino.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include "global_var.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void initIR();
void handleIRRemote();
void processPasswordFSM(char key);

#ifdef __cplusplus
}
#endif

#endif