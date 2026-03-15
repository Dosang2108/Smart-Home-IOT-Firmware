#include "sensors.hpp"

DHT20 DHT;

bool initDHT20()
{
  if (!DHT.begin()) {
    Serial.println("DHT20 init failed");
    dhtDataValid = false;
    return false;
  }
  Serial.println("DHT20 init success");
  dhtDataValid = true;
  return true;
}

void valueSensor()
{
  readDHT20();
  readSoilMoisture();
  readLight();
}

void readDHT20()
{
  if (DHT.read() == DHT20_OK) {
    Value_Humidity = round(DHT.getHumidity() * 100) / 100.0;
    Value_Temperature = round(DHT.getTemperature() * 100) / 100.0;
    dhtDataValid = true;
  } else {
    dhtDataValid = false;
    Serial.println("DHT20 read failed");
  }
}

void readSoilMoisture()
{
  int soilMoistureValue = analogRead(soilMoisturePin);
  Value_SoilMoisture = soilMoistureValue * 100 / 4095;
}

void readLight()
{
  int lightValue = analogRead(light);
  Value_Light = lightValue * 100 / 4095;
}

void readPIR()
{
  pirDetected = digitalRead(PIR_PIN);
}