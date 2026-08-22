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
// BIẾN ĐIỀU KHIỂN TRẠNG THÁI & TỐC ĐỘ
// =============================================================================
int stepDelay = 800;          // Nửa chu kỳ xung (micro giây)
bool currentDir = HIGH;       // HIGH = Chiều thuận, LOW = Chiều ngược
bool isRunning = true;        // Trạng thái chạy/dừng của motor

void printHelp() {
  Serial.println("\n=======================================================");
  Serial.println(">>> BANG LENH DIEU KHIEN DONG CO BUOC ESP32-S3 <<<");
  Serial.println("=======================================================");
  Serial.println(" 1. Nhap SO (vd: 500, 800, 1500, 3000): Doi toc do (us)");
  Serial.println(" 2. F hoac THUAN  : Quay chieu THUAN (Forward)");
  Serial.println(" 3. R hoac NGUOC  : Quay chieu NGUOC (Reverse)");
  Serial.println(" 4. D hoac DAO    : Tu dong DAO CHIEU quay");
  Serial.println(" 5. STOP hoac DUNG: TAM DUNG dong co");
  Serial.println(" 6. RUN hoac CHAY : TIEP TUC quay dong co");
  Serial.println(" 7. HELP hoac ?   : Xem lai bang huong dan nay");
  Serial.println("=======================================================\n");
}

void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  // Kiểm tra nếu người dùng nhập số để đổi tốc độ
  bool isNumber = true;
  for (unsigned int i = 0; i < cmd.length(); i++) {
    if (!isDigit(cmd.charAt(i))) {
      isNumber = false;
      break;
    }
  }

  if (isNumber) {
    int val = cmd.toInt();
    if (val >= 100 && val <= 20000) {
      stepDelay = val;
      Serial.printf("[OK] Da cap nhat toc do moi: %d us (~%.1f xung/giay)\n", stepDelay, 1000000.0 / (stepDelay * 2));
    } else {
      Serial.println("[LOI] Gia tri toc do khong hop le! Vui long nhap tu 100 us den 20000 us.");
    }
    return;
  }

  // Chuyển lệnh sang chữ in hoa để so sánh
  cmd.toUpperCase();

  if (cmd == "F" || cmd == "THUAN" || cmd == "FORWARD" || cmd == "CW") {
    currentDir = HIGH;
    digitalWrite(DIR_PIN, currentDir);
    Serial.println("[OK] Da chuyen sang chieu: THUAN (Forward / HIGH)");
  }
  else if (cmd == "R" || cmd == "NGUOC" || cmd == "REVERSE" || cmd == "CCW") {
    currentDir = LOW;
    digitalWrite(DIR_PIN, currentDir);
    Serial.println("[OK] Da chuyen sang chieu: NGUOC (Reverse / LOW)");
  }
  else if (cmd == "D" || cmd == "DAO" || cmd == "TOGGLE") {
    currentDir = !currentDir;
    digitalWrite(DIR_PIN, currentDir);
    Serial.printf("[OK] Da dao chieu quay -> Hien tai: %s\n", currentDir ? "THUAN (HIGH)" : "NGUOC (LOW)");
  }
  else if (cmd == "STOP" || cmd == "DUNG" || cmd == "PAUSE") {
    isRunning = false;
    Serial.println("[PAUSE] Dong co da TAM DUNG (Truc van duoc khoa giu vi tri).");
  }
  else if (cmd == "RUN" || cmd == "START" || cmd == "CHAY" || cmd == "GO") {
    isRunning = true;
    Serial.println("[RUN] Dong co TIEP TUC quay.");
  }
  else if (cmd == "HELP" || cmd == "?" || cmd == "MENU") {
    printHelp();
  }
  else {
    Serial.printf("[?] Khong nhan dien duoc lenh: '%s'. Go 'HELP' de xem huong dan.\n", cmd.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Cấu hình chân Output
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  // Cấu hình LED báo trạng thái trên mạch
  pinMode(2, OUTPUT);
  pinMode(48, OUTPUT);

  // Thiết lập trạng thái ban đầu:
  digitalWrite(DIR_PIN, currentDir);
  digitalWrite(ENA_PIN, HIGH); // Common Anode: HIGH = Bật driver

  printHelp();
  Serial.printf(">> Trang thai khoi dong: Toc do = %d us | Chieu = THUAN | Dang chay = BAT DAU\n\n", stepDelay);
}

void loop() {
  // Kiểm tra nếu có lệnh từ Serial Monitor
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    processCommand(input);
  }

  // Phát xung bước nếu động cơ đang ở trạng thái chạy
  if (isRunning) {
    digitalWrite(STEP_PIN, LOW);   // Active-LOW (Common Anode)
    delayMicroseconds(stepDelay);
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay);
  } else {
    delay(10); // Nghỉ nhẹ khi tạm dừng
  }
}



