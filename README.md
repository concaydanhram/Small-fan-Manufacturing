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

## 11. Cài đặt môi trường Python cho Backend Server

Python Backend chịu trách nhiệm nhận dữ liệu từ MQTT Broker, phân tích bản tin JSON và lưu dữ liệu cảm biến vào InfluxDB 3 Core.

### 11.1. Yêu cầu hệ thống

- Python 3.11 hoặc phiên bản tương thích.
- Mosquitto MQTT Broker đang chạy.
- InfluxDB 3 Core đang chạy.
- ESP32 có thể publish dữ liệu lên topic `fan_data`.

### 11.2. Cấu trúc thư mục

```text
server/
├── .venv/          # Môi trường Python ảo
├── main.py         # Nhận MQTT và ghi dữ liệu vào InfluxDB
├── check_db.py     # Truy vấn và kiểm tra dữ liệu
├── .env            # Cấu hình môi trường cục bộ
├── .gitignore      # Các tệp không đưa lên Git
└── requirements.txt # Danh sách thư viện Python
```

Thư mục `.venv` chứa các thư viện của môi trường Python ảo và không nên được đưa lên GitHub.

### 11.3. Tạo môi trường Python ảo

Mở PowerShell và di chuyển đến thư mục server:

```powershell
cd D:\prj3\server
```

Tạo môi trường Python ảo bằng Python 3.11:

```powershell
py -3.11 -m venv .venv
```

Kích hoạt môi trường:

```powershell
.\.venv\Scripts\Activate.ps1
```

Nếu PowerShell chặn việc chạy script, có thể cho phép trong phiên terminal hiện tại nếu chính sách bảo mật của máy cho phép:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

Sau đó kích hoạt lại môi trường.

Kiểm tra phiên bản Python:

```powershell
python --version
python -m pip --version
```

### 11.4. Cài đặt các thư viện cần thiết

Cập nhật pip:

```powershell
python -m pip install --upgrade pip
```

Cài đặt các thư viện:

```powershell
python -m pip install paho-mqtt influxdb3-python python-dotenv
```

Chức năng của từng thư viện:

| Thư viện | Chức năng |
|---|---|
| `paho-mqtt` | Kết nối MQTT Broker và nhận dữ liệu từ topic |
| `influxdb3-python` | Ghi và truy vấn dữ liệu trong InfluxDB 3 Core |
| `python-dotenv` | Đọc biến môi trường từ tệp `.env` |

Nếu repository đã có `requirements.txt`, có thể cài đặt toàn bộ thư viện bằng lệnh:

```powershell
python -m pip install -r requirements.txt
```

Để tạo hoặc cập nhật danh sách thư viện từ môi trường đang sử dụng:

```powershell
python -m pip freeze > requirements.txt
```

Nên kiểm tra nội dung tệp trước khi commit lên Git.

### 11.5. Cấu hình biến môi trường

Tạo tệp `.env` cục bộ trong thư mục `server/` và cấu hình các thông tin cần thiết để kết nối MQTT Broker và InfluxDB.

Các nhóm cấu hình bao gồm:

- Địa chỉ và cổng MQTT Broker.
- Tên topic nhận dữ liệu.
- Địa chỉ máy chủ InfluxDB.
- Tên database.
- Token xác thực InfluxDB.

Lưu ý:

- Nếu Python và Mosquitto chạy trên cùng máy tính Windows, có thể sử dụng `127.0.0.1` làm địa chỉ MQTT Broker.
- Nếu kết nối từ thiết bị khác trong mạng LAN, sử dụng địa chỉ IPv4 thực tế của máy tính chạy Mosquitto.
- Nếu Explorer chạy trong Docker và cần truy cập dịch vụ trên Windows, có thể sử dụng `host.docker.internal` thay cho `127.0.0.1`.
- Không đưa token xác thực hoặc mật khẩu thật lên repository công khai.

### 11.6. Cấu hình `.gitignore`

Thêm các dòng sau vào tệp `.gitignore`:

```gitignore
.env
.venv/
__pycache__/
*.py[cod]
```

Các quy tắc này giúp tránh đưa tệp cấu hình chứa thông tin nhạy cảm, môi trường Python ảo và các tệp cache lên Git.

## 12. Cài đặt và cấu hình InfluxDB 3 Core

InfluxDB 3 Core được sử dụng để lưu trữ dữ liệu cảm biến theo chuỗi thời gian.

### 12.1. Khởi động InfluxDB

Ví dụ, nếu chương trình InfluxDB được cài tại `D:\influxDB\influxdb3.exe`, có thể khởi động bằng PowerShell:

```powershell
& "D:\influxDB\influxdb3.exe" serve --plugin-dir "D:\influxDB\plugins"
```

Giữ nguyên các tham số và thư mục dữ liệu đang sử dụng trong hệ thống hiện tại. Không tự ý đổi thư mục dữ liệu nếu chưa xác định vị trí lưu database hiện có.

Giữ cửa sổ terminal mở trong khi sử dụng hệ thống.

### 12.2. Cấu hình database và bảng dữ liệu

Hệ thống sử dụng:

- Database: `fan_monitor`
- Bảng dữ liệu: `fan_telemetry`

Nếu database chưa tồn tại, hãy tạo database bằng công cụ dòng lệnh của InfluxDB 3 Core và sử dụng token phù hợp.

Bảng `fan_telemetry` sẽ được tạo tự động khi có bản ghi đầu tiên được ghi thành công. Không cần tạo bảng thủ công.

### 12.3. Cấu trúc dữ liệu

Mỗi bản tin JSON hợp lệ từ ESP32 được Python chuyển thành một bản ghi trong InfluxDB.

| Trường | Loại dữ liệu | Ý nghĩa |
|---|---|---|
| `time` | Timestamp | Thời điểm ghi nhận dữ liệu |
| `dev` | Tag | Định danh thiết bị |
| `freq` | Số | Tần số đo được |
| `amp` | Số | Biên độ rung |
| `current` | Số | Dòng điện đo được |

Cần đảm bảo đơn vị của các giá trị đo phù hợp với chương trình STM32.

## 13. Khởi chạy Backend Server

Trước khi chạy Python Backend, cần đảm bảo Mosquitto và InfluxDB đã khởi động thành công.

### 13.1. Chạy chương trình nhận dữ liệu MQTT

Mở PowerShell:

```powershell
cd D:\prj3\server
.\.venv\Scripts\Activate.ps1
python main.py
```

Kết quả mong đợi:

```text
Connecting to MQTT broker ...
[MQTT] Connected to ...
[MQTT] Subscribed to topic: fan_data
```

Chương trình sẽ tiếp tục lắng nghe bản tin MQTT cho đến khi người dùng dừng tiến trình.

### 13.2. Kiểm tra dữ liệu nhận được

Khi ESP32 publish một bản tin JSON hợp lệ, Python sẽ phân tích dữ liệu và ghi vào InfluxDB.

Ví dụ bản tin:

```json
{"dev":"fan1","freq":50.0,"amp":0.8,"current":0.35}
```

Các giá trị trên chỉ là dữ liệu minh họa.

Nếu ghi thành công, terminal sẽ hiển thị tương tự:

```text
[DB] Saved | dev=fan1, freq=50.0, amp=0.8, current=0.35
```

Kết nối MQTT thành công không đồng nghĩa với việc ghi database thành công. Cần kiểm tra log để xác nhận dữ liệu đã được xử lý và lưu.

## 14. Kiểm tra dữ liệu đã lưu

### 14.1. Kiểm tra bằng Python

Mở một terminal PowerShell khác:

```powershell
cd D:\prj3\server
.\.venv\Scripts\python.exe check_db.py
```

Chương trình sẽ truy vấn tối đa 10 bản ghi mới nhất trong bảng `fan_telemetry`.

### 14.2. Kiểm tra bằng InfluxDB Explorer

InfluxDB Explorer cung cấp giao diện trực quan để truy vấn dữ liệu trong InfluxDB 3 Core.

Tài liệu cài đặt chính thức:

https://docs.influxdata.com/influxdb3/explorer/install/docker/

Sau khi cấu hình kết nối đến InfluxDB và chọn database `fan_monitor`, chạy:

```sql
SHOW TABLES;
```

Nếu bảng `fan_telemetry` đã xuất hiện, truy vấn dữ liệu:

```sql
SELECT *
FROM fan_telemetry
ORDER BY time DESC
LIMIT 20;
```

Nếu không tìm thấy bảng, hãy kiểm tra log Python để xác nhận có bản ghi nào được ghi thành công hay chưa. Đồng thời, xác minh Explorer đang kết nối đúng máy chủ và database.

## 15. Xử lý lỗi thường gặp

| Lỗi | Nguyên nhân có thể | Cách xử lý |
|---|---|---|
| `Missing INFLUXDB_TOKEN` | Chưa đọc được biến môi trường | Kiểm tra cấu hình cục bộ và vị trí tệp `.env` |
| Không kết nối được MQTT | Sai địa chỉ hoặc broker chưa chạy | Kiểm tra địa chỉ IP, cổng và tiến trình Mosquitto |
| `DATA ERROR` | Bản tin không đúng cú pháp JSON | Kiểm tra cách STM32 hoặc ESP32 tạo bản tin |
| `DB ERROR` | Ghi dữ liệu vào InfluxDB thất bại | Kiểm tra thông báo lỗi, địa chỉ, database và token |
| `table not found` | Chưa ghi thành công hoặc kết nối sai database | Kiểm tra log và cấu hình kết nối |
| Explorer không kết nối được | Sai endpoint hoặc cấu hình mạng Docker | Kiểm tra địa chỉ InfluxDB và thông tin xác thực |
| Không thấy dữ liệu | Chưa ghi thành công hoặc truy vấn sai database | Kiểm tra log `[DB] Saved` và câu SQL |

### Lỗi định dạng JSON

Bản tin không hợp lệ:

```text
{dev:fan1,freq:50.0,amp:0.8,current:0.35}
```

Bản tin hợp lệ:

```json
{"dev":"fan1","freq":50.0,"amp":0.8,"current":0.35}
```

Trong JSON, tên trường và giá trị dạng chuỗi phải được đặt trong dấu ngoặc kép.

## 16. Luồng hoạt động tổng thể

```text
STM32
  |
  | UART / JSON
  v
ESP32 Gateway
  |
  | Wi-Fi / MQTT
  v
Mosquitto MQTT Broker
  |
  | Topic: fan_data
  v
Python Backend (main.py)
  |
  | Phân tích và kiểm tra JSON
  v
InfluxDB 3 Core
  |
  | Truy vấn SQL
  v
InfluxDB Explorer / check_db.py
```

Hệ thống được xem là hoạt động đúng khi:

1. ESP32 gửi được dữ liệu cảm biến lên MQTT Broker.
2. Python nhận và phân tích được bản tin JSON.
3. InfluxDB ghi nhận thành công dữ liệu.
4. Có thể truy vấn lại các bản ghi đã lưu.

## 17. Tài liệu tham khảo

- Python Virtual Environments: https://docs.python.org/3/library/venv.html
- Paho MQTT Python Client: https://eclipse.dev/paho/files/paho.mqtt.python/html/
- InfluxDB 3 Core: https://docs.influxdata.com/influxdb3/core/
- InfluxDB 3 Python Client: https://github.com/InfluxCommunity/influxdb3-python
- InfluxDB 3 Explorer: https://docs.influxdata.com/influxdb3/explorer/install/docker/

