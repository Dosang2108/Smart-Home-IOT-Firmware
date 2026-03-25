#include <FSM_SETTING.hpp>
#include <control.hpp>
#include <MQTT.hpp>
#include <Preferences.h> // Thư viện đọc/ghi bộ nhớ Flash

static IRrecv irrecv(IR_RECV_PIN);
static decode_results irResults;
static Preferences preferences; // Đối tượng thao tác Flash

// Biến quản lý thời gian Timeout
static unsigned long lastKeyPressTime = 0;
static const unsigned long FSM_TIMEOUT_MS = 10000; // 10 giây

// ============ IR Remote Button Code Mapping ============
#define IR_DEBUG

#define IR_CODE_0  0x16
#define IR_CODE_1  0x0C
#define IR_CODE_2  0x18
#define IR_CODE_3  0x5E
#define IR_CODE_4  0x08
#define IR_CODE_5  0x1C
#define IR_CODE_6  0x5A
#define IR_CODE_7  0x42
#define IR_CODE_8  0x52
#define IR_CODE_9  0x4A
#define IR_CODE_F  0x09  // Nút OK/Enter
#define IR_CODE_C  0x46  // Nút Xóa lùi

static char mapIRCode(uint64_t value)
{
  uint8_t cmd = value & 0xFF;
  switch (cmd) {
    case IR_CODE_0: return '0';
    case IR_CODE_1: return '1';
    case IR_CODE_2: return '2';
    case IR_CODE_3: return '3';
    case IR_CODE_4: return '4';
    case IR_CODE_5: return '5';
    case IR_CODE_6: return '6';
    case IR_CODE_7: return '7';
    case IR_CODE_8: return '8';
    case IR_CODE_9: return '9';
    case IR_CODE_F: return 'F'; // Enter
    case IR_CODE_C: return 'C'; // Clear / Backspace
    default: return '\0';
  }
}

void initIR()
{
  irrecv.enableIRIn();
  Serial.println("IR receiver initialized");

  // Khởi tạo và nạp mật khẩu đã lưu từ trước (Chống cúp điện)
  preferences.begin("door-lock", false); 
  adminPassword = preferences.getString("adminPass", "123");
  Serial.printf("Loaded Password from Flash: %s\n", adminPassword.c_str());
}

void handleIRRemote()
{
  if (irrecv.decode(&irResults)) {
    // THÊM DÒNG NÀY: Chỉ xử lý nếu tín hiệu KHÔNG PHẢI là nhiễu (UNKNOWN = -1)
    if (irResults.decode_type != -1) { 
      
      #ifdef IR_DEBUG
      serialPrintUint64(irResults.value, HEX);
      Serial.printf("  Protocol: %d\n", irResults.decode_type);
      #endif

      char key = mapIRCode(irResults.value);
      if (key != '\0') {
        Serial.printf("IR Key: %c\n", key);
        
        lastKeyPressTime = millis_present; // Reset bộ đếm timeout
        processPasswordFSM(key);
      }
    }
    
    // Resume phải luôn được gọi để nhận tín hiệu tiếp theo
    irrecv.resume(); 
  }
}
void processPasswordFSM(char key)
{
  // --- TÍNH NĂNG XÓA LÙI KÝ TỰ ---
  if (key == 'C') {
    if (inputPass.length() > 0) {
      inputPass.remove(inputPass.length() - 1);
      Serial.printf("Deleted 1 char. Current input: %s\n", inputPass.c_str());
    }
    return;
  }

  if (passwordStatus == PASSWORD_STATE_CHECK) {
    // State 0: Check password mode
    if (key >= '0' && key <= '9') {
      if (inputPass.length() < 10) { 
        inputPass += key;
        Serial.printf("PASS input: %s\n", inputPass.c_str());
      }
    }
    else if (key == 'F') {
      String doublePass = adminPassword + adminPassword;
      if (inputPass == doublePass) {
        passwordStatus = PASSWORD_STATE_CHANGE;
        inputPass = "";
        Serial.println("FSM -> Password CHANGE mode");
        publishFeedback("Che do doi mat ma");
      }
      else if (inputPass == adminPassword) {
        Serial.println("Password CORRECT -> Opening door");
        publishFeedback("Mat ma dung - Mo cua");
        servo_open();
        doorState = DOOR_OPENING;
        doorOpenTime = millis_present; 
        inputPass = "";
      }
      else {
        Serial.println("Password WRONG");
        publishFeedback("Sai mat ma!");
        inputPass = "";
      }
    }
  }
  else if (passwordStatus == PASSWORD_STATE_CHANGE) {
    // State 1: Change password mode
    if (key >= '0' && key <= '9') {
      if (inputPass.length() < 10) { 
        inputPass += key;
        Serial.printf("New PASS input: %s\n", inputPass.c_str());
      }
    }
    else if (key == 'F') {
      if (inputPass.length() > 0) {
        adminPassword = inputPass;
        
        // LƯU VÀO Ổ CỨNG FLASH
        preferences.putString("adminPass", adminPassword); 
        
        Serial.printf("Password changed to: %s\n", adminPassword.c_str());
        publishFeedback("Da doi mat ma thanh cong");
      }
      inputPass = "";
      passwordStatus = PASSWORD_STATE_CHECK;
      Serial.println("FSM -> Password CHECK mode");
    }
  }
}

// --- TÍNH NĂNG TỰ ĐỘNG HỦY THAO TÁC NẾU QUÊN (TIMEOUT) ---
void checkFSMTimeout()
{
  if (inputPass.length() > 0 || passwordStatus != PASSWORD_STATE_CHECK) {
    if (millis_present - lastKeyPressTime > FSM_TIMEOUT_MS) {
      inputPass = "";
      passwordStatus = PASSWORD_STATE_CHECK; 
      
      Serial.println("FSM Timeout! Resetting to default state.");
      publishFeedback("Het thoi gian nhap. Da huy thao tac!");
      
      lastKeyPressTime = millis_present; 
    }
  }
}