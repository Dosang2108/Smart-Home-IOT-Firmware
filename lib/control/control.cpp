#include "control.hpp"

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);
static Servo doorServo;
static bool rgbAutoCycle = false;
static uint8_t rgbCycleIndex = 0;
static unsigned long lastRgbCycleMs = 0;
static const unsigned long RGB_CYCLE_PERIOD_MS = 2000;

static unsigned long lastMotionTime = 0;
static const unsigned long PIR_HOLD_TIME_MS = 5000; // Giữ đèn sáng 5 giây


struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

static const RgbColor kRgbCycleColors[] = {
  {255, 0, 0},
  {255, 127, 0},
  {255, 255, 0},
  {0, 255, 0},
  {0, 255, 255},
  {0, 0, 255},
  {148, 0, 211},
  {255, 255, 255}
};

static const uint8_t kRgbCycleColorCount = sizeof(kRgbCycleColors) / sizeof(kRgbCycleColors[0]);

// ============ Pump ============
// void pump_on()
// {
//   analogWrite(pump, 255);
// }

// void pump_off()
// {
//   analogWrite(pump, 0);
// }

// void pump_control_manual(int pumpspeed)
// {
//   pumpspeed = map(pumpspeed, 0, 100, 0, 255);
//   analogWrite(pump, pumpspeed);
// }

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

void led_rgb_set(uint8_t r, uint8_t g, uint8_t b)
{
  // Dùng vòng lặp chạy từ 0 đến NUM_PIXELS (tức là 0, 1, 2, 3) để bật tất cả
  for(int i = 0; i < NUM_PIXELS; i++) {
    NeoPixel.setPixelColor(i, NeoPixel.Color(r, g, b));
  }
  NeoPixel.show();
}

void led_rgb_off()
{
  NeoPixel.clear(); 
  NeoPixel.show();
}

void led_rgb_set_auto(bool enabled)
{
  rgbAutoCycle = enabled;
  if (enabled) {
    rgbCycleIndex = 0;
    lastRgbCycleMs = 0;
  }
}

bool led_rgb_is_auto(void)
{
  return rgbAutoCycle;
}

void led_rgb_tick(void)
{
  if (!rgbAutoCycle || !mqttLedState) {
    return;
  }
  if (millis_present - lastRgbCycleMs < RGB_CYCLE_PERIOD_MS) {
    return;
  }
  lastRgbCycleMs = millis_present;
  const RgbColor color = kRgbCycleColors[rgbCycleIndex];
  mqttLedR = color.r;
  mqttLedG = color.g;
  mqttLedB = color.b;
  led_rgb_set(color.r, color.g, color.b); // Hàm này giờ đã bật cả 4 bóng
  rgbCycleIndex = (rgbCycleIndex + 1) % kRgbCycleColorCount;
}

void ledwhite_on()
{
  // Sửa i = 1 thành i = 0 để cảm biến PIR bật toàn bộ 4 đèn
  for (int i = 0; i < NUM_PIXELS; i++) {
    NeoPixel.setPixelColor(i, NeoPixel.Color(255, 255, 255));
  }
  NeoPixel.show();
}

void ledwhite_off()
{
  NeoPixel.clear();
  NeoPixel.show();
}

void ledred_on()
{
  led_rgb_set(255, 0, 0);
}

void ledgreen_on()
{
  led_rgb_set(0, 255, 0);
}

void led_off()
{
  led_rgb_off();
}
// ============ PIR Auto Light ============
void handlePIRControl()
{
  // Đang điều khiển bằng tay qua MQTT/App thì bỏ qua tự động
  if (mqttLedState) {
    return;
  }

  // Nếu phát hiện chuyển động
  if (pirDetected) {
    ledwhite_on();
    lastMotionTime = millis_present; // Cập nhật lại mốc thời gian liên tục
  } 
  // Nếu không phát hiện chuyển động
  else {
    // Chỉ tắt đèn khi ĐÃ QUÁ 5 GIÂY kể từ lần cuối cùng thấy người
    if (millis_present - lastMotionTime >= PIR_HOLD_TIME_MS) {
      ledwhite_off();
    }
  }
}

// ============ Servo ============
static const uint8_t DOOR_SERVO_OPEN_ANGLE = 90;
static const uint8_t DOOR_SERVO_CLOSED_ANGLE = 0;

static void attachDoorServoIfNeeded()
{
  if (!doorServo.attached()) {
    doorServo.attach(SERVO_PIN);
  }
}

void initServo()
{
  attachDoorServoIfNeeded();
  doorServo.write(DOOR_SERVO_CLOSED_ANGLE);
  delay(500);
  doorServo.detach();
  doorState = DOOR_CLOSED;
}

void servo_open()
{
  attachDoorServoIfNeeded();
  doorServo.write(DOOR_SERVO_OPEN_ANGLE);
}

void servo_close()
{
  attachDoorServoIfNeeded();
  doorServo.write(DOOR_SERVO_CLOSED_ANGLE);
}

void servo_detach_motor()
{
  doorServo.detach();
}

void door_command_open()
{
  servo_open();
  doorState = DOOR_OPENING;
  doorOpenTime = millis_present;
}

void door_command_close()
{
  servo_close();
  doorState = DOOR_CLOSING;
  doorOpenTime = millis_present;
}

const char* door_state_to_text(DoorState state)
{
  switch (state) {
    case DOOR_CLOSED:
      return "closed";
    case DOOR_OPENING:
      return "opening";
    case DOOR_CLOSING:
      return "closing";
    default:
      return "unknown";
  }
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