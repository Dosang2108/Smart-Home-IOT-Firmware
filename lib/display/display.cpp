#include "display.hpp"

LiquidCrystal_I2C lcd(0x21, 16, 2);

void initNTP()
{
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("NTP initialized (GMT+7)");
}

// Row 1: T:xx.x H:xx L:xxx
void lcdDisplayRow1()
{
  char row1[17];
  snprintf(row1, sizeof(row1), "T:%.1f H:%.0f L:%d",
           Value_Temperature, Value_Humidity, Value_Light);
  lcd.setCursor(0, 0);
  lcd.print(row1);
  for (int i = strlen(row1); i < 16; i++) lcd.print(' ');
}

// Row 2: DD/MM/YYYY HH:MM
void lcdDisplayRow2()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    lcd.setCursor(0, 1);
    lcd.print("Time not synced ");
    return;
  }
  char row2[17];
  snprintf(row2, sizeof(row2), "%02d/%02d/%04d %02d%c%02d",
           timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
           timeinfo.tm_hour, colonVisible ? ':' : ' ', timeinfo.tm_min);
  lcd.setCursor(0, 1);
  lcd.print(row2);
}

// Update only the colon character at position 13 on row 2
void lcdUpdateColon()
{
  lcd.setCursor(13, 1);
  lcd.print(colonVisible ? ':' : ' ');
}