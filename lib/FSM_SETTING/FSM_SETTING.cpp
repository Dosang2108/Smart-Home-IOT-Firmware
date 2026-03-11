#include <FSM_SETTING.hpp>
#include <control.hpp>
#include <music_dec.hpp>
#include <MQTT.hpp>

static IRrecv irrecv(IR_RECV_PIN);
static decode_results irResults;

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
#define IR_CODE_F  0x09  // OK/Enter button as "F"

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
    case IR_CODE_F: return 'F';
    default: return '\0';
  }
}

void initIR()
{
  irrecv.enableIRIn();
  Serial.println("IR receiver initialized");
}

void handleIRRemote()
{
  if (irrecv.decode(&irResults)) {
    #ifdef IR_DEBUG
    serialPrintUint64(irResults.value, HEX);
    Serial.printf("  Protocol: %d\n", irResults.decode_type);
    #endif

    char key = mapIRCode(irResults.value);
    if (key != '\0') {
      Serial.printf("IR Key: %c\n", key);
      play_beep(BUZZER_PIN);
      processPasswordFSM(key);
    }
    irrecv.resume();
  }
}

void processPasswordFSM(char key)
{
  if (passwordStatus == PASSWORD_STATE_CHECK) {
    // State 0: Check password mode
    if (key >= '0' && key <= '9') {
      inputPass += key;
      Serial.printf("PASS input: %s\n", inputPass.c_str());
    }
    else if (key == 'F') {
      // Check for double-password entry (e.g., "123123") -> switch to change mode
      String doublePass = adminPassword + adminPassword;
      if (inputPass == doublePass) {
        passwordStatus = PASSWORD_STATE_CHANGE;
        inputPass = "";
        Serial.println("FSM -> Password CHANGE mode");
        publishFeedback("Che do doi mat ma");
        play_success_sound(BUZZER_PIN);
      }
      // Check normal password
      else if (inputPass == adminPassword) {
        Serial.println("Password CORRECT -> Opening door");
        publishFeedback("Mat ma dung - Mo cua");
        servo_open();
        doorState = DOOR_OPENING;
        doorOpenTime = millis();
        play_success_sound(BUZZER_PIN);
        inputPass = "";
      }
      else {
        Serial.println("Password WRONG");
        publishFeedback("Sai mat ma!");
        play_error_sound(BUZZER_PIN);
        inputPass = "";
      }
    }
  }
  else if (passwordStatus == PASSWORD_STATE_CHANGE) {
    // State 1: Change password mode
    if (key >= '0' && key <= '9') {
      inputPass += key;
      Serial.printf("New PASS input: %s\n", inputPass.c_str());
    }
    else if (key == 'F') {
      if (inputPass.length() > 0) {
        adminPassword = inputPass;
        Serial.printf("Password changed to: %s\n", adminPassword.c_str());
        publishFeedback("Da doi mat ma");
        play_success_sound(BUZZER_PIN);
      }
      inputPass = "";
      passwordStatus = PASSWORD_STATE_CHECK;
      Serial.println("FSM -> Password CHECK mode");
    }
  }
}