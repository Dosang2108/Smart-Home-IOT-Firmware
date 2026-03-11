#include <MQTT.hpp>

long lastMsg = 0; // Thời điểm gửi tin nhắn cuối cùng

// Khởi tạo MQTT Client
WiFiClientSecure espClient;
PubSubClient client(espClient);
bool flag_transmit = false;
int count_flag = 0;

void init_Wifi_and_MQTT(void)
{
  // Kết nối WiFi
  Serial.print("Đang kết nối WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối");

  // Cấu hình MQTT Server và callback
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(receiver_data);
}

String mode = "AUTO";
// Transmit_data
void Transmit_receiver_data(void)
{
  if (!client.connected())
  {
    reconnect(); // Kết nối lại nếu mất kết nối
  }
  client.loop(); // Lắng nghe dữ liệu từ MQTT

  // Gửi dữ liệu mỗi 100 ms
  if (millis_present - lastMsg > priod_receiver_data)
  {
    lastMsg = millis_present;
    StaticJsonDocument<200> doc;
    StaticJsonDocument<200> statusdevice;
    // doc["temperature"] = 25;
    // doc["humidity"] = 60;
    // doc["pressure"] = 1012;
    // doc["gas"] = 300;
    doc["area"] = ID_area_send;
    doc["temperature"] = Value_Temperature;
    doc["humidity"] = Value_Humidity;
    doc["light"] = Value_Light;
    doc["soilMoisture"] = Value_SoilMoisture;

    //statusdevice["deviceName"]=0;
    statusdevice["area"]= ID_area_send;
    statusdevice["mode"]= mode;

    char output[200];
    serializeJson(doc, output);
    char output1[200];
    serializeJson(statusdevice, output1);

    if (flag_transmit){
      Serial.println("------------------------------------------------");
      Serial.println("Gửi dữ liệu tới topic:");
      Serial.println(topicPub);
      Serial.println(output);
      client.publish(topicPub, output);
      Serial.println("------------------------------------------------");
    }
    if(!flag_transmit)
    {
      Serial.println("------------------------------------------------");
      Serial.println("Gửi dữ liệu tới topic:");
      Serial.println(output1);
      client.publish(topicPub, output1);
      Serial.println("------------------------------------------------");
      if(count_flag > 5){ 
        flag_transmit = true;
      }
      count_flag++;
      delay(1000);
    }
    
    
  }
}

// Hàm xử lý khi nhận dữ liệu từ MQTT
// Dự phòng dung lượng đủ để chứa dữ liệu JSON
StaticJsonDocument<200> receivedValues;

// Hàm xử lý khi nhận dữ liệu từ MQTT
void receiver_data(char *topic, byte *payload, unsigned int length)
{
  // Kiểm tra dữ liệu payload có hợp lệ không
  if (payload == nullptr || length == 0)
  {
    Serial.println("Received empty or invalid payload.");
    return;
  }

  // Chuyển payload (dữ liệu nhận được) thành chuỗi string
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0'; // Đảm bảo kết thúc chuỗi

  // In ra chuỗi JSON nhận được
  Serial.println("Received JSON string:");
  Serial.println(message);

  // Giải mã JSON từ chuỗi string
  DeserializationError error = deserializeJson(receivedValues, message);
  if (error)
  {
    Serial.printf("JSON parsing failed: %s\n", error.c_str());
    return;
  }

  if (receivedValues["area"].as<int>() == ID_area_recv){
      // Kiểm tra và lấy các giá trị từ JSON
    Serial.printf("Area: %d\n",receivedValues["area"].as<int>());
    if (receivedValues.containsKey("deviceMode"))
        {
          State_FSM = receivedValues["deviceMode"].as<bool>();
          Serial.printf("deviceMode: %d\n", State_FSM);
        }
    if (receivedValues.containsKey("deviceName"))
    {
        
      // fan1
      if (receivedValues["deviceName"].as<String>() == "Fan1")
      {
        Fan1.name = receivedValues["deviceName"].as<String>();
        Serial.printf("Device Name: %s\n", Fan1.name.c_str());

        if (receivedValues.containsKey("active"))
        {
          Fan1.active = receivedValues["active"].as<bool>();
          Serial.printf("Active: %d\n", Fan1.active);
        }
        else
        {
          Serial.println("No Fan1 active found in JSON.");
        }

        if (receivedValues.containsKey("value"))
        {
          Fan1.value = receivedValues["value"].as<int>();
          Serial.printf("value: %d\n", Fan1.value);
        }
        else
        {
          Serial.println("No Fan1 value found in JSON.");
        }

        // if (receivedValues.containsKey("deviceMode"))
        // {
        //   State_FSM = receivedValues["deviceMode"].as<bool>();
        //   Serial.printf("deviceMode: %d\n", State_FSM);
        // }
      }

      // Pump1
      if (receivedValues["deviceName"].as<String>() == "Pump1")
      {
        Pump1.name = receivedValues["deviceName"].as<String>();
        Serial.printf("Device Name: %s\n", Pump1.name.c_str());

        if (receivedValues.containsKey("active"))
        {
          Pump1.active = receivedValues["active"].as<bool>();
          Serial.printf("Active: %d\n", Pump1.active);
        }
        else
        {
          Serial.println("No Pump1 active found in JSON.");
        }

        if (receivedValues.containsKey("value"))
        {
          Pump1.value = receivedValues["value"].as<int>();
          Serial.printf("value: %d\n", Pump1.value);
        }
        else
        {
          Serial.println("No Pump1 value found in JSON.");
        }

        // if (receivedValues.containsKey("deviceMode"))
        // {
        //   State_FSM = receivedValues["deviceMode"].as<bool>();
        //   Serial.printf("deviceMode: %d\n", State_FSM);
        // }
      }

      // Led1
      if (receivedValues["deviceName"].as<String>() == "Led1")
      {
        Led1.name = receivedValues["deviceName"].as<String>();
        Serial.printf("Device Name: %s\n", Led1.name.c_str());

        if (receivedValues.containsKey("active"))
        {
          Led1.active = receivedValues["active"].as<bool>();
          Serial.printf("Active: %d\n", Led1.active);
        }
        else
        {
          Serial.println("No Led1 active found in JSON.");
        }

        if (receivedValues.containsKey("value"))
        {
          Led1.value = receivedValues["value"].as<int>();
          Serial.printf("Value: %d\n", Led1.value);
        }
        else
        {
          Serial.println("No Led1 value found in JSON.");
        }

        // if (receivedValues.containsKey("deviceMode"))
        // {
        //   State_FSM = receivedValues["deviceMode"].as<bool>();
        //   Serial.printf("deviceMode: %d\n", State_FSM);
        // }
      }
    }

    else
    {
      Serial.println("No deviceName found in JSON.");
    }

  }

  

  //   // Fan2
  //   if (receivedValues["deviceName"].as<String>() == "Fan2")
  //   {
  //     Fan2.name = receivedValues["deviceName"].as<String>();
  //     Serial.printf("Device Name: %s\n", Fan2.name.c_str());

  //     if (receivedValues.containsKey("active"))
  //     {
  //       Fan2.active = receivedValues["active"].as<bool>();
  //       Serial.printf("Active: %d\n", Fan2.active);
  //     }
  //     else
  //     {
  //       Serial.println("No Fan2 active found in JSON.");
  //     }

  //     if (receivedValues.containsKey("value"))
  //     {
  //       Fan2.value = receivedValues["value"].as<int>();
  //       Serial.printf("value: %d\n", Fan2.value);
  //     }
  //     else
  //     {
  //       Serial.println("No Fan2 value found in JSON.");
  //     }

  //     if (receivedValues.containsKey("deviceMode"))
  //     {
  //       State_FSM = receivedValues["deviceMode"].as<bool>();
  //       Serial.printf("deviceMode: %d\n", State_FSM);
  //     }
  //   }

  //   // Pump2
  //   if (receivedValues["deviceName"].as<String>() == "Pump2")
  //   {
  //     Pump2.name = receivedValues["deviceName"].as<String>();
  //     Serial.printf("Device Name: %s\n", Pump2.name.c_str());

  //     if (receivedValues.containsKey("active"))
  //     {
  //       Pump2.active = receivedValues["active"].as<bool>();
  //       Serial.printf("Active: %d\n", Pump2.active);
  //     }
  //     else
  //     {
  //       Serial.println("No Pump2 active found in JSON.");
  //     }

  //     if (receivedValues.containsKey("value"))
  //     {
  //       Pump2.value = receivedValues["value"].as<int>();
  //       Serial.printf("value: %d\n", Pump2.value);
  //     }
  //     else
  //     {
  //       Serial.println("No Pump2 value found in JSON.");
  //     }

  //     if (receivedValues.containsKey("deviceMode"))
  //     {
  //       State_FSM = receivedValues["deviceMode"].as<bool>();
  //       Serial.printf("deviceMode: %d\n", State_FSM);
  //     }
  //   }

  //   // Led2
  //   if (receivedValues["deviceName"].as<String>() == "Led2")
  //   {
  //     Led2.name = receivedValues["deviceName"].as<String>();
  //     Serial.printf("Device Name: %s\n", Led2.name.c_str());

  //     if (receivedValues.containsKey("active"))
  //     {
  //       Led2.active = receivedValues["active"].as<bool>();
  //       Serial.printf("Active: %d\n", Led2.active);
  //     }
  //     else
  //     {
  //       Serial.println("No Led2 active found in JSON.");
  //     }

  //     if (receivedValues.containsKey("value"))
  //     {
  //       Led2.value = receivedValues["value"].as<int>();
  //       Serial.printf("value: %d\n", Led2.value);
  //     }
  //     else
  //     {
  //       Serial.println("No Led2 value found in JSON.");
  //     }

  //     if (receivedValues.containsKey("deviceMode"))
  //     {
  //       State_FSM = receivedValues["deviceMode"].as<bool>();
  //       Serial.printf("deviceMode: %d\n", State_FSM);
  //     }
  //   }
  // }


  //   if (receivedValues.containsKey("active")) {
  //     bool active = receivedValues["active"].as<bool>();
  //     Serial.printf("Active: %d\n", active);
  //   } else {
  //     Serial.println("No active found in JSON.");
  //   }

  //   if (receivedValues.containsKey("value")) {
  //     JsonVariant value = receivedValues["value"];
  //     if (value.isNull()) {
  //       Serial.println("Value is null");
  //     } else {
  //       Serial.printf("Value: %s\n", value.as<String>().c_str());
  //     }
  //   } else {
  //     Serial.println("No value found in JSON.");
  //   }
}

// Hàm tự động kết nối lại MQTT
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Đang kết nối lại MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    //clientID += String(random(0xffff),HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Đã kết nối MQTT");
      client.subscribe(topicSub); // Đăng ký nhận dữ liệu từ topic
      Serial.print("Đã đăng ký lắng nghe topic: ");
      Serial.println(topicSub);
    }
    else
    {
      Serial.print("Kết nối thất bại, mã lỗi: ");
      Serial.println(client.state());
      delay(5000); // Đợi 5 giây trước khi thử lại
    }
  }
}


