#include <Arduino.h>

// =============================================================================
// CẤU HÌNH CHÂN GPIO
// =============================================================================
const int STEP_PIN = 4; // Chân xung PUL / STEP
const int DIR_PIN  = 5; // Chân chiều DIR
const int ENA_PIN  = 6; // Chân kích hoạt ENA

// =============================================================================
// THỜI GIAN TRỄ (TỐC ĐỘ)
// =============================================================================
// 1000us - 2000us là tốc độ chuẩn an toàn giúp động cơ khởi động êm, không bị khựng/kẹt
int stepDelay = 1500; // microseconds (Nửa chu kỳ xung)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================");
  Serial.println("ESP32-S3 Stepper Motor Running...");
  Serial.println("=================================");

  // Cấu hình chân Output
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  // Cấu hình LED báo trạng thái
  pinMode(2, OUTPUT);
  pinMode(48, OUTPUT);

  // Thiết lập mức ban đầu:
  // 1. Chân DIR: HIGH hoặc LOW để chọn chiều quay
  digitalWrite(DIR_PIN, HIGH);

  // 2. Chân ENA: 
  // Đối với driver DM542/TB6600 kiểu Chung Dương (+ nối 3.3V/5V), 
  // xuất HIGH để tắt opto ENA -> Driver được BẬT (Enable).
  digitalWrite(ENA_PIN, HIGH);
}

void loop() {
  // Phát xung điều khiển bước
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(stepDelay);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(stepDelay);
}
