#include "app_rtos.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <global_var.hpp>
#include <MQTT.hpp>
#include <FSM_SETTING.hpp>
#include <display.hpp>
#include <sensors.hpp>
#include <sensor_dashboard.hpp>
#include <control.hpp>

static bool rtosStarted = false;

static void taskComms(void* parameter)
{
  (void)parameter;
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(20);

  for (;;) {
    millis_update();
    mqttLoop();
    handleSensorDashboard();
    vTaskDelayUntil(&lastWake, period);
  }
}

static void taskControl(void* parameter)
{
  (void)parameter;
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(PERIOD_100MS);

  for (;;) {
    vTaskDelayUntil(&lastWake, period);
    millis_update();
    led_rgb_tick();
    readPIR();
    handleIRRemote();
    handlePIRControl();
    handleDoorServo();
    checkFSMTimeout();
  }
}

static void taskUi(void* parameter)
{
  (void)parameter;
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(PERIOD_1S);

  for (;;) {
    vTaskDelayUntil(&lastWake, period);
    millis_update();
    lcdDisplaySensors();
  }
}

static void taskTelemetry(void* parameter)
{
  (void)parameter;
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(PERIOD_30S);

  for (;;) {
    vTaskDelayUntil(&lastWake, period);
    millis_update();
    valueSensor();
    publishSensorData();
  }
}

void appRtosStart(void)
{
  if (rtosStarted) {
    return;
  }

  BaseType_t commsOk = xTaskCreatePinnedToCore(
      taskComms,
      "task_comms",
      6144,
      nullptr,
      3,
      nullptr,
      0);

  BaseType_t controlOk = xTaskCreatePinnedToCore(
      taskControl,
      "task_control",
      4096,
      nullptr,
      2,
      nullptr,
      1);

  BaseType_t uiOk = xTaskCreatePinnedToCore(
      taskUi,
      "task_ui",
      4096,
      nullptr,
      1,
      nullptr,
      1);

  BaseType_t telemetryOk = xTaskCreatePinnedToCore(
      taskTelemetry,
      "task_telemetry",
      4096,
      nullptr,
      1,
      nullptr,
      1);

  if (commsOk == pdPASS && controlOk == pdPASS && uiOk == pdPASS && telemetryOk == pdPASS) {
    rtosStarted = true;
    Serial.println("RTOS tasks started: comms, control, ui, telemetry");
  } else {
    Serial.println("RTOS task creation failed");
  }
}
