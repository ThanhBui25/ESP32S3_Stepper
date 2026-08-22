#include <Arduino.h>

// =============================================================================
// CẤU HÌNH CHÂN GPIO (KIỂU ĐẤU CỰC ÂM CHUNG - COMMON CATHODE)
// =============================================================================
// ESP32-S3 GPIO -> Driver Dương (+):
// GPIO 4  -> PUL+ (Xung bước)
// GPIO 5  -> DIR+ (Chiều quay)
// GPIO 6  -> ENA+ (Kích hoạt Driver)
// Chân GND ESP32 -> Nối chung vào PUL-, DIR-, ENA-
// =============================================================================
const int STEP_PIN = 4; // Chân xung PUL+ / STEP+
const int DIR_PIN  = 5; // Chân chiều DIR+
const int ENA_PIN  = 6; // Chân kích hoạt ENA+

// =============================================================================
// THỜI GIAN TRỄ (TỐC ĐỘ)
// =============================================================================
// 1000us - 2000us là tốc độ chuẩn an toàn giúp động cơ khởi động êm, không bị khựng/kẹt
int stepDelay = 1500; // microseconds (Nửa chu kỳ xung)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================");
  Serial.println("ESP32-S3 Stepper Motor (Common Cathode)...");
  Serial.println("=================================");

  // Cấu hình chân Output
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  // Cấu hình LED báo trạng thái
  pinMode(2, OUTPUT);
  pinMode(48, OUTPUT);

  // Thiết lập mức ban đầu:
  // 1. Chân DIR: HIGH hoặc LOW để chọn chiều quay (HIGH = một chiều, LOW = chiều ngược lại)
  digitalWrite(DIR_PIN, HIGH);

  // 2. Chân ENA: 
  // Đối với kiểu Cực Âm Chung (Common Cathode):
  // - Xuất LOW: Opto ENA tắt -> Driver được BẬT (Enable/Giữ trục).
  // - Xuất HIGH: Opto ENA sáng -> Driver bị TẮT (Disable/Thả tự do).
  digitalWrite(ENA_PIN, LOW);
}

void loop() {
  // Phát xung điều khiển bước (Kéo HIGH để kích opto PUL+, sau đó kéo LOW)
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(stepDelay);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(stepDelay);
}

