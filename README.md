# ESP32 – Mosquitto MQTT Setup Guide

Hướng dẫn cài đặt và cấu hình ESP32 kết nối với Mosquitto MQTT Broker trên máy tính Windows để truyền dữ liệu cảm biến qua mạng Wi-Fi.

## 1. System Architecture

```text
STM32
  |
  | UART (115200 baud)
  v
ESP32
  |
  | Wi-Fi / MQTT
  v
Mosquitto MQTT Broker (Windows PC)
  |
  | Topic: fan_data
  v
MQTT Subscriber / Backend Server
```

ESP32 nhận dữ liệu JSON từ STM32 qua UART và publish message lên MQTT Broker thông qua Wi-Fi.

## 2. Requirements

- ESP32 development board.
- Máy tính Windows kết nối Wi-Fi.
- Arduino IDE.
- Mosquitto MQTT Broker for Windows.
- Thư viện Arduino:
  - `WiFi.h`
  - `PubSubClient.h`

Cài đặt thư viện `PubSubClient` trong Arduino IDE:

1. Mở **Tools → Manage Libraries**.
2. Tìm `PubSubClient`.
3. Cài đặt thư viện của Nick O'Leary.

Mosquitto download: https://mosquitto.org/download/

PubSubClient: https://github.com/knolleary/pubsubclient

## 3. Install Mosquitto on Windows

1. Tải và cài đặt Mosquitto cho Windows.
2. Mở PowerShell.
3. Kiểm tra cài đặt:

```powershell
mosquitto -h
mosquitto_sub -h localhost -p 1883 -t fan_data -v
```

Nếu lệnh `mosquitto` không được nhận diện, hãy thêm thư mục cài đặt Mosquitto vào biến môi trường `PATH`, hoặc chạy lệnh bằng đường dẫn đầy đủ tới file thực thi.

## 4. Configure Mosquitto Environment

### 4.1. Find the PC IPv4 Address

Mở PowerShell:

```powershell
ipconfig
```

Tìm địa chỉ IPv4 của card Wi-Fi đang sử dụng.

Ví dụ:

```text
IPv4 Address: 192.168.113.203
```

Sử dụng địa chỉ IPv4 thực tế của máy tính trong cấu hình ESP32.

### 4.2. Create a Mosquitto Configuration File

Tạo file cấu hình:

```powershell
notepad "$env:USERPROFILE\mosquitto-local.conf"
```

Thêm nội dung:

```conf
listener 1883 0.0.0.0
allow_anonymous true
```

Giải thích:

- `listener 1883 0.0.0.0`: Cho phép Mosquitto lắng nghe kết nối IPv4 trên cổng TCP 1883.
- `allow_anonymous true`: Cho phép client kết nối mà không cần username/password.

**Security note:** Cấu hình trên chỉ phù hợp với môi trường thử nghiệm trên mạng LAN đáng tin cậy. Không sử dụng cấu hình anonymous trên broker công khai hoặc mở trực tiếp ra Internet.

### 4.3. Start the MQTT Broker

Chạy:

```powershell
mosquitto -c "$env:USERPROFILE\mosquitto-local.conf" -v
```

Giữ cửa sổ PowerShell này mở trong khi thử nghiệm.

Log mong đợi:

```text
Opening ipv4 listen socket on port 1883.
```

Nếu cổng 1883 đã được sử dụng, hãy kiểm tra các tiến trình Mosquitto đang chạy trước khi khởi động thêm broker.

### 4.4. Configure Windows Firewall

Mở PowerShell với quyền Administrator:

```powershell
New-NetFirewallRule `
  -DisplayName "Mosquitto MQTT 1883" `
  -Direction Inbound `
  -Protocol TCP `
  -LocalPort 1883 `
  -Action Allow `
  -Profile Private
```

Quy tắc này cho phép kết nối TCP đến cổng 1883 trên mạng Private. Đảm bảo mạng Wi-Fi được Windows phân loại đúng là Private nếu muốn sử dụng quy tắc này.

## 5. Configure ESP32

### 5.1. Install Required Libraries

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
```

### 5.2. Configure Wi-Fi and MQTT

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqtt_server = "192.168.113.203";
const int mqtt_port = 1883;

const char* mqtt_topic_pub = "fan_data";

#define RXD2 16
#define TXD2 17

WiFiClient espClient;
PubSubClient client(espClient);
```

Replace:

- `YOUR_WIFI_SSID` with your Wi-Fi network name.
- `YOUR_WIFI_PASSWORD` with your Wi-Fi password.
- `192.168.113.203` with the actual IPv4 address of the PC running Mosquitto.

Do not include extra spaces in the MQTT broker IP address.

### 5.3. Initialize Serial and MQTT

```cpp
void setup() {
    Serial.begin(115200);

    Serial2.begin(
        115200,
        SERIAL_8N1,
        RXD2,
        TXD2
    );

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    client.setServer(mqtt_server, mqtt_port);
    client.setBufferSize(512);
}
```

### 5.4. Connect to MQTT Broker

```cpp
void reconnect() {
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");

        if (client.connect("ESP32Client")) {
            Serial.println("MQTT connected!");
        } else {
            Serial.print("MQTT connection failed, state = ");
            Serial.println(client.state());
            delay(2000);
        }
    }
}
```

### 5.5. Publish JSON Data

Ví dụ publish dữ liệu JSON đã nhận từ STM32:

```cpp
void loop() {
    if (!client.connected()) {
        reconnect();
    }

    client.loop();

    if (Serial2.available() > 0) {
        String payload = Serial2.readStringUntil('\n');
        payload.trim();

        if (payload.length() > 0) {
            Serial.print("Received from STM32: ");
            Serial.println(payload);

            bool success = client.publish(
                mqtt_topic_pub,
                payload.c_str()
            );

            if (success) {
                Serial.println("MQTT publish successful");
            } else {
                Serial.println("MQTT publish failed");
            }
        }
    }
}
```

Đây là đoạn code mẫu cho phần kết nối và truyền dữ liệu; có thể tích hợp vào chương trình ESP32 hiện có. Dữ liệu nhận từ STM32 cần được kết thúc bằng ký tự newline `\n`.

## 6. UART Connection

Nếu sử dụng ESP32-WROOM-32 với UART2 mặc định được ánh xạ sang GPIO16/GPIO17:

| STM32 | ESP32 |
|---|---|
| USART2_TX (ví dụ PA2) | GPIO16 / RXD2 |
| USART2_RX (ví dụ PA3, nếu cần truyền hai chiều) | GPIO17 / TXD2 |
| GND | GND |

Với luồng dữ liệu một chiều từ STM32 đến ESP32, chỉ cần STM32 TX nối ESP32 RX và nối chung GND.

Kiểm tra pin mapping thực tế trong STM32CubeMX. Đảm bảo tín hiệu UART không vượt quá mức điện áp mà ESP32 hỗ trợ.

## 7. Test the MQTT Broker

### 7.1. Subscribe to a Topic

Mở PowerShell thứ hai:

```powershell
mosquitto_sub -h localhost -p 1883 -t fan_data -v
```

### 7.2. Publish a Test Message

Mở PowerShell thứ ba:

```powershell
mosquitto_pub -h localhost -p 1883 -t fan_data -m "test123"
```

Kết quả mong đợi ở subscriber:

```text
fan_data test123
```

Nếu kết quả xuất hiện, broker có thể nhận và phân phối message trong bài kiểm tra cục bộ.

### 7.3. Test ESP32 Data

1. Giữ broker chạy.
2. Khởi động MQTT subscriber với topic `fan_data`.
3. Reset ESP32.
4. Đảm bảo STM32 đang gửi JSON qua UART.
5. Kiểm tra message nhận được ở subscriber.

Ví dụ message:

```json
{"dev":"fan1","freq":50.0,"amp":0.8,"current":0.35}
```

Các giá trị trên chỉ là dữ liệu minh họa.

## 8. Troubleshooting

| Problem | Possible solution |
|---|---|
| `mosquitto` is not recognized | Add the Mosquitto installation directory to `PATH`. |
| MQTT connection fails with state `-2` | Verify the PC IPv4 address, listener, firewall and network connectivity. |
| ESP32 connects to Wi-Fi but not MQTT | Check broker IP, port 1883, active broker process and LAN client isolation. |
| Subscriber receives no messages | Verify topic name, ESP32 publish status and UART input. |
| JSON messages are incomplete | Ensure STM32 sends a newline after every JSON message. |
| Port 1883 is already in use | Inspect running processes with `netstat -ano \| findstr :1883`. |

Check the active listener:

```powershell
netstat -ano | findstr :1883
```

The following entry indicates that a process is listening on all IPv4 interfaces:

```text
TCP    0.0.0.0:1883    0.0.0.0:0    LISTENING
```

## 9. Current System Scope

- STM32 sensor data acquisition.
- UART communication between STM32 and ESP32.
- Wi-Fi connectivity on ESP32.
- MQTT publishing through Mosquitto.
- JSON data transmission using topic `fan_data`.

### Future Development

- Python backend to subscribe to MQTT messages.
- Real-time database for sensor measurements.
- FastAPI endpoints for data retrieval.
- Machine Learning anomaly detection.
- Dashboard for monitoring vibration and current measurements.

## 10. References

- Mosquitto: https://mosquitto.org/
- PubSubClient: https://github.com/knolleary/pubsubclient
- Arduino ESP32: https://docs.espressif.com/projects/arduino-esp32/en/latest/
