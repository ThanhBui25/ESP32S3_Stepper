#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Update.h>

// =============================================================================
// CẤU HÌNH THÔNG TIN WIFI & TÊN MIỀN OTA
// =============================================================================
const char* WIFI_SSID = "V94_VP2.4";
const char* WIFI_PASS = "00008888";
const char* HOSTNAME  = "esp32s3-stepper";

WebServer server(80);
bool isOtaUpdating = false;
int otaProgressPercent = 0;
unsigned long lastWifiCheckMs = 0;
bool wifiWasConnected = false;

// =============================================================================
// CẤU HÌNH CHÂN GPIO 1 ĐỘNG CƠ (KIỂU ĐẤU CỰC DƯƠNG CHUNG - COMMON ANODE)
// =============================================================================
// 1. Chân 5V (hoặc 3.3V) của ESP32-S3 -> Nối chung vào 2 chân: PUL+ , DIR+
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
// - VCC -> 5V (hoặc 3.3V) của ESP32-S3
// - GND -> GND của ESP32-S3
// - STB -> GPIO 10
// - CLK -> GPIO 11
// - DIO -> GPIO 12
// =============================================================================
const int TM_STB_PIN = 10;
const int TM_CLK_PIN = 11;
const int TM_DIO_PIN = 12;

// =============================================================================
// CẤU HÌNH CHÂN MÀN HÌNH LCD 20x4 I2C (PCF8574)
// =============================================================================
// - VCC -> 5V của ESP32-S3 (Bắt buộc 5V để màn hình hiển thị đậm nét)
// - GND -> GND của ESP32-S3
// - SDA -> GPIO 8
// - SCL -> GPIO 9
// =============================================================================
const int I2C_SDA_PIN = 8;
const int I2C_SCL_PIN = 9;
LiquidCrystal_I2C lcd(0x27, 20, 4);
bool lcdFound = false;

// Ký tự đồ họa tùy biến trên LCD
byte customIconPlay[8] = { 0x08, 0x0C, 0x0E, 0x0F, 0x0E, 0x0C, 0x08, 0x00 }; // ▶ Play
byte customIconStop[8] = { 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00 }; // ■ Stop

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

// Biến hiển thị tương tác trên màn hình LCD 20x4
String lastCmdText = "[READY] SYSTEM OK";
unsigned long lastLcdUpdateMs = 0;

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
const uint8_t SEG_o     = 0x5C;
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
  Serial.println(">>> BANG LENH DIEU KHIEN ESP32-S3 (SERIAL + TM1638 + LCD + OTA WIFI) <<<");
  Serial.println("=========================================================================");
  Serial.println(" A. DIEU KHIEN QUA SERIAL MONITOR:");
  Serial.println("   - Nhap SO (vd: 8000, 16000, STEP 8000): Chay du buoc roi DUNG [LED CYAN/TIM]");
  Serial.println("   - SPEED <xung/giay> (vd: SPEED 2000)   : Doi toc do quay       [LED XANH LAM]");
  Serial.println("   - F / THUAN                            : Quay THUAN            [LED VANG]");
  Serial.println("   - R / NGUOC                            : Quay NGUOC            [LED CAM]");
  Serial.println("   - D / DAO                              : DAO CHIEU quay        [LED TRANG]");
  Serial.println("   - CONT / RUN                           : Quay LIEN TUC         [LED XANH LA]");
  Serial.println("   - STOP / DUNG                          : DUNG KHAN CAP         [LED DO]");
  Serial.println("   - IP / WIFI / STATUS                   : Kiem tra IP & Thong tin OTA");
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

  // Lệnh: IP / WIFI / STATUS
  if (command == "IP" || command == "WIFI" || command == "INFO") {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n==================================================");
      Serial.printf(" [WIFI] SSID      : %s\n", WIFI_SSID);
      Serial.printf(" [WIFI] IP Address: %s\n", WiFi.localIP().toString().c_str());
      Serial.printf(" [WIFI] Hostname  : http://%s.local\n", HOSTNAME);
      Serial.printf(" [WIFI] Tin hieu  : %d dBm\n", WiFi.RSSI());
      Serial.printf(" [OTA]  ArduinoOTA: Port 3232 (espota protocol)\n");
      Serial.printf(" [OTA]  HTTP Post : http://%s/update\n", WiFi.localIP().toString().c_str());
      Serial.println("==================================================\n");
      lastCmdText = "IP:" + WiFi.localIP().toString();
    } else {
      Serial.println("\n[WIFI] Chua ket noi den mang WiFi: " + String(WIFI_SSID));
      lastCmdText = "WIFI DISCONNECTED";
    }
    return;
  }

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
        lastCmdText = String("STEP ") + steps;
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
      lastCmdText = String("SPEED ") + speedHz + "Hz";
    } else {
      Serial.println("[HUONG DAN] Cu phap: SPEED <xung_tren_giay>. Vi du: SPEED 2000");
    }
    return;
  }

  // Nhập số trực tiếp (vd: 8000, 16000)
  long directSteps = command.toInt();
  if (directSteps > 0) {
    targetSteps = directSteps;
    remainingSteps = directSteps;
    isContinuousMode = false;
    isRunning = true;
    pulsePinState = HIGH;
    digitalWrite(STEP_PIN, HIGH);
    lastPulseUs = micros();
    setLedColor(0, 255, 255);
    lastCmdText = String("STEP ") + directSteps;
    Serial.printf("[RUN] Nhan lenh chay %ld buoc (Toc do: %d xung/giay)!\n", targetSteps, speedHz);
    return;
  }

  // Đổi chiều quay
  if (command == "F" || command == "THUAN" || command == "CW") {
    currentDir = HIGH;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(255, 200, 0);
    lastCmdText = "DIR -> THUAN [CW]";
    Serial.println("[OK] Da chuyen sang chieu THUAN (CW)");
  }
  else if (command == "R" || command == "NGUOC" || command == "CCW") {
    currentDir = LOW;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(255, 100, 0);
    lastCmdText = "DIR -> NGUOC [CCW]";
    Serial.println("[OK] Da chuyen sang chieu NGUOC (CCW)");
  }
  else if (command == "D" || command == "DAO" || command == "REV") {
    currentDir = !currentDir;
    digitalWrite(DIR_PIN, currentDir);
    setLedColor(200, 200, 200);
    lastCmdText = currentDir ? "DAO -> THUAN [CW]" : "DAO -> NGUOC [CCW]";
    Serial.printf("[OK] Da dao chieu quay -> Hien tai: %s\n", currentDir ? "THUAN" : "NGUOC");
  }
  // Chạy liên tục / Dừng
  else if (command == "CONT" || command == "RUN" || command == "CHAY" || command == "GO") {
    isContinuousMode = true;
    isRunning = true;
    pulsePinState = HIGH;
    digitalWrite(STEP_PIN, HIGH);
    lastPulseUs = micros();
    setLedColor(0, 255, 0);
    lastCmdText = "RUN CONTINOUS";
    Serial.printf("[RUN] Dong co quay LIEN TUC | Toc do: %d XUNG/GIAY\n", speedHz);
  }
  else if (command == "STOP" || command == "DUNG" || command == "PAUSE" || command == "HALT") {
    isRunning = false;
    isContinuousMode = false;
    remainingSteps = 0;
    pulsePinState = HIGH;
    digitalWrite(STEP_PIN, HIGH);
    setLedColor(255, 0, 0);
    lastCmdText = "[STOP] DUNG DONG CO";
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
      lastCmdText = "[S1] 1 VONG (8000s)";
      Serial.println("\n👉 [BUTTON S1]: Quay 1 Vong (8000 buoc)");
      processCommand("8000");
    }
    // Nút S2: Quay 2 Vòng (16000 bước)
    if (pressedButtons & (1 << 1)) {
      currentActiveLed = 2;
      lastCmdText = "[S2] 2 VONG (16000)";
      Serial.println("\n👉 [BUTTON S2]: Quay 2 Vong (16000 buoc)");
      processCommand("16000");
    }
    // Nút S3: Quay 90 độ (2000 bước)
    if (pressedButtons & (1 << 2)) {
      currentActiveLed = 3;
      lastCmdText = "[S3] QUAY 90 DO";
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
      lastCmdText = String("[S5] TANG +200Hz");
    }
    // Nút S6: Giảm tốc độ (-200 Hz)
    if (pressedButtons & (1 << 5)) {
      int newSpd = speedHz - 200;
      if (newSpd < 100) newSpd = 100;
      currentActiveLed = 6;
      tempLedTimerMs = millis() + 500;
      Serial.printf("\n👉 [BUTTON S6]: Giam toc do xuong %d Hz\n", newSpd);
      updateSpeed(newSpd);
      lastCmdText = String("[S6] GIAM -200Hz");
    }
    // Nút S7: Quay liên tục (RUN)
    if (pressedButtons & (1 << 6)) {
      currentActiveLed = 7;
      lastCmdText = "[S7] CHAY LIEN TUC";
      Serial.println("\n👉 [BUTTON S7]: Quay LIEN TUC (RUN)");
      processCommand("RUN");
    }
    // Nút S8: Dừng khẩn cấp (STOP)
    if (pressedButtons & (1 << 7)) {
      currentActiveLed = 8;
      lastCmdText = "[S8] !! E-STOP !!";
      Serial.println("\n👉 [BUTTON S8]: DUNG KHAN CAP (STOP)");
      processCommand("STOP");
    }
  }
}

// Cập nhật màn hình 8 số LED 7 đoạn và 8 đèn LED đỏ trên TM1638
void updateTm1638Display() {
  if (isOtaUpdating) return;

  uint8_t digits[8] = {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK};
  uint8_t ledsMask = 0;

  // Xác định ĐÚNG 1 LED DUY NHẤT được phép sáng:
  if (millis() < tempLedTimerMs) {
    ledsMask = (1 << (currentActiveLed - 1));
  }
  else if (isRunning) {
    if (isContinuousMode) {
      currentActiveLed = 7; // Chỉ sáng duy nhất D7 khi quay liên tục (RUN)
    }
    ledsMask = (1 << (currentActiveLed - 1));
  } else {
    currentActiveLed = 8;   // Chỉ sáng duy nhất D8 khi đang STOP
    ledsMask = (1 << 7);
  }

  // Hiển thị chữ lên 8 LED 7 đoạn:
  if (isRunning) {
    if (isContinuousMode) {
      // Đang quay liên tục: Hiển thị "rUn. 1600"
      digits[0] = SEG_r;
      digits[1] = SEG_u;
      digits[2] = SEG_n;
      digits[3] = SEG_BLANK;
      int spd = speedHz;
      for (int i = 7; i >= 4; i--) {
        digits[i] = SEG_DIGITS[spd % 10];
        spd /= 10;
      }
    } else {
      // Đang quay theo số bước: Hiển thị số bước còn lại (đếm ngược về 0)
      long steps = remainingSteps;
      for (int i = 7; i >= 0; i--) {
        if (steps > 0 || i == 7) {
          digits[i] = SEG_DIGITS[steps % 10];
          steps /= 10;
        } else {
          digits[i] = SEG_BLANK;
        }
      }
    }
  } else {
    // Khi đang DỪNG: Hiển thị "StoP. 1600"
    digits[0] = SEG_S;
    digits[1] = SEG_t;
    digits[2] = SEG_o;
    digits[3] = SEG_P;
    int spd = speedHz;
    for (int i = 7; i >= 4; i--) {
      digits[i] = SEG_DIGITS[spd % 10];
      spd /= 10;
    }
  }

  tm_updateDisplay(digits, ledsMask);
}

// =============================================================================
// HÀM HIỂN THỊ MÀN HÌNH LCD 20x4 I2C
// =============================================================================
void updateLcdDisplay() {
  if (!lcdFound || isOtaUpdating) return;

  char l0[21], l1[21], l2[21], l3[21];

  // Dòng 0: SYS : MOTOR [RUNNING ▶ / STOPPED ■]
  if (isRunning) {
    if (isContinuousMode) {
      snprintf(l0, sizeof(l0), "SYS : MOTOR RUN-CONT");
    } else {
      snprintf(l0, sizeof(l0), "SYS : MOTOR STEPPING");
    }
  } else {
    snprintf(l0, sizeof(l0), "SYS : MOTOR STOPPED ");
  }

  // Dòng 1: SPD : 1600Hz | CW  120RPM (tính theo 800 xung/vòng)
  int rpm = (speedHz * 60) / 800;
  snprintf(l1, sizeof(l1), "SPD : %4dHz|%-3s%3dRPM", speedHz, currentDir ? "CW" : "CCW", rpm);

  // Dòng 2: Tọa độ bước / Tiến trình chạy
  if (isRunning && !isContinuousMode) {
    snprintf(l2, sizeof(l2), "REM : %05ld / %05ld", remainingSteps, targetSteps);
  } else {
    snprintf(l2, sizeof(l2), "POS : %+9ld BUOC", totalExecutedSteps);
  }

  // Dòng 3: Hiển thị IP WiFi hoặc Lệnh thao tác gần nhất
  if (lastCmdText == "[READY] SYSTEM OK" && WiFi.status() == WL_CONNECTED) {
    snprintf(l3, sizeof(l3), "IP  : %-14s", WiFi.localIP().toString().c_str());
  } else {
    snprintf(l3, sizeof(l3), "CMD : %-14s", lastCmdText.c_str());
  }

  // Đảm bảo kết thúc chuỗi đúng 20 ký tự
  l0[20] = '\0';
  l1[20] = '\0';
  l2[20] = '\0';
  l3[20] = '\0';

  lcd.setCursor(0, 0);
  lcd.print(l0);
  lcd.setCursor(19, 0);
  lcd.write(isRunning ? 0 : 1); // Ký tự ▶ hoặc ■

  lcd.setCursor(0, 1);
  lcd.print(l1);

  lcd.setCursor(0, 2);
  lcd.print(l2);

  lcd.setCursor(0, 3);
  lcd.print(l3);
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

// =============================================================================
// THIẾT LẬP CÁC DỊCH VỤ OTA & WEB SERVER
// =============================================================================

// Trang HTML đơn giản tải trực tiếp từ bộ nhớ Flash
const char indexHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <title>ESP32-S3 Stepper OTA</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; text-align: center; margin:0; padding: 30px 15px; background: #0f172a; color: #f8fafc; }
    .card { background: #1e293b; padding: 25px; border-radius: 16px; max-width: 460px; margin: auto; box-shadow: 0 10px 25px rgba(0,0,0,0.5); border: 1px solid #334155; }
    h2 { color: #38bdf8; margin: 0 0 10px 0; font-size: 24px; }
    p { color: #94a3b8; font-size: 14px; margin-bottom: 20px; }
    .file-box { border: 2px dashed #475569; padding: 20px; border-radius: 10px; margin-bottom: 20px; background: #0f172a; }
    input[type=file] { color: #cbd5e1; font-size: 14px; }
    .btn { background: #0284c7; color: #fff; border: none; padding: 12px 28px; font-weight: bold; border-radius: 8px; cursor: pointer; font-size: 15px; width: 100%; transition: 0.2s; }
    .btn:hover { background: #0369a1; }
    .status-badge { display: inline-block; padding: 4px 12px; border-radius: 20px; background: #064e3b; color: #34d399; font-size: 13px; font-weight: bold; margin-bottom: 15px; }
  </style>
</head>
<body>
  <div class="card">
    <div class="status-badge">&#9679; ESP32-S3 ONLINE</div>
    <h2>OTA FIRMWARE UPDATE</h2>
    <p>Cap nhat firmware khong day cho Mach Dieu Khien Stepper</p>
    <form method='POST' action='/update' enctype='multipart/form-data'>
      <div class="file-box">
        <input type='file' name='update' required accept=".bin">
      </div>
      <input type='submit' value='TIEN HANH NAP FIRMWARE' class='btn'>
    </form>
  </div>
</body>
</html>
)rawliteral";

void setupOtaAndWebServer() {
  // 1. Trang Web chính
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", indexHtml);
  });

  // 2. REST API trạng thái JSON phục vụ WinFormsApp1 hoặc các tool giám sát
  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"status\":\"ONLINE\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"hostname\":\"" + String(HOSTNAME) + "\",";
    json += "\"running\":" + String(isRunning ? "true" : "false") + ",";
    json += "\"speed\":" + String(speedHz) + ",";
    json += "\"dir\":\"" + String(currentDir ? "CW" : "CCW") + "\",";
    json += "\"totalSteps\":" + String(totalExecutedSteps) + ",";
    json += "\"remainingSteps\":" + String(remainingSteps) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI());
    json += "}";
    server.send(200, "application/json", json);
  });

  // 3. Endpoint /cmd: Nhận và thực thi lệnh điều khiển trực tiếp qua WiFi
  server.on("/cmd", HTTP_GET, []() {
    String c = "";
    if (server.hasArg("c")) c = server.arg("c");
    else if (server.hasArg("cmd")) c = server.arg("cmd");
    
    if (c.length() > 0) {
      processCommand(c);
      String json = "{";
      json += "\"status\":\"OK\",";
      json += "\"cmd\":\"" + c + "\",";
      json += "\"running\":" + String(isRunning ? "true" : "false") + ",";
      json += "\"speed\":" + String(speedHz) + ",";
      json += "\"dir\":\"" + String(currentDir ? "CW" : "CCW") + "\",";
      json += "\"totalSteps\":" + String(totalExecutedSteps) + ",";
      json += "\"remainingSteps\":" + String(remainingSteps) + ",";
      json += "\"rssi\":" + String(WiFi.RSSI());
      json += "}";
      server.send(200, "application/json", json);
    } else {
      server.send(400, "text/plain", "Missing cmd parameter. Example: /cmd?c=RUN");
    }
  });

  server.on("/cmd", HTTP_POST, []() {
    String c = server.arg("plain");
    if (c.length() == 0 && server.hasArg("c")) c = server.arg("c");
    if (c.length() == 0 && server.hasArg("cmd")) c = server.arg("cmd");

    if (c.length() > 0) {
      processCommand(c);
      String json = "{";
      json += "\"status\":\"OK\",";
      json += "\"cmd\":\"" + c + "\",";
      json += "\"running\":" + String(isRunning ? "true" : "false") + ",";
      json += "\"speed\":" + String(speedHz) + ",";
      json += "\"dir\":\"" + String(currentDir ? "CW" : "CCW") + "\",";
      json += "\"totalSteps\":" + String(totalExecutedSteps) + ",";
      json += "\"remainingSteps\":" + String(remainingSteps) + ",";
      json += "\"rssi\":" + String(WiFi.RSSI());
      json += "}";
      server.send(200, "application/json", json);
    } else {
      server.send(400, "text/plain", "Empty command body");
    }
  });

  // 3. Endpoint HTTP POST /update (Dành cho WinFormsApp và Web Browser)
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "UPDATE THAT BAI!" : "UPDATE THANH CONG! DANG KHOI DONG LAI...");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      isOtaUpdating = true;
      isRunning = false;
      digitalWrite(ENA_PIN, HIGH); // Ngắt động cơ an toàn
      setLedColor(255, 165, 0);   // Đèn vàng cam
      Serial.printf("\n[HTTP-OTA] Nhan file firmware: %s\n", upload.filename.c_str());
      if (lcdFound) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("=== HTTP OTA ===");
        lcd.setCursor(0, 1);
        lcd.print("Dang nhan firmware..");
      }
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        setLedColor(0, 255, 0); // Xanh lá
        Serial.printf("[HTTP-OTA] Nap thanh cong: %u bytes! Dang khoi dong lai...\n", upload.totalSize);
        if (lcdFound) {
          lcd.setCursor(0, 2);
          lcd.print("NAP XONG 100%!");
          lcd.setCursor(0, 3);
          lcd.print("Dang Reset Kit...");
        }
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();
  Serial.println("[HTTP] Web Server & Update Endpoint [/update] da san sang!");

  // 4. Cấu hình ArduinoOTA (Giao thức ESPOTA - Cổng 3232)
  ArduinoOTA.setHostname(HOSTNAME);

  ArduinoOTA.onStart([]() {
    isOtaUpdating = true;
    isRunning = false;
    digitalWrite(ENA_PIN, HIGH); // Thả động cơ để bảo vệ an toàn
    setLedColor(255, 165, 0);    // Vàng cam
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Firmware" : "Filesystem";
    Serial.println("\n[ARDUINO-OTA] Bat dau nap " + type + " qua mang...");
    if (lcdFound) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("=== ARDUINO OTA ===");
      lcd.setCursor(0, 1);
      lcd.print("Dang nap firmware...");
    }
  });

  ArduinoOTA.onEnd([]() {
    setLedColor(0, 255, 0); // Xanh lá
    Serial.println("\n[ARDUINO-OTA] Nap thanh cong! Dang khoi dong lai...");
    if (lcdFound) {
      lcd.setCursor(0, 2);
      lcd.print("NAP THANH CONG!");
      lcd.setCursor(0, 3);
      lcd.print("Dang Reset Kit...");
    }
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    otaProgressPercent = (progress / (total / 100));
    Serial.printf("[ARDUINO-OTA] Tien do: %u%%\r", otaProgressPercent);
    if (lcdFound && otaProgressPercent % 10 == 0) {
      lcd.setCursor(0, 2);
      lcd.printf("Tien do: %3d%% [", otaProgressPercent);
      int bars = otaProgressPercent / 10;
      for (int i = 0; i < 10; i++) {
        lcd.print(i < bars ? "=" : " ");
      }
      lcd.print("]");
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    isOtaUpdating = false;
    setLedColor(255, 0, 0); // Đỏ
    Serial.printf("\n[ARDUINO-OTA] LOI [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Loi xac thuc");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Loi khoi tao Begin");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Loi ket noi Connect");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Loi nhan du lieu Receive");
    else if (error == OTA_END_ERROR) Serial.println("Loi ket thuc End");
  });

  ArduinoOTA.begin();
  Serial.println("[ARDUINO-OTA] San sang nhan code qua giao thuc ESPOTA (Port 3232)!");
}

// =============================================================================
// SETUP & LOOP CHÍNH
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(500);

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

  // Khởi động giao tiếp I2C cho Màn hình LCD 20x4
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.beginTransmission(0x27);
  if (Wire.endTransmission() == 0) {
    lcd = LiquidCrystal_I2C(0x27, 20, 4);
    lcdFound = true;
  } else {
    Wire.beginTransmission(0x3F);
    if (Wire.endTransmission() == 0) {
      lcd = LiquidCrystal_I2C(0x3F, 20, 4);
      lcdFound = true;
    }
  }

  if (lcdFound) {
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, customIconPlay);
    lcd.createChar(1, customIconStop);
    lcd.clear();
    updateLcdDisplay();
  }

  printHelp();
  Serial.printf(">> Khoi dong san sang! (PUL=%d, DIR=%d, ENA=%d) | TM1638 (STB=%d, CLK=%d, DIO=%d) | LCD 20x4 I2C (SDA=%d, SCL=%d, Found=%s)\n", 
                STEP_PIN, DIR_PIN, ENA_PIN, TM_STB_PIN, TM_CLK_PIN, TM_DIO_PIN, I2C_SDA_PIN, I2C_SCL_PIN, lcdFound ? "YES" : "NO");

  // Khởi động kết nối WiFi (Station Mode)
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf(">> Dang ket noi WiFi: %s ...\n", WIFI_SSID);

  // Chờ tối đa 6 giây để kết nối ban đầu
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 6000) {
    delay(300);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiWasConnected = true;
    Serial.printf("\n[WIFI] Da ket noi thanh cong! IP: %s\n", WiFi.localIP().toString().c_str());
    if (MDNS.begin(HOSTNAME)) {
      Serial.printf("[mDNS] Hostname: http://%s.local\n", HOSTNAME);
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("arduino", "tcp", 3232);
    }
    setupOtaAndWebServer();
    lastCmdText = "IP:" + WiFi.localIP().toString();
  } else {
    Serial.printf("\n[WIFI] Dang thu ket noi trong nen... (Trang thai: %d)\n", WiFi.status());
  }

  Serial.println(">> Nhap so buoc (vd: 8000 hoac RUN) hoac BAM NUT S1-S8 tren module TM1638:\n");
}

void loop() {
  unsigned long currentMs = millis();

  // 1. Quản lý trạng thái kết nối WiFi & OTA
  if (currentMs - lastWifiCheckMs >= 3000) {
    lastWifiCheckMs = currentMs;
    if (WiFi.status() == WL_CONNECTED) {
      if (!wifiWasConnected) {
        wifiWasConnected = true;
        Serial.printf("\n[WIFI] Da ket noi! IP: %s\n", WiFi.localIP().toString().c_str());
        if (MDNS.begin(HOSTNAME)) {
          MDNS.addService("http", "tcp", 80);
          MDNS.addService("arduino", "tcp", 3232);
        }
        setupOtaAndWebServer();
        lastCmdText = "IP:" + WiFi.localIP().toString();
      }
    } else {
      if (wifiWasConnected) {
        wifiWasConnected = false;
        Serial.println("\n[WIFI] Mat ket noi WiFi! Dang tu dong ket noi lai...");
      }
    }
  }

  if (wifiWasConnected) {
    ArduinoOTA.handle();
    server.handleClient();
  }

  // 2. Kiểm tra lệnh qua Serial Monitor
  checkSerial();

  // 3. Định kỳ quét nút bấm và cập nhật màn hình TM1638 mỗi 50ms (Non-blocking)
  if (currentMs - lastTmUpdateMs >= 50) {
    lastTmUpdateMs = currentMs;
    handleButtons();
    updateTm1638Display();
  }

  // 4. Định kỳ cập nhật màn hình LCD 20x4 mỗi 100ms (Non-blocking)
  if (currentMs - lastLcdUpdateMs >= 100) {
    lastLcdUpdateMs = currentMs;
    updateLcdDisplay();
  }

  // 5. Phát xung Non-Blocking bằng hàm micros() chính xác cao
  if (isRunning && !isOtaUpdating) {
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
            lastCmdText = "[DONE] HOAN THANH";
            Serial.printf("\n🎉 [HOAN THANH] Da quay dung %ld buoc! Dong co da tu dong dung. (LED: Tim)\n\n", targetSteps);
          }
        }
      }
    }
  }
}
