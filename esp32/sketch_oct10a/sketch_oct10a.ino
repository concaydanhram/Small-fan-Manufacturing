#include <WiFi.h>
#include <PubSubClient.h>

// 1. Wi-Fi
const char* ssid = "An_Home T3";
const char* password = "*12345678#";

// 2. MQTT Broker
const char* mqtt_server = "192.168.113.203";
const int mqtt_port = 1883;
const char* mqtt_topic_pub = "fan_data";

// 3. UART2
#define RXD2 16
#define TXD2 17

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    String clientId = "ESP32_Gateway_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected!");
    } else {
      Serial.print("Failed, state = ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // UART2: RX = GPIO16, TX = GPIO17
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial2.setTimeout(100);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(512);
}

void loop() {
  // Duy trì kết nối Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  // Duy trì kết nối MQTT
  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  // Nhận một dòng JSON từ STM32
  if (Serial2.available() > 0) {
    String payload = Serial2.readStringUntil('\n');
    payload.trim();

    if (payload.length() == 0) {
      return;
    }

    Serial.print("Received from STM32: ");
    Serial.println(payload);

    // Publish dữ liệu lên MQTT
    bool success = client.publish(
      mqtt_topic_pub,
      payload.c_str()
    );

    if (success) {
      Serial.println("Published to MQTT successfully!");
    } else {
      Serial.println("MQTT publish failed!");
    }
  }
}
