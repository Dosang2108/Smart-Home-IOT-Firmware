#include "sensors.hpp"

DHT20 DHT; //*********** */
int soilMoistureSensor;
int lightSensor;
int temperatureSensor;
int humiditySensor;

void valueSensor()
{
  readDHT20();
  readSoilMoisture();
  readLight();
}

void readDHT20()
{
  DHT.read();
  Value_Humidity = round(DHT.getHumidity() * 100) / 100.0;
  Value_Temperature = round(DHT.getTemperature() * 100) / 100.0;
  // Serial.print("Temperature: ");
  // Serial.println(DHT.getTemperature());
  // Serial.print("Humidity: ");
  // Serial.println(DHT.getHumidity());

}

void readSoilMoisture()
{
  int soilMoistureValue = analogRead(soilMoisturePin);
  soilMoistureSensor = (soilMoistureValue * 100 / 4095);
  Value_SoilMoisture = (soilMoistureSensor * 100) / 100.0;
  // Serial.print("Soil Moisture: ");
  // Serial.print(soilMoistureSensor);
  // Serial.println("%");
}

void readLight()
{
  int lightValue = analogRead(light);
  lightSensor = lightValue * 100 / 4095;
  Value_Light = round(lightSensor * 100) / 100.0;
  // Serial.print("Light Level: ");
  // Serial.print(lightSensor);
}