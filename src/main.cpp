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

// =============================================================================
// CẤU HÌNH CHÂN MODULE BÀN PHÍM & MÀN HÌNH TM1638 (LED & KEY)
// =============================================================================
// Header J1 trên TM1638:
// - VCC -> 3.3V (hoặc 5V) của ESP32-S3
// - GND -> GND của ESP32-S3
// - STB -> GPIO 10
// - CLK -> GPIO 11
// - DIO -> GPIO 12
// =============================================================================
const int TM_STB_PIN = 10;
const int TM_CLK_PIN = 11;
const int TM_DIO_PIN = 12;

// Chân RGB LED tích hợp trên ESP32-S3 DevKitC-1
#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48
#endif

// =============================================================================
// BIẾN ĐIỀU KHIỂN TRẠNG THÁI, TỐC ĐỘ & SỐ BƯỚC (NON-BLOCKING)
// =============================================================================
int speedHz = 1600;                   // Tốc độ phát xung trực tiếp: XUNG TRÊN GIÂY (Hz)
unsigned long stepHalfPeriodUs = 312; // Nửa chu kỳ xung tính toán (us)
bool currentDir = HIGH;               // HIGH = Chiều thuận, LOW = Chiều ngược
bool isRunning = false;               // Trạng thái chạy / dừng của motor
bool isContinuousMode = false;        // true: quay liên tục, false: chạy theo số bước
long targetSteps = 0;                 // Tổng số bước cần chạy
long remainingSteps = 0;              // Số bước còn lại cần chạy
long totalExecutedSteps = 0;          // Tổng số bước đã thực hiện từ lúc khởi động

// Biến điều khiển phát xung thời gian thực (Non-blocking)
unsigned long lastPulseUs = 0;
bool pulsePinState = HIGH;            // HIGH = Inactive, LOW = Active (Common Anode)
String inputBuffer = "";              // Bộ đệm nhận lệnh Serial

// Biến thời gian cập nhật hiển thị TM1638 & đọc phím
unsigned long lastTmUpdateMs = 0;
uint8_t lastButtonsState = 0;
uint8_t currentActiveLed = 8; // 1: D1, 2: D2, 3: D3, 4: D4, 5: D5, 6: D6, 7: D7, 8: D8 (STOP)
unsigned long tempLedTimerMs = 0;

// =============================================================================
// TRÌNH ĐIỀU KHIỂN MODULE TM1638 (NATIVE - KHÔNG CẦN THƯ VIỆN NGOÀI)
// =============================================================================

// Bảng mã 7 thanh cho các ký tự thông dụng
const uint8_t SEG_DIGITS[] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F  // 9
};

const uint8_t SEG_BLANK = 0x00;
const uint8_t SEG_DASH  = 0x40;
const uint8_t SEG_S     = 0x6D;
const uint8_t SEG_P     = 0x73;
const uint8_t SEG_d     = 0x5E;
const uint8_t SEG_r     = 0x50;
const uint8_t SEG_u     = 0x1C;
const uint8_t SEG_n     = 0x54;
const uint8_t SEG_F     = 0x71;
const uint8_t SEG_t     = 0x78;
const uint8_t SEG_O     = 0x3F;
const uint8_t SEG_E     = 0x79;

void tm_sendByte(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(TM_CLK_PIN, LOW);
    digitalWrite(TM_DIO_PIN, (data & (1 << i)) ? HIGH : LOW);
    digitalWrite(TM_CLK_PIN, HIGH);
  }
}

void tm_sendCommand(uint8_t cmd) {
  digitalWrite(TM_STB_PIN, LOW);
  tm_sendByte(cmd);
  digitalWrite(TM_STB_PIN, HIGH);
}

void tm_init() {
  pinMode(TM_STB_PIN, OUTPUT);
  pinMode(TM_CLK_PIN, OUTPUT);
  pinMode(TM_DIO_PIN, OUTPUT);
  digitalWrite(TM_STB_PIN, HIGH);
  digitalWrite(TM_CLK_PIN, HIGH);

  // Bật hiển thị, độ sáng mức 4 (0x88 | độ sáng 0-7)
  tm_sendCommand(0x8C);
}

// Ghi đồng thời 8 số LED 7 đoạn và 8 đèn LED đỏ
void tm_updateDisplay(uint8_t digits[8], uint8_t ledsMask) {
  tm_sendCommand(0x40); // Chế độ ghi địa chỉ tự tăng
  digitalWrite(TM_STB_PIN, LOW);
  tm_sendByte(0xC0);    // Bắt đầu từ địa chỉ 0x00

  for (int i = 0; i < 8; i++) {
    tm_sendByte(digits[i]);                       // Byte chẵn: LED 7 đoạn thứ i
    tm_sendByte((ledsMask & (1 << i)) ? 0x01 : 0x00); // Byte lẻ: Đèn LED đỏ thứ i
  }
  digitalWrite(TM_STB_PIN, HIGH);
}

// Đọc trạng thái 8 nút nhấn S1 - S8 (trả về bitmask 8 bit)
uint8_t tm_readButtons() {
  digitalWrite(TM_STB_PIN, LOW);
  tm_sendByte(0x42); // Lệnh đọc phím quét

  pinMode(TM_DIO_PIN, INPUT_PULLUP);
  uint8_t keys = 0;

  for (int i = 0; i < 4; i++) {
    uint8_t byteIn = 0;
    for (int j = 0; j < 8; j++) {
      digitalWrite(TM_CLK_PIN, LOW);
      digitalWrite(TM_CLK_PIN, HIGH);
      if (digitalRead(TM_DIO_PIN)) {
        byteIn |= (1 << j);
      }
    }
    // S1, S2, S3, S4 nằm ở bit 0 của 4 byte
    if (byteIn & 0x01) keys |= (1 << i);
    // S5, S6, S7, S8 nằm ở bit 4 của 4 byte
    if (byteIn & 0x10) keys |= (1 << (i + 4));
  }

  pinMode(TM_DIO_PIN, OUTPUT);
  digitalWrite(TM_STB_PIN, HIGH);
  return keys;
}

// =============================================================================
// HÀM ĐỔI MÀU LED RGB & XỬ LÝ LỆNH
// =============================================================================

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
    setLedColor(255, 0, 0);
    Serial.println("[LOI] Toc do phai tu 10 den 5000 xung/giay! Vi du: SPEED 1600");
  }
}

void printHelp() {
  setLedColor(200, 200, 200); // Trắng
  Serial.println("\n=========================================================================");
  Serial.println(">>> BANG LENH DIEU KHIEN ESP32-S3 (SERIAL + PHIM BAM TM1638) <<<");
  Serial.println("=========================================================================");
  Serial.println(" A. DIEU KHIEN QUA SERIAL MONITOR:");
  Serial.println("   - Nhap SO (vd: 8000, 16000, STEP 8000): Chay du buoc roi DUNG [LED CYAN/TIM]");
  Serial.println("   - SPEED <xung/giay> (vd: SPEED 2000)   : Doi toc do quay       [LED XANH LAM]");
  Serial.println("   - F / THUAN                            : Quay THUAN            [LED VANG]");
  Serial.println("   - R / NGUOC                            : Quay NGUOC            [LED CAM]");
  Serial.println("   - D / DAO                              : DAO CHIEU quay        [LED TRANG]");
  Serial.println("   - CONT / RUN                           : Quay LIEN TUC         [LED XANH LA]");
  Serial.println("   - STOP / DUNG                          : DUNG KHAN CAP         [LED DO]");
  Serial.println("-------------------------------------------------------------------------");
  Serial.println(" B. DIEU KHIEN QUA 8 NUT BAM TREN MODULE TM1638:");
  Serial.println("   - [S1]: Quay 1 VONG (8000 buoc)   | [S5]: Tang toc (+200 Hz)");
  Serial.println("   - [S2]: Quay 2 VONG (16000 buoc)  | [S6]: Giam toc (-200 Hz)");
  Serial.println("   - [S3]: Quay 90 DO  (2000 buoc)   | [S7]: Quay LIEN TUC (RUN)");
  Serial.println("   - [S4]: DAO CHIEU (Thuận/Ngược)   | [S8]: DUNG KHAN CAP (STOP)");
  Serial.println("=========================================================================\n");
}

void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  String command = cmd;
  String param = "";
  int spaceIdx = cmd.indexOf(' ');
  if (spaceIdx != -1) {
    command = cmd.substring(0, spaceIdx);
    param = cmd.substring(spaceIdx + 1);
    param.trim();
  }
  command.toUpperCase();

  // Lệnh: STEP <số bước>
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
        setLedColor(0, 255, 255);
        Serial.printf("[RUN] Bat dau chay %ld buoc | Toc do: %d xung/s | Chieu: %s\n", 
                      targetSteps, speedHz, currentDir ? "THUAN" : "NGUOC");
      } else {
        setLedColor(255, 0, 0);
        Serial.println("[LOI] So buoc phai lon hon 0! Vi du: STEP 8000");
      }
    } else {
      Serial.println("[HUONG DAN] Cu phap: STEP <so_buoc>. Vi du: STEP 8000");
    }
    return;
  }

  // Lệnh: SPEED <xung/giây>
  if (command == "SPEED" || command == "SPD" || command == "HZ") {
    if (param.length() > 0) {
      int val = param.toInt();
      updateSpeed(val);
    } else {
      Serial.println("[HUONG DAN] Cu phap: SPEED <xung_tren_giay>. Vi du: SPEED 2000");
    }
    return;
  }

  // Nhập số trực tiếp (vd: 8000, 16000)
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
      setLedColor(0, 255, 255);
      Serial.printf("[RUN] Nhan lenh chay %ld buoc (Toc do: %d xung/giay)!\n", targetSteps, speedHz);
      return;
    }
  }

  // Các lệnh đơn
  if (command == "F" || command == "THUAN" || command == "FORWARD" || command == "CW") {
    currentDir = HIGH;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(255, 200, 0);
    Serial.printf("[OK] Da chuyen sang chieu: THUAN (Forward / HIGH)%s\n", isRunning ? " (Dang quay)" : "");
  }
  else if (command == "R" || command == "NGUOC" || command == "REVERSE" || command == "CCW") {
    currentDir = LOW;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(255, 80, 0);
    Serial.printf("[OK] Da chuyen sang chieu: NGUOC (Reverse / LOW)%s\n", isRunning ? " (Dang quay)" : "");
  }
  else if (command == "D" || command == "DAO" || command == "TOGGLE") {
    currentDir = !currentDir;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(255, 255, 255);
    Serial.printf("[OK] Da dao chieu quay -> Hien tai: %s%s\n", currentDir ? "THUAN" : "NGUOC", isRunning ? " (Dang quay)" : "");
  }
  else if (command == "CONT" || command == "RUN" || command == "START" || command == "CHAY" || command == "GO") {
    isContinuousMode = true;
    isRunning = true;
    pulsePinState = HIGH;
    digitalWrite(STEP_PIN, HIGH);
    lastPulseUs = micros();
    setLedColor(0, 255, 0);
    Serial.printf("[RUN] Dong co quay LIEN TUC | Toc do: %d XUNG/GIAY\n", speedHz);
  }
  else if (command == "STOP" || command == "DUNG" || command == "PAUSE" || command == "HALT") {
    isRunning = false;
    isContinuousMode = false;
    remainingSteps = 0;
    pulsePinState = HIGH;
    digitalWrite(STEP_PIN, HIGH);
    setLedColor(255, 0, 0);
    Serial.println("[PAUSE] Dong co da DUNG (Truc van duoc khoa giu vi tri).");
  }
  else if (command == "HELP" || command == "?" || command == "MENU") {
    printHelp();
  }
  else {
    setLedColor(255, 0, 0);
    Serial.printf("[?] Khong nhan dien duoc lenh: '%s'. Go 'HELP' de xem huong dan.\n", cmd.c_str());
  }
}

// Xử lý nút bấm trên TM1638
void handleButtons() {
  uint8_t currentButtons = tm_readButtons();

  // Phát hiện nút mới được nhấn (cạnh lên)
  uint8_t pressedButtons = currentButtons & (~lastButtonsState);
  lastButtonsState = currentButtons;

  if (pressedButtons != 0) {
    // Nút S1: Quay 1 Vòng (8000 bước)
    if (pressedButtons & (1 << 0)) {
      currentActiveLed = 1;
      Serial.println("\n👉 [BUTTON S1]: Quay 1 Vong (8000 buoc)");
      processCommand("8000");
    }
    // Nút S2: Quay 2 Vòng (16000 bước)
    if (pressedButtons & (1 << 1)) {
      currentActiveLed = 2;
      Serial.println("\n👉 [BUTTON S2]: Quay 2 Vong (16000 buoc)");
      processCommand("16000");
    }
    // Nút S3: Quay 90 độ (2000 bước)
    if (pressedButtons & (1 << 2)) {
      currentActiveLed = 3;
      Serial.println("\n👉 [BUTTON S3]: Quay 90 Do (2000 buoc)");
      processCommand("2000");
    }
    // Nút S4: Đảo chiều quay
    if (pressedButtons & (1 << 3)) {
      currentActiveLed = 4;
      tempLedTimerMs = millis() + 500;
      Serial.println("\n👉 [BUTTON S4]: Dao chieu quay");
      processCommand("D");
    }
    // Nút S5: Tăng tốc độ (+200 Hz)
    if (pressedButtons & (1 << 4)) {
      int newSpd = speedHz + 200;
      if (newSpd > 5000) newSpd = 5000;
      currentActiveLed = 5;
      tempLedTimerMs = millis() + 500;
      Serial.printf("\n👉 [BUTTON S5]: Tang toc do len %d Hz\n", newSpd);
      updateSpeed(newSpd);
    }
    // Nút S6: Giảm tốc độ (-200 Hz)
    if (pressedButtons & (1 << 5)) {
      int newSpd = speedHz - 200;
      if (newSpd < 100) newSpd = 100;
      currentActiveLed = 6;
      tempLedTimerMs = millis() + 500;
      Serial.printf("\n👉 [BUTTON S6]: Giam toc do xuong %d Hz\n", newSpd);
      updateSpeed(newSpd);
    }
    // Nút S7: Quay liên tục (RUN)
    if (pressedButtons & (1 << 6)) {
      currentActiveLed = 7;
      Serial.println("\n👉 [BUTTON S7]: Quay LIEN TUC (RUN)");
      processCommand("RUN");
    }
    // Nút S8: Dừng khẩn cấp (STOP)
    if (pressedButtons & (1 << 7)) {
      currentActiveLed = 8;
      Serial.println("\n👉 [BUTTON S8]: DUNG KHAN CAP (STOP)");
      processCommand("STOP");
    }
  }
}

// Cập nhật màn hình 8 số LED 7 đoạn và 8 đèn LED đỏ trên TM1638
void updateTm1638Display() {
  uint8_t digits[8] = {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK};
  uint8_t ledsMask = 0;

  // Xác định ĐÚNG 1 LED DUY NHẤT được phép sáng:
  if (millis() < tempLedTimerMs) {
    // Nếu vừa bấm phím tạm thời (S4 đảo chiều, S5 tăng tốc, S6 giảm tốc)
    ledsMask = (1 << (currentActiveLed - 1));
  }
  else if (isRunning) {
    if (isContinuousMode) {
      currentActiveLed = 7; // Chỉ sáng duy nhất D7 khi quay liên tục (RUN)
    } else {
      if (targetSteps == 8000) {
        currentActiveLed = 1; // Chỉ sáng duy nhất D1 khi quay 1 Vòng
      } else if (targetSteps == 16000) {
        currentActiveLed = 2; // Chỉ sáng duy nhất D2 khi quay 2 Vòng
      } else if (targetSteps == 2000) {
        currentActiveLed = 3; // Chỉ sáng duy nhất D3 khi quay 90 Độ
      } else {
        currentActiveLed = 1; // Mặc định D1 cho các số bước khác
      }
    }
    ledsMask = (1 << (currentActiveLed - 1));
  } else {
    currentActiveLed = 8; // Chỉ sáng duy nhất D8 khi đang DỪNG (STOP)
    ledsMask = (1 << (currentActiveLed - 1));
  }

  // Cập nhật hiển thị màn hình 7 đoạn:
  if (isRunning) {
    if (!isContinuousMode) {
      // Đang chạy số bước cụ thể -> Đếm lùi số bước còn lại
      long num = remainingSteps;
      for (int i = 7; i >= 0; i--) {
        if (num > 0 || i == 7) {
          digits[i] = SEG_DIGITS[num % 10];
          num /= 10;
        } else {
          digits[i] = SEG_BLANK;
        }
      }
    } else {
      // Đang chạy liên tục -> Hiển thị "rUn" + Tốc độ (vd: "rUn 1600")
      digits[0] = SEG_r;
      digits[1] = SEG_u;
      digits[2] = SEG_n;
      digits[3] = SEG_BLANK;
      int spd = speedHz;
      for (int i = 7; i >= 4; i--) {
        if (spd > 0 || i == 7) {
          digits[i] = SEG_DIGITS[spd % 10];
          spd /= 10;
        } else {
          digits[i] = SEG_BLANK;
        }
      }
    }
  } else {
    // Khi đang DỪNG -> Hiển thị "SP" + Tốc độ cài đặt (vd: "SP  1600")
    digits[0] = SEG_S;
    digits[1] = SEG_P;
    digits[2] = SEG_BLANK;
    digits[3] = SEG_BLANK;
    int spd = speedHz;
    for (int i = 7; i >= 4; i--) {
      if (spd > 0 || i == 7) {
        digits[i] = SEG_DIGITS[spd % 10];
        spd /= 10;
      } else {
        digits[i] = SEG_BLANK;
      }
    }
  }

  tm_updateDisplay(digits, ledsMask);
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

  // Cấu hình chân Output Motor
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  // Cấu hình LED RGB trên mạch
  pinMode(2, OUTPUT);
  pinMode(48, OUTPUT);

  // Thiết lập trạng thái ban đầu:
  digitalWrite(STEP_PIN, HIGH);
  digitalWrite(DIR_PIN, currentDir);
  digitalWrite(ENA_PIN, HIGH);

  setLedColor(0, 0, 150); // Xanh dương

  // Khởi động module TM1638 LED & KEY
  tm_init();

  printHelp();
  Serial.printf(">> Khoi dong san sang! (PUL=%d, DIR=%d, ENA=%d) | TM1638 (STB=%d, CLK=%d, DIO=%d)\n", 
                STEP_PIN, DIR_PIN, ENA_PIN, TM_STB_PIN, TM_CLK_PIN, TM_DIO_PIN);
  Serial.println(">> Nhap so buoc (vd: 8000 hoac RUN) hoac BAM NUT S1-S8 tren module TM1638:\n");
}

void loop() {
  // 1. Kiểm tra lệnh qua Serial Monitor
  checkSerial();

  // 2. Định kỳ quét nút bấm và cập nhật màn hình TM1638 mỗi 50ms (Non-blocking)
  unsigned long currentMs = millis();
  if (currentMs - lastTmUpdateMs >= 50) {
    lastTmUpdateMs = currentMs;
    handleButtons();
    updateTm1638Display();
  }

  // 3. Phát xung Non-Blocking bằng hàm micros() chính xác cao
  if (isRunning) {
    unsigned long nowUs = micros();
    if (nowUs - lastPulseUs >= stepHalfPeriodUs) {
      lastPulseUs = nowUs;

      if (pulsePinState == HIGH) {
        digitalWrite(STEP_PIN, LOW); // Phát xung kích
        pulsePinState = LOW;
      } else {
        digitalWrite(STEP_PIN, HIGH);
        pulsePinState = HIGH;
        totalExecutedSteps++;

        if (!isContinuousMode) {
          remainingSteps--;
          if (remainingSteps <= 0) {
            isRunning = false;
            setLedColor(255, 0, 255); // Tím khi hoàn thành
            Serial.printf("\n🎉 [HOAN THANH] Da quay dung %ld buoc! Dong co da tu dong dung. (LED: Tim)\n\n", targetSteps);
          }
        }
      }
    }
  }
}
