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
// BIẾN ĐIỀU KHIỂN TRẠNG THÁI, TỐC ĐỘ & SỐ BƯỚC
// =============================================================================
int stepDelay = 800;              // Nửa chu kỳ xung (micro giây)
bool currentDir = HIGH;           // HIGH = Chiều thuận, LOW = Chiều ngược
bool isRunning = false;           // Trạng thái chạy/dừng của motor
bool isContinuousMode = false;    // true: quay liên tục, false: chạy theo số bước
long targetSteps = 0;             // Tổng số bước cần chạy
long remainingSteps = 0;          // Số bước còn lại cần chạy
long totalExecutedSteps = 0;      // Tổng số bước đã thực hiện từ lúc khởi động

void printHelp() {
  Serial.println("\n=======================================================");
  Serial.println(">>> BANG LENH DIEU KHIEN DONG CO BUOC ESP32-S3 <<<");
  Serial.println("=======================================================");
  Serial.println(" 1. Nhap SO hoac STEP <so> (vd: 1600, STEP 3200): Chay dung so buoc roi DUNG");
  Serial.println(" 2. SPEED <so> (vd: SPEED 500): Doi toc do quay (us)");
  Serial.println(" 3. F hoac THUAN  : Chon chieu quay THUAN (Forward)");
  Serial.println(" 4. R hoac NGUOC  : Chon chieu quay NGUOC (Reverse)");
  Serial.println(" 5. D hoac DAO    : Tu dong DAO CHIEU quay");
  Serial.println(" 6. CONT hoac RUN : Quay LIEN TUC khong dung");
  Serial.println(" 7. STOP hoac DUNG: DUNG KHAN CAP dong co");
  Serial.println(" 8. HELP hoac ?   : Xem lai bang huong dan nay");
  Serial.println("=======================================================\n");
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
        Serial.printf("[RUN] Bat dau chay %ld buoc | Toc do: %d us | Chieu: %s\n", 
                      targetSteps, stepDelay, currentDir ? "THUAN" : "NGUOC");
      } else {
        Serial.println("[LOI] So buoc phai lon hon 0! Vi du: STEP 1600");
      }
    } else {
      Serial.println("[HUONG DAN] Cu phap: STEP <so_buoc>. Vi du: STEP 1600");
    }
    return;
  }

  // Lệnh: SPEED <số us> hoặc SPD <số us>
  if (command == "SPEED" || command == "SPD" || command == "DELAY") {
    if (param.length() > 0) {
      int val = param.toInt();
      if (val >= 100 && val <= 20000) {
        stepDelay = val;
        Serial.printf("[OK] Da doi toc do: %d us (~%.1f xung/giay)\n", 
                      stepDelay, 1000000.0 / (stepDelay * 2));
      } else {
        Serial.println("[LOI] Toc do phai tu 100 us den 20000 us! Vi du: SPEED 500");
      }
    } else {
      Serial.println("[HUONG DAN] Cu phap: SPEED <so_us>. Vi du: SPEED 500");
    }
    return;
  }

  // Nếu người dùng nhập trực tiếp một con số thuần túy (ví dụ 1600):
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
      Serial.printf("[RUN] Nhan lenh chay %ld buoc (Tu dong dung sau khi chay xong)!\n", targetSteps);
      return;
    }
  }

  // Các lệnh đơn
  if (command == "F" || command == "THUAN" || command == "FORWARD" || command == "CW") {
    currentDir = HIGH;
    digitalWrite(DIR_PIN, currentDir);
    Serial.println("[OK] Da chuyen sang chieu: THUAN (Forward / HIGH)");
  }
  else if (command == "R" || command == "NGUOC" || command == "REVERSE" || command == "CCW") {
    currentDir = LOW;
    digitalWrite(DIR_PIN, currentDir);
    Serial.println("[OK] Da chuyen sang chieu: NGUOC (Reverse / LOW)");
  }
  else if (command == "D" || command == "DAO" || command == "TOGGLE") {
    currentDir = !currentDir;
    digitalWrite(DIR_PIN, currentDir);
    Serial.printf("[OK] Da dao chieu quay -> Hien tai: %s\n", currentDir ? "THUAN (HIGH)" : "NGUOC (LOW)");
  }
  else if (command == "CONT" || command == "RUN" || command == "START" || command == "CHAY" || command == "GO") {
    isContinuousMode = true;
    isRunning = true;
    Serial.printf("[RUN] Dong co quay LIEN TUC (Khong dung) | Toc do: %d us\n", stepDelay);
  }
  else if (command == "STOP" || command == "DUNG" || command == "PAUSE" || command == "HALT") {
    isRunning = false;
    isContinuousMode = false;
    remainingSteps = 0;
    Serial.println("[PAUSE] Dong co da DUNG (Truc van duoc khoa giu vi tri).");
  }
  else if (command == "HELP" || command == "?" || command == "MENU") {
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
  Serial.printf(">> Khoi dong san sang! Toc do = %d us | Chieu = THUAN\n", stepDelay);
  Serial.println(">> Nhap so buoc (vd: 1600 hoac STEP 1600) de bat dau quay:\n");
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

    totalExecutedSteps++;

    // Nếu đang ở chế độ chạy số bước cụ thể
    if (!isContinuousMode) {
      remainingSteps--;
      if (remainingSteps <= 0) {
        isRunning = false;
        Serial.printf("\n🎉 [HOAN THANH] Da quay dung %ld buoc! Dong co da tu dong dung.\n\n", targetSteps);
      }
    }
  } else {
    delay(10); // Nghỉ nhẹ khi tạm dừng
  }
}




