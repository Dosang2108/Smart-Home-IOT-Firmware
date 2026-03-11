#include "control.hpp"

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);
static Servo doorServo;

// ============ Pump ============
void pump_on()
{
  analogWrite(pump, 255);
}

void pump_off()
{
  analogWrite(pump, 0);
}

void pump_control_manual(int pumpspeed)
{
  pumpspeed = map(pumpspeed, 0, 100, 0, 255);
  analogWrite(pump, pumpspeed);
}

// ============ Fan ============
void fan_set_speed(int speed)
{
  int pwm = map(constrain(speed, 0, 100), 0, 100, 0, 255);
  analogWrite(FAN_PIN, pwm);
}

void fan_control_manual(bool active)
{
  analogWrite(FAN_PIN, active ? 255 : 0);
}

// ============ LED RGB ============
void led_rgb_set(uint8_t r, uint8_t g, uint8_t b)
{
  NeoPixel.setPixelColor(0, NeoPixel.Color(r, g, b));
  NeoPixel.show();
}

void led_rgb_off()
{
  NeoPixel.setPixelColor(0, NeoPixel.Color(0, 0, 0));
  NeoPixel.show();
}

void ledwhite_on()
{
  for (int i = 1; i < NUM_PIXELS; i++)
    NeoPixel.setPixelColor(i, NeoPixel.Color(255, 255, 255));
  NeoPixel.show();
}

void ledwhite_off()
{
  for (int i = 1; i < NUM_PIXELS; i++)
    NeoPixel.setPixelColor(i, NeoPixel.Color(0, 0, 0));
  NeoPixel.show();
}

void ledred_on()
{
  NeoPixel.setPixelColor(0, NeoPixel.Color(255, 0, 0));
  NeoPixel.show();
}

void ledgreen_on()
{
  NeoPixel.setPixelColor(0, NeoPixel.Color(0, 255, 0));
  NeoPixel.show();
}

void led_off()
{
  NeoPixel.setPixelColor(0, NeoPixel.Color(0, 0, 0));
  NeoPixel.show();
}

// ============ PIR Auto Light ============
void handlePIRControl()
{
  if (pirDetected) {
    ledwhite_on();
  } else {
    ledwhite_off();
  }
}

// ============ Servo ============
void initServo()
{
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);
  delay(500);
  doorServo.detach();
}

void servo_open()
{
  doorServo.attach(SERVO_PIN);
  doorServo.write(90);
}

void servo_close()
{
  doorServo.write(0);
}

void servo_detach_motor()
{
  doorServo.detach();
}

void handleDoorServo()
{
  switch (doorState) {
    case DOOR_CLOSED:
      break;
    case DOOR_OPENING:
      if (millis_present - doorOpenTime >= DOOR_OPEN_DURATION) {
        servo_close();
        doorState = DOOR_CLOSING;
        doorOpenTime = millis_present;
      }
      break;
    case DOOR_CLOSING:
      if (millis_present - doorOpenTime >= DOOR_OPEN_DURATION) {
        servo_detach_motor();
        doorState = DOOR_CLOSED;
      }
      break;
  }
}
