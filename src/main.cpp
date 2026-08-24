#include <Arduino.h>

// =============================================================================
// CẤU HÌNH CHÂN GPIO 1 ĐỘNG CƠ (KIỂU ĐẤU CỰC DƯƠNG CHUNG - COMMON ANODE)
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

// Chân RGB LED tích hợp trên ESP32-S3 DevKitC-1
#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48
#endif

// =============================================================================
// BIẾN ĐIỀU KHIỂN TRẠNG THÁI, TỐC ĐỘ & SỐ BƯỚC (NON-BLOCKING)
// =============================================================================
int speedHz = 1600;               // Tốc độ phát xung trực tiếp: XUNG TRÊN GIÂY (Hz)
unsigned long stepHalfPeriodUs = 312; // Nửa chu kỳ xung tính toán (us)
bool currentDir = HIGH;           // HIGH = Chiều thuận, LOW = Chiều ngược
bool isRunning = false;           // Trạng thái chạy / dừng của motor
bool isContinuousMode = false;    // true: quay liên tục, false: chạy theo số bước
long targetSteps = 0;             // Tổng số bước cần chạy
long remainingSteps = 0;          // Số bước còn lại cần chạy
long totalExecutedSteps = 0;      // Tổng số bước đã thực hiện từ lúc khởi động

// Biến điều khiển phát xung thời gian thực (Non-blocking)
unsigned long lastPulseUs = 0;
bool pulsePinState = HIGH;        // HIGH = Inactive, LOW = Active (Common Anode)
String inputBuffer = "";          // Bộ đệm nhận lệnh Serial

// Hàm đổi màu LED RGB tích hợp
void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_BUILTIN, r, g, b);
  neopixelWrite(48, r, g, b);
  neopixelWrite(38, r, g, b);
}

void updateSpeed(int hz) {
  if (hz >= 10 && hz <= 5000) {
    speedHz = hz;
    stepHalfPeriodUs = 1000000UL / (2 * (unsigned long)speedHz);
    if (stepHalfPeriodUs < 100) stepHalfPeriodUs = 100;
    setLedColor(0, 150, 255); // Xanh lam
    Serial.printf("[OK] Da dat toc do: %d XUNG/GIAY (Hz) [halfPeriod = %lu us]\n", speedHz, stepHalfPeriodUs);
  } else {
    setLedColor(255, 0, 0); // Đỏ báo lỗi
    Serial.println("[LOI] Toc do phai tu 10 den 5000 xung/giay! Vi du: SPEED 1600");
  }
}

void printHelp() {
  setLedColor(200, 200, 200); // Trắng
  Serial.println("\n===================================================================");
  Serial.println(">>> BANG LENH DIEU KHIEN 1 DONG CO BUOC ESP32-S3 (NON-BLOCKING) <<<");
  Serial.println("===================================================================");
  Serial.println(" 1. Nhap SO (vd: 1600, 3200 hoac STEP 1600): Chay du buoc roi DUNG [LED CYAN/TIM]");
  Serial.println(" 2. SPEED <xung/giay> (vd: SPEED 1600, SPEED 800)  : Doi toc do    [LED XANH LAM]");
  Serial.println(" 3. F hoac THUAN  : Chon chieu quay THUAN (Forward)                 [LED VANG]");
  Serial.println(" 4. R hoac NGUOC  : Chon chieu quay NGUOC (Reverse)                 [LED CAM]");
  Serial.println(" 5. D hoac DAO    : Tu dong DAO CHIEU quay                          [LED TRANG]");
  Serial.println(" 6. CONT hoac RUN : Quay LIEN TUC khong dung                        [LED XANH LA]");
  Serial.println(" 7. STOP hoac DUNG: DUNG KHAN CAP dong co                           [LED DO]");
  Serial.println(" 8. HELP hoac ?   : Xem lai bang huong dan nay                      [LED TRANG]");
  Serial.println("===================================================================\n");
}

void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  // Tách lệnh và tham số (nếu có)
  String command = cmd;
  String param = "";
  int spaceIdx = cmd.indexOf(' ');
  if (spaceIdx != -1) {
    command = cmd.substring(0, spaceIdx);
    param = cmd.substring(spaceIdx + 1);
    param.trim();
  }
  command.toUpperCase();

  // Lệnh: STEP <số bước> hoặc S <số bước>
  if (command == "STEP" || command == "MOVE" || command == "S") {
    if (param.length() > 0) {
      long steps = param.toInt();
      if (steps > 0) {
        targetSteps = steps;
        remainingSteps = steps;
        isContinuousMode = false;
        isRunning = true;
        pulsePinState = HIGH;
        digitalWrite(STEP_PIN, HIGH);
        lastPulseUs = micros();
        setLedColor(0, 255, 255); // Xanh ngọc
        Serial.printf("[RUN] Bat dau chay %ld buoc | Toc do: %d xung/s | Chieu: %s (LED: Xanh ngoc)\n", 
                      targetSteps, speedHz, currentDir ? "THUAN" : "NGUOC");
      } else {
        setLedColor(255, 0, 0);
        Serial.println("[LOI] So buoc phai lon hon 0! Vi du: STEP 1600");
      }
    } else {
      Serial.println("[HUONG DAN] Cu phap: STEP <so_buoc>. Vi du: STEP 1600");
    }
    return;
  }

  // Lệnh: SPEED <xung/giây> hoặc SPD <xung/giây> hoặc HZ <xung/giây>
  if (command == "SPEED" || command == "SPD" || command == "HZ") {
    if (param.length() > 0) {
      int val = param.toInt();
      updateSpeed(val);
    } else {
      Serial.println("[HUONG DAN] Cu phap: SPEED <xung_tren_giay>. Vi du: SPEED 1600");
    }
    return;
  }

  // Nếu người dùng nhập trực tiếp một con số thuần túy (ví dụ 1600, 3200):
  bool isAllDigits = true;
  for (unsigned int i = 0; i < cmd.length(); i++) {
    if (!isDigit(cmd.charAt(i))) {
      isAllDigits = false;
      break;
    }
  }

  if (isAllDigits) {
    long steps = cmd.toInt();
    if (steps > 0) {
      targetSteps = steps;
      remainingSteps = steps;
      isContinuousMode = false;
      isRunning = true;
      pulsePinState = HIGH;
      digitalWrite(STEP_PIN, HIGH);
      lastPulseUs = micros();
      setLedColor(0, 255, 255); // Xanh ngọc
      Serial.printf("[RUN] Nhan lenh chay %ld buoc (Toc do: %d xung/giay | LED: Xanh ngoc)!\n", targetSteps, speedHz);
      return;
    }
  }

  // Các lệnh đơn
  if (command == "F" || command == "THUAN" || command == "FORWARD" || command == "CW") {
    currentDir = HIGH;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(255, 200, 0); // Vàng
    Serial.printf("[OK] Da chuyen sang chieu: THUAN (Forward / HIGH)%s\n", 
                  isRunning ? " (Dong co van tiep tuc quay)" : "");
  }
  else if (command == "R" || command == "NGUOC" || command == "REVERSE" || command == "CCW") {
    currentDir = LOW;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(255, 80, 0); // Cam
    Serial.printf("[OK] Da chuyen sang chieu: NGUOC (Reverse / LOW)%s\n", 
                  isRunning ? " (Dong co van tiep tuc quay)" : "");
  }
  else if (command == "D" || command == "DAO" || command == "TOGGLE") {
    currentDir = !currentDir;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(255, 255, 255); // Trắng
    Serial.printf("[OK] Da dao chieu quay -> Hien tai: %s%s\n", 
                  currentDir ? "THUAN (HIGH)" : "NGUOC (LOW)",
                  isRunning ? " (Dong co van tiep tuc quay)" : "");
  }
  else if (command == "CONT" || command == "RUN" || command == "START" || command == "CHAY" || command == "GO") {
    isContinuousMode = true;
    isRunning = true;
    pulsePinState = HIGH;
    digitalWrite(STEP_PIN, HIGH);
    lastPulseUs = micros();
    setLedColor(0, 255, 0); // Xanh lá
    Serial.printf("[RUN] Dong co quay LIEN TUC (Khong dung) | Toc do: %d XUNG/GIAY (LED: Xanh la)\n", speedHz);
  }
  else if (command == "STOP" || command == "DUNG" || command == "PAUSE" || command == "HALT") {
    isRunning = false;
    isContinuousMode = false;
    remainingSteps = 0;
    pulsePinState = HIGH;
    digitalWrite(STEP_PIN, HIGH); // Tắt xung kích
    setLedColor(255, 0, 0); // Đỏ
    Serial.println("[PAUSE] Dong co da DUNG (Truc van duoc khoa giu vi tri) (LED: Do).");
  }
  else if (command == "HELP" || command == "?" || command == "MENU") {
    printHelp();
  }
  else {
    setLedColor(255, 0, 0); // Đỏ
    Serial.printf("[?] Khong nhan dien duoc lenh: '%s'. Go 'HELP' de xem huong dan.\n", cmd.c_str());
  }
}

void checkSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      if (inputBuffer.length() < 64) {
        inputBuffer += c;
      }
    }
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
  digitalWrite(STEP_PIN, HIGH); // Common Anode: HIGH là không kích xung
  digitalWrite(DIR_PIN, currentDir);
  digitalWrite(ENA_PIN, HIGH);  // Common Anode: HIGH = Bật driver

  // Đổi màu LED sang Xanh dương dịu báo hiệu sẵn sàng
  setLedColor(0, 0, 150);

  printHelp();
  Serial.printf(">> Khoi dong san sang! 1 Dong co (PUL=%d, DIR=%d, ENA=%d) | Toc do = %d XUNG/GIAY\n", 
                STEP_PIN, DIR_PIN, ENA_PIN, speedHz);
  Serial.println(">> Nhap so buoc (vd: 1600 hoac RUN) de bat dau quay:\n");
}

void loop() {
  // 1. Luôn kiểm tra lệnh Serial liên tục không bị chặn
  checkSerial();

  // 2. Phát xung Non-Blocking bằng hàm micros() mượt mà và không treo CPU
  if (isRunning) {
    unsigned long nowUs = micros();
    if (nowUs - lastPulseUs >= stepHalfPeriodUs) {
      lastPulseUs = nowUs;

      if (pulsePinState == HIGH) {
        // Kéo xuống LOW để kích hoạt optocoupler phát xung
        digitalWrite(STEP_PIN, LOW);
        pulsePinState = LOW;
      } else {
        // Kéo về HIGH để kết thúc xung
        digitalWrite(STEP_PIN, HIGH);
        pulsePinState = HIGH;
        totalExecutedSteps++;

        // Nếu đang chạy số bước cụ thể
        if (!isContinuousMode) {
          remainingSteps--;
          if (remainingSteps <= 0) {
            isRunning = false;
            setLedColor(255, 0, 255); // Màu Tím khi hoàn thành
            Serial.printf("\n🎉 [HOAN THANH] Da quay dung %ld buoc! Dong co da tu dong dung. (LED: Tim)\n\n", targetSteps);
          }
        }
      }
    }
  }
}
