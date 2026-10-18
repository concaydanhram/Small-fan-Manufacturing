#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "An_Home T3_5G";
const char* password = "*12345678#";

// --- Cấu hình MQTT Broker (HiveMQ Public) ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_pub = "fan_data";

// Khởi tạo các đối tượng mạng
WiFiClient espClient;
PubSubClient client(espClient);

// Cấu hình cổng UART2 (Hardware Serial 2 trên ESP32)
#define RXD2 16
#define TXD2 17

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Lặp lại cho đến khi kết nối thành công với MQTT Broker
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Tạo ID ngẫu nhiên cho Client để tránh trùng lặp trên Public Broker
    String clientId = "ESP32_Gateway_";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // Cổng Serial 0: Dùng để in log ra màn hình máy tính (Debug)
  Serial.begin(115200);
  
  // Cổng Serial 2: Dùng để nhận chuỗi JSON từ STM32
  // Tốc độ 115200 bps phải khớp chính xác với cấu hình trên CubeMX
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Giữ kết nối MQTT sống (Keep-alive)

  // Kiểm tra xem STM32 có gửi gói dữ liệu nào qua UART hay không
  if (Serial2.available()) {
    // Đọc toàn bộ chuỗi cho đến khi gặp ký tự xuống dòng (\n)
    String json_payload = Serial2.readStringUntil('\n');
    
    // In ra màn hình máy tính để kiểm tra
    Serial.print("Received from STM32: ");
    Serial.println(json_payload);
    
    // Đẩy chuỗi JSON thẳng lên MQTT Broker
    if (client.publish(mqtt_topic_pub, json_payload.c_str())) {
      Serial.println("-> Published to MQTT successfully.");
    } else {
      Serial.println("-> Publish failed!");
    }
  }
}
