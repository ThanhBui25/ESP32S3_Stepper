#include <Arduino.h>

// =============================================================================
// CẤU HÌNH CHÂN GPIO (KIỂU ĐẤU CỰC DƯƠNG CHUNG - COMMON ANODE)
// =============================================================================
// 1. Chân 3.3V của ESP32-S3 -> Nối chung vào 3 chân: PUL+ , DIR+ , ENA+
// 2. Chân GPIO của ESP32-S3  -> Nối vào các chân âm của Driver:
//    - GPIO 4 -> PUL- (Phát xung bước)
//    - GPIO 5 -> DIR- (Chọn chiều quay)
//    - GPIO 6 -> ENA- (Kích hoạt Driver - hoặc bỏ trống không cần cắm)
// =============================================================================
const int STEP_PIN = 4; // Nối vào PUL-
const int DIR_PIN  = 5; // Nối vào DIR-
const int ENA_PIN  = 6; // Nối vào ENA-

// =============================================================================
// THỜI GIAN TRỄ (TỐC ĐỘ)
// =============================================================================
// 1000us - 2000us là tốc độ chuẩn an toàn giúp động cơ khởi động êm, không bị khựng/kẹt
int stepDelay = 1500; // microseconds (Nửa chu kỳ xung)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================");
  Serial.println("ESP32-S3 Stepper Motor (Common Anode)...");
  Serial.println("=================================");

  // Cấu hình chân Output
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  // Cấu hình LED báo trạng thái trên mạch
  pinMode(2, OUTPUT);
  pinMode(48, OUTPUT);

  // Thiết lập trạng thái ban đầu:
  // 1. Chân DIR: HIGH hoặc LOW để chọn chiều quay
  digitalWrite(DIR_PIN, HIGH);

  // 2. Chân ENA: 
  // Đối với kiểu Cực Dương Chung (Common Anode):
  // - Xuất HIGH (3.3V - 3.3V = 0V): Opto ENA tắt -> Driver được BẬT (Enable/Khóa trục motor).
  // - Xuất LOW  (3.3V - 0V = 3.3V): Opto ENA sáng -> Driver bị TẮT (Disable/Thả tự do motor).
  digitalWrite(ENA_PIN, HIGH);
}

void loop() {
  // Phát xung bước (Active-LOW: Kéo LOW để kích sáng Opto PUL-, sau đó kéo HIGH để tắt)
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(stepDelay);
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(stepDelay);
}


