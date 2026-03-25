#include "display.hpp"

// Khởi tạo đối tượng toàn cục với địa chỉ tạm (sẽ được cập nhật lại sau khi dò quét)
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);

static uint8_t currentSensorPage = 0;
static uint8_t lastSensorPage = 255;
static unsigned long lastPageSwitchMs = 0;
static unsigned long lastPageRenderMs = 0;
static const unsigned long LCD_PAGE_SWITCH_MS = 5000;
static const unsigned long LCD_RENDER_MS = 1000;

static void printPaddedLine(uint8_t row, const char* text)
{
  char line[17];
  snprintf(line, sizeof(line), "%-16.16s", text);
  lcd.setCursor(0, row);
  lcd.print(line);
}

static void displayTempHumidityPage()
{
  char line1[32];
  char line2[32];

  if (dhtDataValid) {
    snprintf(line1, sizeof(line1), "Temp: %.1f C", Value_Temperature);
    snprintf(line2, sizeof(line2), "Hum : %.1f %%", Value_Humidity);
  } else {
    snprintf(line1, sizeof(line1), "DHT20 Error");
    snprintf(line2, sizeof(line2), "Check wiring");
  }

  printPaddedLine(0, line1);
  printPaddedLine(1, line2);
}

static void displayAnalogPage()
{
  char line1[32];
  char line2[32];

  snprintf(line1, sizeof(line1), "Light: %d %%", Value_Light);
  snprintf(line2, sizeof(line2), "                "); 

  printPaddedLine(0, line1);
  printPaddedLine(1, line2);
}

static void displayPirPage()
{
  char line1[32];
  char line2[32];

  snprintf(line1, sizeof(line1), "PIR: %s", pirDetected ? "Motion" : "No motion");
  snprintf(line2, sizeof(line2), "Sensor page 3/3");

  printPaddedLine(0, line1);
  printPaddedLine(1, line2);
}
static uint8_t detectLcdAddress() {
  // 1. Ưu tiên kiểm tra các địa chỉ phổ biến của module chuyển đổi I2C LCD
  const uint8_t commonAddresses[] = {0x27, 0x3F, 0x20, 0x26};
  for (int i = 0; i < sizeof(commonAddresses); i++) {
    Wire.beginTransmission(commonAddresses[i]);
    if (Wire.endTransmission() == 0) {
      return commonAddresses[i]; // Tìm thấy địa chỉ hợp lệ
    }
  }

  // 2. Nếu không có trong danh sách trên, quét toàn bộ I2C Bus
  for (uint8_t address = 1; address < 127; address++) {
    // Phải bỏ qua 0x38 vì đó là địa chỉ của cảm biến DHT20
    if (address == 0x38) continue; 

    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      return address;
    }
  }
  
  return 0x27; // Trả về mặc định nếu rút dây/lỗi để tránh hệ thống crash
}

void initLCD()
{
  Serial.println("Detecting LCD I2C Address...");
  
  // Tự động tìm địa chỉ
  uint8_t foundAddress = detectLcdAddress();
  Serial.printf("Auto-detected LCD Address: 0x%02X\n", foundAddress);
  
  // Khởi tạo lại đối tượng LCD bằng địa chỉ vừa tìm được
  lcd = LiquidCrystal_I2C(foundAddress, LCD_COLUMNS, LCD_ROWS);
  
  lcd.init(); 
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  lcd.setCursor(0, 1);
  lcd.print("LCD Connected");
  delay(2000);
  Serial.println("LCD: Connected successfully");
}

void scanI2CBus()
{
  Serial.println("I2C scan start...");
  uint8_t found = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) {
        Serial.print('0');
      }
      Serial.println(address, HEX);
      found++;
    }
    delay(2);
  }
  if (found == 0) {
    Serial.println("No I2C devices found.");
  } else {
    Serial.print("I2C scan done. Devices found: ");
    Serial.println(found);
  }
}

void initNTP()
{
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("NTP initialized (GMT+7)");
}

void lcdDisplaySensors()
{
  unsigned long now = millis();

  if (now - lastPageSwitchMs >= LCD_PAGE_SWITCH_MS) {
    lastPageSwitchMs = now;
    currentSensorPage = (currentSensorPage + 1) % 3;
  }

  if (currentSensorPage != lastSensorPage) {
    lcd.clear();
    lastSensorPage = currentSensorPage;
    lastPageRenderMs = 0;
  }

  if (lastPageRenderMs != 0 && (now - lastPageRenderMs < LCD_RENDER_MS)) {
    return;
  }
  lastPageRenderMs = now;

  switch (currentSensorPage) {
    case 0:
      displayTempHumidityPage();
      break;
    case 1:
      displayAnalogPage();
      break;
    case 2:
      displayPirPage();
      break;
  }
}

void lcdDisplayRow1()
{
  displayTempHumidityPage();
}
void lcdDisplayRow2()
{
  displayAnalogPage();
}

void lcdUpdateColon()
{
}