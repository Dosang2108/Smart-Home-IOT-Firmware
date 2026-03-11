#include "control.hpp"

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

//control manual initialization
void pump_control_manual(int pumpspeed){
  pumpspeed = map(pumpspeed, 0, 100, 0, 255);
  analogWrite(pump, pumpspeed);
}

void fan_control_manual(bool active){
  active ? analogWrite(fan, 255) : analogWrite(fan, 0);
}

void led_control_manual(bool active){
  if (active){
    ledwhite_on();
  }
  else ledwhite_off();
  
}


//
void pump_on()
{
  analogWrite(pump, 255);
}

void pump_off()
{
  analogWrite(pump, 0);
} 

void controlFan()
{
  float temperature = Value_Temperature;
  int fanSpeed = temperature * 100 /255;
  analogWrite(fan, fanSpeed);
  // digitalWrite(fan, HIGH);

}

void ledwhite_on()
{   
  NeoPixel.setPixelColor(1, NeoPixel.Color(255, 255, 255));
  NeoPixel.setPixelColor(2, NeoPixel.Color(255, 255, 255)); 
  NeoPixel.setPixelColor(3, NeoPixel.Color(255, 255, 255));  
  NeoPixel.show();
}

void ledwhite_off()
{
  NeoPixel.setPixelColor(1, NeoPixel.Color(0, 0, 0));
  NeoPixel.setPixelColor(2, NeoPixel.Color(0, 0, 0)); 
  NeoPixel.setPixelColor(3, NeoPixel.Color(0, 0, 0));  
  NeoPixel.show();
}

void ledred_on()
{
  // NeoPixel.clear(); 
  NeoPixel.setPixelColor(0, NeoPixel.Color(255, 0, 0)); 
  NeoPixel.show();
}

void ledgreen_on()
{
  // NeoPixel.clear(); 
  NeoPixel.setPixelColor(0, NeoPixel.Color(0, 255, 0)); 
  NeoPixel.show();
}

void led_off()
{
  NeoPixel.setPixelColor(0, NeoPixel.Color(0, 0, 0));  
  NeoPixel.show();
}
