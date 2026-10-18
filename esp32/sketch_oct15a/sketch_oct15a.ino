#define RXp2 16
#define TXp2 17

void setup() {
  // Mở cổng Serial mặc định để in kết quả lên máy tính (Baudrate 115200)
  Serial.begin(115200);

  // Mở cổng Serial2 để giao tiếp với STM32 (Baudrate 115200, 8 bit data, no parity, 1 stop bit)
  Serial2.begin(115200, SERIAL_8N1, RXp2, TXp2);
  
  Serial.println("ESP32 dang cho du lieu tu STM32...");
}

void loop() {
  // Kiểm tra xem có dữ liệu gửi tới từ STM32 ở bộ đệm hay không
  if (Serial2.available()) {
    
    // Đọc trọn vẹn chuỗi cho đến khi gặp ký tự xuống dòng '\n'
    String incomingData = Serial2.readStringUntil('\n');
    
    // In chuỗi JSON nhận được lên Serial Monitor
    Serial.print("Nhan duoc: ");
    Serial.println(incomingData);
  }
}
