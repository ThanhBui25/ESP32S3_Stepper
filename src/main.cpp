#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_timer.h>

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
// CẤU HÌNH CHÂN GPIO 3 ĐỘNG CƠ BƯỚC (COMMON ANODE - CỰC DƯƠNG CHUNG 3.3V)
// =============================================================================
// 1. Chân 3.3V của ESP32-S3 -> Nối chung vào: PUL+, DIR+, ENA+ của cả 3 Driver
// 2. Chân GPIO của ESP32-S3 nối vào các chân âm của Driver:
//    --- DRIVER 1 (MOTOR 1) ---
//    - GPIO 4  -> PUL1- (Phát xung bước Motor 1)
//    - GPIO 5  -> DIR1- (Chiều quay Motor 1)
//    - GPIO 6  -> ENA1- (Kích hoạt Driver 1)
//    --- DRIVER 2 (MOTOR 2) ---
//    - GPIO 15 -> PUL2- (Phát xung bước Motor 2)
//    - GPIO 16 -> DIR2- (Chiều quay Motor 2)
//    - GPIO 17 -> ENA2- (Kích hoạt Driver 2)
//    --- DRIVER 3 (MOTOR 3) ---
//    - GPIO 7  -> PUL3- (Phát xung bước Motor 3)
//    - GPIO 18 -> DIR3- (Chiều quay Motor 3)
//    - GPIO 13 -> ENA3- (Kích hoạt Driver 3)
// =============================================================================
const int STEP1_PIN = 4;
const int DIR1_PIN  = 5;
const int ENA1_PIN  = 6;

const int STEP2_PIN = 15;
const int DIR2_PIN  = 16;
const int ENA2_PIN  = 17;

const int STEP3_PIN = 7;
const int DIR3_PIN  = 18;
const int ENA3_PIN  = 13;

// =============================================================================
// CẤU HÌNH CHÂN MODULE BÀN PHÍM & MÀN HÌNH TM1638 (LED & KEY)
// =============================================================================
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
// CẤU TRÚC DỮ LIỆU ĐIỀU KHIỂN ĐỘNG CƠ BƯỚC (STEPPER MOTOR OBJECT)
// =============================================================================
struct StepperMotor {
  int id;
  const char* roleName;            // "CHINH" (Master), "PHU 1" (Slave 1), "PHU 2" (Slave 2)
  int stepPin;
  int dirPin;
  int enaPin;
  esp_timer_handle_t timerHandle;
  volatile int speedHz;
  volatile bool currentDir;        // HIGH = CW (Thuận), LOW = CCW (Ngược)
  volatile bool isRunning;
  volatile bool isContinuousMode;
  volatile long targetSteps;
  volatile long remainingSteps;
  volatile long totalExecutedSteps;
  bool completedNotified;
};

// Khởi tạo 3 đối tượng động cơ: Motor 1 CHÍNH (Master), Motor 2 & 3 PHỤ (Slaves)
StepperMotor motor1 = { 1, "CHINH", STEP1_PIN, DIR1_PIN, ENA1_PIN, NULL, 3200, HIGH, false, false, 0, 0, 0, false };
StepperMotor motor2 = { 2, "PHU 1", STEP2_PIN, DIR2_PIN, ENA2_PIN, NULL, 3200, HIGH, false, false, 0, 0, 0, false };
StepperMotor motor3 = { 3, "PHU 2", STEP3_PIN, DIR3_PIN, ENA3_PIN, NULL, 3200, HIGH, false, false, 0, 0, 0, false };

// Bộ đệm nhận lệnh Serial
String inputBuffer = "";

// Biến thời gian cập nhật hiển thị TM1638 & đọc phím
unsigned long lastTmUpdateMs = 0;
uint8_t lastButtonsState = 0;
uint8_t currentActiveLed = 8; // 1..8
unsigned long tempLedTimerMs = 0;
unsigned long s7HoldStartMs = 0;
unsigned long s7LastRepeatMs = 0;
unsigned long s8HoldStartMs = 0;
unsigned long s8LastRepeatMs = 0;

// Biến hiển thị tương tác trên màn hình LCD 20x4
String lastCmdText = "[READY] SYSTEM 3M";
unsigned long lastLcdUpdateMs = 0;

// =============================================================================
// NGẮT PHÁT XUNG PHẦN CỨNG 3 ĐỘNG CƠ (3 HARDWARE TIMERS RIÊNG BIỆT)
// =============================================================================
void IRAM_ATTR onStepTimerCallback1(void* arg) {
  if (motor1.isRunning && !isOtaUpdating) {
    digitalWrite(motor1.stepPin, LOW); // Kéo LOW kích xung Common Anode
    esp_rom_delay_us(4);               // Xung tối thiểu 4us cho Opto DM542E
    digitalWrite(motor1.stepPin, HIGH);
    motor1.totalExecutedSteps++;

    if (!motor1.isContinuousMode) {
      if (motor1.remainingSteps > 0) {
        motor1.remainingSteps--;
        if (motor1.remainingSteps == 0) {
          motor1.isRunning = false;
        }
      }
    }
  }
}

void IRAM_ATTR onStepTimerCallback2(void* arg) {
  if (motor2.isRunning && !isOtaUpdating) {
    digitalWrite(motor2.stepPin, LOW);
    esp_rom_delay_us(4);
    digitalWrite(motor2.stepPin, HIGH);
    motor2.totalExecutedSteps++;

    if (!motor2.isContinuousMode) {
      if (motor2.remainingSteps > 0) {
        motor2.remainingSteps--;
        if (motor2.remainingSteps == 0) {
          motor2.isRunning = false;
        }
      }
    }
  }
}

void IRAM_ATTR onStepTimerCallback3(void* arg) {
  if (motor3.isRunning && !isOtaUpdating) {
    digitalWrite(motor3.stepPin, LOW);
    esp_rom_delay_us(4);
    digitalWrite(motor3.stepPin, HIGH);
    motor3.totalExecutedSteps++;

    if (!motor3.isContinuousMode) {
      if (motor3.remainingSteps > 0) {
        motor3.remainingSteps--;
        if (motor3.remainingSteps == 0) {
          motor3.isRunning = false;
        }
      }
    }
  }
}

// =============================================================================
// TRÌNH ĐIỀU KHIỂN MODULE TM1638 (NATIVE)
// =============================================================================
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
    delayMicroseconds(1);
    digitalWrite(TM_CLK_PIN, HIGH);
    delayMicroseconds(1);
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
  tm_sendCommand(0x8C);
}

void tm_updateDisplay(uint8_t digits[8], uint8_t ledsMask) {
  tm_sendCommand(0x40);
  digitalWrite(TM_STB_PIN, LOW);
  tm_sendByte(0xC0);

  for (int i = 0; i < 8; i++) {
    tm_sendByte(digits[i]);
    tm_sendByte((ledsMask & (1 << i)) ? 0x01 : 0x00);
  }
  digitalWrite(TM_STB_PIN, HIGH);
}

uint8_t tm_readButtons() {
  digitalWrite(TM_STB_PIN, LOW);
  tm_sendByte(0x42);
  delayMicroseconds(2);

  pinMode(TM_DIO_PIN, INPUT_PULLUP);
  uint8_t keys = 0;

  for (int i = 0; i < 4; i++) {
    uint8_t byteIn = 0;
    for (int j = 0; j < 8; j++) {
      digitalWrite(TM_CLK_PIN, LOW);
      delayMicroseconds(1);
      if (digitalRead(TM_DIO_PIN)) {
        byteIn |= (1 << j);
      }
      digitalWrite(TM_CLK_PIN, HIGH);
      delayMicroseconds(1);
    }
    if (byteIn & 0x01) keys |= (1 << i);
    if (byteIn & 0x10) keys |= (1 << (i + 4));
  }

  pinMode(TM_DIO_PIN, OUTPUT);
  digitalWrite(TM_STB_PIN, HIGH);
  return keys;
}

// =============================================================================
// HÀM ĐỔI MÀU LED RGB & XỬ LÝ ĐỘNG CƠ
// =============================================================================
void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_BUILTIN, r, g, b);
  neopixelWrite(48, r, g, b);
  neopixelWrite(38, r, g, b);
}

void setMotorSpeed(StepperMotor& m, int hz) {
  if (hz >= 100 && hz <= 25000) {
    m.speedHz = hz;
    if (m.timerHandle != NULL) {
      esp_timer_stop(m.timerHandle);
      esp_timer_start_periodic(m.timerHandle, 1000000ULL / (uint64_t)m.speedHz);
    }
  }
}

void updateSpeedAll(int hz, bool verbose = true) {
  if (hz < 100) hz = 100;
  if (hz > 25000) hz = 25000;
  setMotorSpeed(motor1, hz);
  setMotorSpeed(motor2, hz);
  setMotorSpeed(motor3, hz);
  setLedColor(0, 150, 255);
  if (verbose) {
    Serial.printf("[OK] Da dat toc do CA 3 MOTOR: %d XUNG/GIAY (Hz)\n", hz);
  }
}

void setMotorDir(StepperMotor& m, bool dir) {
  m.currentDir = dir;
  digitalWrite(m.dirPin, m.currentDir);
}

void toggleMotorDir(StepperMotor& m) {
  setMotorDir(m, !m.currentDir);
}

void setDirAll(bool dir) {
  setMotorDir(motor1, dir);
  setMotorDir(motor2, dir);
  setMotorDir(motor3, dir);
}

void toggleDirAll() {
  toggleMotorDir(motor1);
  toggleMotorDir(motor2);
  toggleMotorDir(motor3);
}

void stopAll();
bool isMotorsUnlocked = false; // false = LOCKED (giữ cứng), true = UNLOCKED (mở khóa, xoay tay tự do)

void setMotorsEnable(bool enable) {
  // Với mạch Common Anode 3.3V (ENA+ nối 3.3V, ENA- nối GPIO):
  // - ENA pin = HIGH (3.3V) -> Opto tắt -> Driver ENABLED (Khóa/giữ chặt động cơ)
  // - ENA pin = LOW (0V) -> Opto dẫn -> Driver DISABLED (Mở khóa ENA, thả trôi motor để xoay tay tự do)
  if (enable) {
    digitalWrite(ENA1_PIN, HIGH);
    digitalWrite(ENA2_PIN, HIGH);
    digitalWrite(ENA3_PIN, HIGH);
    isMotorsUnlocked = false;
    lastCmdText = "[EN ON] LOCKED";
    setLedColor(0, 255, 0);
    Serial.println("[EN] DA KHOA EN (LOCKED): Driver duoc cap dien, giu chat truc motor.");
  } else {
    stopAll();
    digitalWrite(ENA1_PIN, LOW);
    digitalWrite(ENA2_PIN, LOW);
    digitalWrite(ENA3_PIN, LOW);
    isMotorsUnlocked = true;
    lastCmdText = "[EN OFF] FREE";
    setLedColor(255, 180, 0);
    Serial.println("[EN] DA MO KHOA EN (UNLOCKED/FREE): Tha troi motor, co the xoay bang tay tu do!");
  }
}

// Bật tạm thời nguồn driver khi bắt đầu chạy xung nhưng không làm mất chế độ Unlocked của người dùng
void prepareDriversForMotion() {
  digitalWrite(ENA1_PIN, HIGH);
  digitalWrite(ENA2_PIN, HIGH);
  digitalWrite(ENA3_PIN, HIGH);
}

void runMotorSteps(StepperMotor& m, long steps) {
  if (steps <= 0) return;
  digitalWrite(m.enaPin, HIGH);
  m.isRunning = false;
  m.completedNotified = false;
  m.targetSteps = steps;
  m.remainingSteps = steps;
  m.isContinuousMode = false;
  m.isRunning = true;
}

void runAllSteps(long steps) {
  prepareDriversForMotion();
  runMotorSteps(motor1, steps);
  runMotorSteps(motor2, steps);
  runMotorSteps(motor3, steps);
  setLedColor(0, 255, 255);
}

void runMotorContinuous(StepperMotor& m) {
  digitalWrite(m.enaPin, HIGH);
  m.isRunning = false;
  m.completedNotified = true;
  m.targetSteps = 0;
  m.remainingSteps = 0;
  m.isContinuousMode = true;
  m.isRunning = true;
}

void runAllContinuous() {
  prepareDriversForMotion();
  runMotorContinuous(motor1);
  runMotorContinuous(motor2);
  runMotorContinuous(motor3);
  setLedColor(0, 255, 0);
}

void stopMotor(StepperMotor& m) {
  m.isRunning = false;
  m.isContinuousMode = false;
  m.remainingSteps = 0;
  m.targetSteps = 0;
  m.completedNotified = true;
  if (isMotorsUnlocked) {
    digitalWrite(m.enaPin, LOW);
  }
}

void stopAll() {
  stopMotor(motor1);
  stopMotor(motor2);
  stopMotor(motor3);
  if (isMotorsUnlocked) {
    digitalWrite(ENA1_PIN, LOW);
    digitalWrite(ENA2_PIN, LOW);
    digitalWrite(ENA3_PIN, LOW);
    setLedColor(255, 180, 0);
  } else {
    setLedColor(255, 0, 0);
  }
}

// Chức năng TIẾN LÊN: Motor 1 ĐỨNG YÊN, Motor 2 quay NGƯỢC (CCW/LOW), Motor 3 quay ĐÚNG/THUẬN (CW/HIGH)
void runForwardSubs(long steps = 1600) {
  prepareDriversForMotion();
  stopMotor(motor1);
  setMotorDir(motor2, LOW);  // Motor 2: Ngược chiều kim đồng hồ (CCW)
  setMotorDir(motor3, HIGH); // Motor 3: Đúng/Thuận chiều kim đồng hồ (CW)
  runMotorSteps(motor2, steps);
  runMotorSteps(motor3, steps);
  lastCmdText = String("[TIEN] M2:CCW M3:CW");
  setLedColor(0, 255, 120);
  Serial.printf("[TIEN LEN] Motor 1 DUNG | Motor 2 CCW (Nguoc) & Motor 3 CW (Thuan) %ld buoc\n", steps);
}

// Chức năng LÙI LẠI: Motor 1 ĐỨNG YÊN, Motor 2 quay ĐÚNG/THUẬN (CW/HIGH), Motor 3 quay NGƯỢC (CCW/LOW)
void runBackwardSubs(long steps = 1600) {
  prepareDriversForMotion();
  stopMotor(motor1);
  setMotorDir(motor2, HIGH); // Motor 2: Đúng/Thuận chiều kim đồng hồ (CW)
  setMotorDir(motor3, LOW);  // Motor 3: Ngược chiều kim đồng hồ (CCW)
  runMotorSteps(motor2, steps);
  runMotorSteps(motor3, steps);
  lastCmdText = String("[LUI] M2:CW M3:CCW");
  setLedColor(255, 120, 0);
  Serial.printf("[LUI LAI] Motor 1 DUNG | Motor 2 CW (Thuan) & Motor 3 CCW (Nguoc) %ld buoc\n", steps);
}

// Chức năng XOAY TẠI CHỖ THUẬN (CW): Cả 3 motor cùng quay CHIỀU THUẬN (CW/HIGH)
void runRotateInPlaceCW(long steps = 0) {
  prepareDriversForMotion();
  setMotorDir(motor1, HIGH);
  setMotorDir(motor2, HIGH);
  setMotorDir(motor3, HIGH);
  if (steps > 0) {
    runAllSteps(steps);
    lastCmdText = String("[XOAY CW] +") + steps;
    Serial.printf("[XOAY THUAN] Ca 3 Motor quay THUAN (CW) %ld buoc\n", steps);
  } else {
    runAllContinuous();
    lastCmdText = "[S3] XOAY CW";
    Serial.println("\n👉 [BUTTON S3 GIU]: XOAY TAI CHO THUAN -> Ca 3 Motor quay CW (Nha nut de dung)...");
  }
  setLedColor(0, 200, 255);
}

// Chức năng XOAY TẠI CHỖ NGƯỢC (CCW): Cả 3 motor cùng quay CHIỀU NGƯỢC (CCW/LOW)
void runRotateInPlaceCCW(long steps = 0) {
  prepareDriversForMotion();
  setMotorDir(motor1, LOW);
  setMotorDir(motor2, LOW);
  setMotorDir(motor3, LOW);
  if (steps > 0) {
    runAllSteps(steps);
    lastCmdText = String("[XOAY CCW] -") + steps;
    Serial.printf("[XOAY NGUOC] Ca 3 Motor quay NGUOC (CCW) %ld buoc\n", steps);
  } else {
    runAllContinuous();
    lastCmdText = "[S4] XOAY CCW";
    Serial.println("\n👉 [BUTTON S4 GIU]: XOAY TAI CHO NGUOC -> Ca 3 Motor quay CCW (Nha nut de dung)...");
  }
  setLedColor(255, 100, 200);
}

// Chức năng CHẠY SANG PHẢI: M1 tốc độ gấp đôi (2x CW), M2 quay cùng chiều M1 (CW), M3 quay ngược chiều M1 (CCW)
void runRightMove(long steps = 0) {
  prepareDriversForMotion();
  int v = motor2.speedHz > 0 ? motor2.speedHz : 2000;
  int vDouble = min(v * 2, 25000);
  setMotorSpeed(motor1, vDouble);
  setMotorSpeed(motor2, v);
  setMotorSpeed(motor3, v);
  setMotorDir(motor1, HIGH); // M1 quay Thuận (CW)
  setMotorDir(motor2, HIGH); // M2 cùng chiều M1 (CW)
  setMotorDir(motor3, LOW);  // M3 ngược chiều M1 (CCW)

  if (steps > 0) {
    runMotorSteps(motor1, steps * 2);
    runMotorSteps(motor2, steps);
    runMotorSteps(motor3, steps);
    lastCmdText = String("[PHAI] M1:2x M2:CW M3:CCW");
    Serial.printf("[CHAY PHAI] M1 (CW 2x spd: %dHz), M2 (CW: %dHz), M3 (CCW: %dHz) %ld buoc\n", vDouble, v, v, steps);
  } else {
    runAllContinuous();
    lastCmdText = "[S5] CHAY PHAI";
    Serial.println("\n👉 [BUTTON S5 GIU]: CHAY SANG PHAI -> M1 (2x Spd CW), M2 (CW), M3 (CCW) - Nha nut de dung...");
  }
  setLedColor(0, 255, 255);
}

// Chức năng CHẠY SANG TRÁI (Ngược lại với Chạy Phải): M1 tốc độ 2x CCW, M2 quay cùng chiều M1 (CCW), M3 quay ngược chiều M1 (CW)
void runLeftMove(long steps = 0) {
  prepareDriversForMotion();
  int v = motor2.speedHz > 0 ? motor2.speedHz : 2000;
  int vDouble = min(v * 2, 25000);
  setMotorSpeed(motor1, vDouble);
  setMotorSpeed(motor2, v);
  setMotorSpeed(motor3, v);
  setMotorDir(motor1, LOW);  // M1 quay Ngược (CCW)
  setMotorDir(motor2, LOW);  // M2 cùng chiều M1 (CCW)
  setMotorDir(motor3, HIGH); // M3 ngược chiều M1 (CW)

  if (steps > 0) {
    runMotorSteps(motor1, steps * 2);
    runMotorSteps(motor2, steps);
    runMotorSteps(motor3, steps);
    lastCmdText = String("[TRAI] M1:2x M2:CCW M3:CW");
    Serial.printf("[CHAY TRAI] M1 (CCW 2x spd: %dHz), M2 (CCW: %dHz), M3 (CW: %dHz) %ld buoc\n", vDouble, v, v, steps);
  } else {
    runAllContinuous();
    lastCmdText = "[S6] CHAY TRAI";
    Serial.println("\n👉 [BUTTON S6 GIU]: CHAY SANG TRAI -> M1 (2x Spd CCW), M2 (CCW), M3 (CW) - Nha nut de dung...");
  }
  setLedColor(255, 0, 255);
}

bool isAnyMotorRunning() {
  return motor1.isRunning || motor2.isRunning || motor3.isRunning;
}

// =============================================================================
// HIỂN THỊ MENU TRỢ GIÚP (HELP)
// =============================================================================
void printHelp() {
  setLedColor(200, 200, 200);
  Serial.println("\n=========================================================================");
  Serial.println(">>> HE THONG DIEU KHIEN 3 TRUC: MOTOR 1 (CHINH) + MOTOR 2,3 (PHU) <<<");
  Serial.println("=========================================================================");
  Serial.println(" A. LENH TIEN LEN, LUI LAI, XOAY & CHAY NGANG TRAI/PHAI:");
  Serial.println("   - TIEN / FWD [so_buoc | RUN]           : M1 DUNG, M2:CCW (Nguoc), M3:CW (Thuan)");
  Serial.println("   - LUI  / BACK [so_buoc | RUN]          : M1 DUNG, M2:CW (Thuan), M3:CCW (Nguoc)");
  Serial.println("   - XOAY CW / THUAN [so_buoc | RUN]      : Ca 3 Motor quay THUAN (CW) tai cho");
  Serial.println("   - XOAY CCW / NGUOC [so_buoc | RUN]     : Ca 3 Motor quay NGUOC (CCW) tai cho");
  Serial.println("   - PHAI / RIGHT [so_buoc | RUN]         : M1 (2x Spd CW), M2 (CW), M3 (CCW)");
  Serial.println("   - TRAI / LEFT [so_buoc | RUN]          : M1 (2x Spd CCW), M2 (CCW), M3 (CW)");
  Serial.println("   - EN [ON | OFF | UNLOCK | LOCK]        : KHOA hoac MO KHOA EN (tha troi de xoay tay)");
  Serial.println("-------------------------------------------------------------------------");
  Serial.println(" B. LENH DIEU KHIEN DONG THOI TOAN BO (CHINH + 2 PHU CHAY DONG BO):");
  Serial.println("   - Nhap SO (vd: 800, 1600, 3200)        : Ca 3 Motor chay du buoc roi DUNG");
  Serial.println("   - SPEED <xung/giay> (vd: SPEED 4000)   : Dat toc do ca 3 Motor");
  Serial.println("   - CONT / RUN                           : Ca 3 Motor quay LIEN TUC");
  Serial.println("   - STOP / DUNG                          : DUNG KHAN CAP ca 3 Motor");
  Serial.println("   - F / THUAN                            : Ca 3 Motor quay THUAN (CW)");
  Serial.println("   - R / NGUOC                            : Ca 3 Motor quay NGUOC (CCW)");
  Serial.println("   - D / DAO                              : DAO CHIEU ca 3 Motor");
  Serial.println("-------------------------------------------------------------------------");
  Serial.println(" C. LENH DIEU KHIEN VAI TRO (CHINH / PHU 1 / PHU 2 / SUBS):");
  Serial.println("   - M1 / MAIN <so_buoc | RUN | STOP | SPEED x | F | R | D> : MOTOR 1 (CHINH)");
  Serial.println("   - M2 / SUB1 <so_buoc | RUN | STOP | SPEED x | F | R | D> : MOTOR 2 (PHU 1)");
  Serial.println("   - M3 / SUB2 <so_buoc | RUN | STOP | SPEED x | F | R | D> : MOTOR 3 (PHU 2)");
  Serial.println("   - SUBS / PHU <so_buoc | RUN | STOP | SPEED x | F | R | D>: 2 MOTOR PHU (M2+M3)");
  Serial.println("   - ALL <so_buoc | RUN | STOP | SPEED x | F | R | D>       : CA 3 MOTOR");
  Serial.println("-------------------------------------------------------------------------");
  Serial.println(" D. DIEU KHIEN QUA 8 NUT BAM TREN MODULE TM1638:");
  Serial.println("   - [S1]: GIU de TIEN (M1 dung, M2:CCW, M3:CW) - Nha ra DUNG NGAY");
  Serial.println("   - [S2]: GIU de LUI  (M1 dung, M2:CW, M3:CCW) - Nha ra DUNG NGAY");
  Serial.println("   - [S3]: GIU de XOAY THUAN (Ca 3 motor quay CW) - Nha ra DUNG NGAY");
  Serial.println("   - [S4]: GIU de XOAY NGUOC (Ca 3 motor quay CCW) - Nha ra DUNG NGAY");
  Serial.println("   - [S5]: GIU de CHAY PHAI (M1 2x CW, M2 CW, M3 CCW) - Nha ra DUNG NGAY");
  Serial.println("   - [S6]: GIU de CHAY TRAI (M1 2x CCW, M2 CCW, M3 CW) - Nha ra DUNG NGAY");
  Serial.println("   - [S7]: Tang toc (+100 Hz / Giu de tang) | [S8]: Giam toc (-100 Hz / Giu de giam)");
  Serial.println("=========================================================================\n");
}

void runSingleMotorCW(StepperMotor& m, long steps = 0) {
  prepareDriversForMotion();
  if (m.id == 1) { stopMotor(motor2); stopMotor(motor3); }
  else if (m.id == 2) { stopMotor(motor1); stopMotor(motor3); }
  else if (m.id == 3) { stopMotor(motor1); stopMotor(motor2); }

  setMotorDir(m, HIGH);
  if (steps > 0) {
    runMotorSteps(m, steps);
    lastCmdText = String("M") + m.id + " CW +" + steps;
    Serial.printf("[RUN M%d CW - %s] Chay %ld buoc THUAN (CW)\n", m.id, m.roleName, steps);
  } else {
    runMotorContinuous(m);
    lastCmdText = String("M") + m.id + " CW CONT";
    Serial.printf("[RUN M%d CW - %s] Quay LIEN TUC THUAN (CW)\n", m.id, m.roleName);
  }
  setLedColor(0, 255, 120);
}

void runSingleMotorCCW(StepperMotor& m, long steps = 0) {
  prepareDriversForMotion();
  if (m.id == 1) { stopMotor(motor2); stopMotor(motor3); }
  else if (m.id == 2) { stopMotor(motor1); stopMotor(motor3); }
  else if (m.id == 3) { stopMotor(motor1); stopMotor(motor2); }

  setMotorDir(m, LOW);
  if (steps > 0) {
    runMotorSteps(m, steps);
    lastCmdText = String("M") + m.id + " CCW -" + steps;
    Serial.printf("[RUN M%d CCW - %s] Chay %ld buoc NGUOC (CCW)\n", m.id, m.roleName, steps);
  } else {
    runMotorContinuous(m);
    lastCmdText = String("M") + m.id + " CCW CONT";
    Serial.printf("[RUN M%d CCW - %s] Quay LIEN TUC NGUOC (CCW)\n", m.id, m.roleName);
  }
  setLedColor(255, 100, 0);
}

void processSingleMotorCommand(StepperMotor& m, String subCmd) {
  subCmd.trim();
  if (subCmd.length() == 0) {
    runSingleMotorCW(m, 0);
    return;
  }

  String cmd = subCmd;
  String param = "";
  int spaceIdx = subCmd.indexOf(' ');
  if (spaceIdx != -1) {
    cmd = subCmd.substring(0, spaceIdx);
    param = subCmd.substring(spaceIdx + 1);
    param.trim();
  }
  cmd.toUpperCase();

  if (cmd == "CW" || cmd == "THUAN" || cmd == "F") {
    long steps = (param.length() > 0 && param != "RUN" && param != "CONT") ? param.toInt() : 0;
    runSingleMotorCW(m, steps);
    return;
  }

  if (cmd == "CCW" || cmd == "NGUOC" || cmd == "R") {
    long steps = (param.length() > 0 && param != "RUN" && param != "CONT") ? param.toInt() : 0;
    runSingleMotorCCW(m, steps);
    return;
  }

  if (cmd == "STEP" || cmd == "MOVE" || cmd == "S") {
    long steps = param.toInt();
    if (steps > 0) {
      runSingleMotorCW(m, steps);
    } else {
      Serial.printf("[LOI] So buoc phai > 0! Vi du: M%d STEP 1600\n", m.id);
    }
    return;
  }

  long directSteps = cmd.toInt();
  if (directSteps > 0) {
    runSingleMotorCW(m, directSteps);
    return;
  }

  if (cmd == "SPEED" || cmd == "SPD" || cmd == "HZ") {
    int val = param.toInt();
    if (val >= 100 && val <= 25000) {
      setMotorSpeed(m, val);
      setLedColor(0, 150, 255);
      lastCmdText = String("M") + m.id + " SPD " + m.speedHz + "Hz";
      Serial.printf("[OK M%d - %s] Da dat toc do: %d Hz\n", m.id, m.roleName, m.speedHz);
    } else {
      Serial.printf("[LOI] Toc do 100 - 25000 Hz! Vi du: M%d SPEED 3200\n", m.id);
    }
    return;
  }

  if (cmd == "D" || cmd == "DAO" || cmd == "REV") {
    toggleMotorDir(m);
    setLedColor(200, 200, 200);
    lastCmdText = String("M") + m.id + (m.currentDir ? " DIR CW" : " DIR CCW");
    Serial.printf("[OK M%d - %s] Dao chieu -> %s\n", m.id, m.roleName, m.currentDir ? "CW" : "CCW");
    return;
  }

  if (cmd == "CONT" || cmd == "RUN" || cmd == "CHAY" || cmd == "GO") {
    runSingleMotorCW(m, 0);
    return;
  }

  if (cmd == "STOP" || cmd == "DUNG" || cmd == "PAUSE" || cmd == "HALT") {
    stopMotor(m);
    setLedColor(255, 0, 0);
    lastCmdText = String("M") + m.id + " STOP";
    Serial.printf("[STOP M%d - %s] Da DUNG.\n", m.id, m.roleName);
    return;
  }

  Serial.printf("[?] Lenh M%d khong hop le: '%s'\n", m.id, subCmd.c_str());
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

  // 1. Kiểm tra tiền tố điều khiển: M1/MAIN, M2/SUB1, M3/SUB2, SUBS, ALL
  if (command == "M1" || command == "MAIN" || command == "MASTER" || command == "CHINH") {
    processSingleMotorCommand(motor1, param);
    return;
  }
  if (command == "M2" || command == "SUB1" || command == "SLAVE1" || command == "PHU1") {
    processSingleMotorCommand(motor2, param);
    return;
  }
  if (command == "M3" || command == "SUB2" || command == "SLAVE2" || command == "PHU2") {
    processSingleMotorCommand(motor3, param);
    return;
  }
  if (command == "SUBS" || command == "SUB" || command == "SLAVES" || command == "PHU" || command == "M23") {
    processSingleMotorCommand(motor2, param);
    processSingleMotorCommand(motor3, param);
    return;
  }
  if (command == "ALL" || command == "BOTH" || command == "M123") {
    if (param.length() > 0) {
      processCommand(param);
    }
    return;
  }

  // Lệnh TIẾN LÊN: M1 ĐỨNG YÊN, M2 quay NGƯỢC (CCW/LOW), M3 quay ĐÚNG/THUẬN (CW/HIGH)
  if (command == "TIEN" || command == "FWD" || command == "FORWARD" || command == "TIENLEN") {
    if (param == "RUN" || param == "CONT") {
      stopMotor(motor1);
      setMotorDir(motor2, LOW);  // M2: Ngược chiều kim đồng hồ (CCW)
      setMotorDir(motor3, HIGH); // M3: Đúng/Thuận chiều kim đồng hồ (CW)
      runMotorContinuous(motor2);
      runMotorContinuous(motor3);
      lastCmdText = "TIEN CONT (M2:CCW M3:CW)";
      setLedColor(0, 255, 120);
      Serial.println("[TIEN CONT] Motor 1 DUNG | Motor 2 CCW (Nguoc) & Motor 3 CW (Thuan) lien tuc");
    } else {
      long steps = (param.length() > 0) ? param.toInt() : 1600;
      if (steps <= 0) steps = 1600;
      runForwardSubs(steps);
    }
    return;
  }

  // Lệnh LÙI LẠI: M1 ĐỨNG YÊN, M2 quay ĐÚNG/THUẬN (CW/HIGH), M3 quay NGƯỢC (CCW/LOW)
  if (command == "LUI" || command == "BACK" || command == "BACKWARD" || command == "LUILAI") {
    if (param == "RUN" || param == "CONT") {
      stopMotor(motor1);
      setMotorDir(motor2, HIGH); // M2: Đúng/Thuận chiều kim đồng hồ (CW)
      setMotorDir(motor3, LOW);  // M3: Ngược chiều kim đồng hồ (CCW)
      runMotorContinuous(motor2);
      runMotorContinuous(motor3);
      lastCmdText = "LUI CONT (M2:CW M3:CCW)";
      setLedColor(255, 120, 0);
      Serial.println("[LUI CONT] Motor 1 DUNG | Motor 2 CW (Thuan) & Motor 3 CCW (Nguoc) lien tuc");
    } else {
      long steps = (param.length() > 0) ? param.toInt() : 1600;
      if (steps <= 0) steps = 1600;
      runBackwardSubs(steps);
    }
    return;
  }

  // Lệnh XOAY TẠI CHỖ: Cả 3 motor cùng quay CW (Thuận) hoặc CCW (Ngược)
  if (command == "XOAY" || command == "SPIN" || command == "ROTATE") {
    if (param == "CW" || param == "THUAN" || param == "F" || param == "CW RUN" || param == "CW CONT" || param == "RIGHT") {
      runRotateInPlaceCW(0); // Chạy liên tục cho đến khi nhận STOP (giữ nút để chạy, nhả để dừng)
    } else if (param == "CCW" || param == "NGUOC" || param == "R" || param == "CCW RUN" || param == "CCW CONT" || param == "LEFT") {
      runRotateInPlaceCCW(0); // Chạy liên tục cho đến khi nhận STOP (giữ nút để chạy, nhả để dừng)
    } else if (param == "RUN" || param == "CONT") {
      runRotateInPlaceCW(0);
    } else if (param.startsWith("CW ") || param.startsWith("THUAN ")) {
      long steps = param.substring(param.indexOf(' ') + 1).toInt();
      if (steps <= 0) steps = 1600;
      runRotateInPlaceCW(steps);
    } else if (param.startsWith("CCW ") || param.startsWith("NGUOC ")) {
      long steps = param.substring(param.indexOf(' ') + 1).toInt();
      if (steps <= 0) steps = 1600;
      runRotateInPlaceCCW(steps);
    } else {
      long steps = param.toInt();
      if (steps > 0) {
        runRotateInPlaceCW(steps);
      } else {
        runRotateInPlaceCW(0);
      }
    }
    return;
  }

  // Lệnh MỞ KHÓA / KHÓA EN (ENABLE / UNLOCK)
  if (command == "EN" || command == "ENA" || command == "ENABLE" || command == "UNLOCK" || command == "LOCK" || command == "FREE") {
    if (param == "OFF" || param == "0" || param == "FREE" || param == "UNLOCK" || param == "MO" || command == "UNLOCK" || command == "FREE") {
      setMotorsEnable(false); // Mở khóa EN, thả trôi motor để xoay tay tự do
    } else {
      setMotorsEnable(true);  // Khóa EN, cấp điện giữ chặt động cơ
    }
    return;
  }

  // Lệnh CHẠY SANG PHẢI (M1: 2x CW, M2: CW, M3: CCW)
  if (command == "PHAI" || command == "RIGHT" || command == "RMOVE") {
    if (param == "RUN" || param == "CONT") {
      runRightMove(0);
    } else {
      long steps = (param.length() > 0) ? param.toInt() : 1600;
      if (steps <= 0) steps = 1600;
      runRightMove(steps);
    }
    return;
  }

  // Lệnh CHẠY SANG TRÁI (M1: 2x CCW, M2: CCW, M3: CW)
  if (command == "TRAI" || command == "LEFT" || command == "LMOVE") {
    if (param == "RUN" || param == "CONT") {
      runLeftMove(0);
    } else {
      long steps = (param.length() > 0) ? param.toInt() : 1600;
      if (steps <= 0) steps = 1600;
      runLeftMove(steps);
    }
    return;
  }

  // 2. Lệnh hệ thống / IP / WiFi
  if (command == "IP" || command == "WIFI" || command == "INFO" || command == "STATUS") {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n==================================================");
      Serial.printf(" [WIFI] SSID      : %s\n", WIFI_SSID);
      Serial.printf(" [WIFI] IP Address: %s\n", WiFi.localIP().toString().c_str());
      Serial.printf(" [WIFI] Hostname  : http://%s.local\n", HOSTNAME);
      Serial.printf(" [WIFI] Tin hieu  : %d dBm\n", WiFi.RSSI());
      Serial.printf(" [MOTOR 1 - CHINH] Spd: %dHz | Dir: %s | Run: %s\n", 
                    motor1.speedHz, motor1.currentDir ? "CW" : "CCW", motor1.isRunning ? "RUN" : "STOP");
      Serial.printf(" [MOTOR 2 - PHU 1] Spd: %dHz | Dir: %s | Run: %s\n", 
                    motor2.speedHz, motor2.currentDir ? "CW" : "CCW", motor2.isRunning ? "RUN" : "STOP");
      Serial.printf(" [MOTOR 3 - PHU 2] Spd: %dHz | Dir: %s | Run: %s\n", 
                    motor3.speedHz, motor3.currentDir ? "CW" : "CCW", motor3.isRunning ? "RUN" : "STOP");
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

  // 3. Chạy số bước đồng bộ cho cả 3 motor: STEP <số bước>
  if (command == "STEP" || command == "MOVE" || command == "S") {
    if (param.length() > 0) {
      long steps = param.toInt();
      if (steps > 0) {
        runAllSteps(steps);
        lastCmdText = String("ALL STEP ") + steps;
        Serial.printf("[RUN ALL 3M] Chay ca 3 motor %ld buoc | M1:%dHz M2:%dHz M3:%dHz\n", 
                      steps, motor1.speedHz, motor2.speedHz, motor3.speedHz);
      } else {
        setLedColor(255, 0, 0);
        Serial.println("[LOI] So buoc phai > 0! Vi du: STEP 1600");
      }
    } else {
      Serial.println("[HUONG DAN] Cu phap: STEP <so_buoc>. Vi du: STEP 1600");
    }
    return;
  }

  // 4. Nhập số trực tiếp (vd: 800, 1600, 3200) -> Chạy cả 3 motor
  long directSteps = command.toInt();
  if (directSteps > 0) {
    runAllSteps(directSteps);
    lastCmdText = String("ALL STEP ") + directSteps;
    Serial.printf("[RUN ALL 3M] Nhan lenh chay ca 3 motor %ld buoc!\n", directSteps);
    return;
  }

  // 5. Đổi tốc độ cả 3 motor: SPEED <xung/giây>
  if (command == "SPEED" || command == "SPD" || command == "HZ") {
    if (param.length() > 0) {
      int val = param.toInt();
      updateSpeedAll(val);
      lastCmdText = String("SPEED ") + val + "Hz";
    } else {
      Serial.println("[HUONG DAN] Cu phap: SPEED <xung_tren_giay>. Vi du: SPEED 4000");
    }
    return;
  }

  // 6. Đổi chiều quay cả 3 motor
  if (command == "F" || command == "THUAN" || command == "CW") {
    setDirAll(HIGH);
    setLedColor(255, 200, 0);
    lastCmdText = "ALL DIR -> THUAN";
    Serial.println("[OK ALL] Da chuyen ca 3 motor sang chieu THUAN (CW)");
  }
  else if (command == "R" || command == "NGUOC" || command == "CCW") {
    setDirAll(LOW);
    setLedColor(255, 100, 0);
    lastCmdText = "ALL DIR -> NGUOC";
    Serial.println("[OK ALL] Da chuyen ca 3 motor sang chieu NGUOC (CCW)");
  }
  else if (command == "D" || command == "DAO" || command == "REV") {
    toggleDirAll();
    setLedColor(200, 200, 200);
    lastCmdText = "ALL DIR TOGGLED";
    Serial.printf("[OK ALL] Da dao chieu ca 3 motor -> M1:%s M2:%s M3:%s\n", 
                  motor1.currentDir ? "CW" : "CCW", motor2.currentDir ? "CW" : "CCW", motor3.currentDir ? "CW" : "CCW");
  }
  // 7. Chạy liên tục / Dừng cả 3 motor
  else if (command == "CONT" || command == "RUN" || command == "CHAY" || command == "GO") {
    runAllContinuous();
    lastCmdText = "ALL RUN CONT";
    Serial.printf("[RUN ALL 3M] Ca 3 Motor quay LIEN TUC\n");
  }
  else if (command == "STOP" || command == "DUNG" || command == "PAUSE" || command == "HALT") {
    stopAll();
    lastCmdText = "[STOP] DUNG CA 3 MOTOR";
    Serial.println("[PAUSE] CA 3 DONG CO DA DUNG (Khoa truc giu vi tri).");
  }
  else if (command == "HELP" || command == "?" || command == "MENU") {
    printHelp();
  }
  else {
    setLedColor(255, 0, 0);
    Serial.printf("[?] Khong nhan dien duoc lenh: '%s'. Go 'HELP' de xem huong dan.\n", cmd.c_str());
  }
}

// =============================================================================
// XỬ LÝ 8 NÚT NHẤN TRÊN MODULE TM1638
// =============================================================================
void handleButtons() {
  uint8_t currentButtons = tm_readButtons();
  uint8_t pressedButtons = currentButtons & (~lastButtonsState);
  uint8_t releasedButtons = (~currentButtons) & lastButtonsState;
  lastButtonsState = currentButtons;

  // --- 1. XỬ LÝ KHI BẮT ĐẦU NHẤN XUỐNG (PRESSED) ---
  if (pressedButtons != 0) {
    // S1: BẤM GIỮ -> M1 ĐỨNG YÊN, M2 quay NGƯỢC (CCW), M3 quay THUẬN (CW) liên tục
    if (pressedButtons & (1 << 0)) {
      currentActiveLed = 1;
      stopMotor(motor1);
      setMotorDir(motor2, LOW);  // M2: Ngược chiều kim đồng hồ (CCW)
      setMotorDir(motor3, HIGH); // M3: Đúng/Thuận chiều kim đồng hồ (CW)
      runMotorContinuous(motor2);
      runMotorContinuous(motor3);
      lastCmdText = "[S1] M2:CCW M3:CW";
      setLedColor(0, 255, 120);
      Serial.println("\n👉 [BUTTON S1 GIU]: Motor 1 DUNG | Motor 2 NGUOC (CCW) & Motor 3 THUAN (CW) - Nha nut de dung...");
    }
    // S2: BẤM GIỮ -> M1 ĐỨNG YÊN, M2 quay THUẬN (CW), M3 quay NGƯỢC (CCW) liên tục
    if (pressedButtons & (1 << 1)) {
      currentActiveLed = 2;
      stopMotor(motor1);
      setMotorDir(motor2, HIGH); // M2: Đúng/Thuận chiều kim đồng hồ (CW)
      setMotorDir(motor3, LOW);  // M3: Ngược chiều kim đồng hồ (CCW)
      runMotorContinuous(motor2);
      runMotorContinuous(motor3);
      lastCmdText = "[S2] M2:CW M3:CCW";
      setLedColor(255, 120, 0);
      Serial.println("\n👉 [BUTTON S2 GIU]: Motor 1 DUNG | Motor 2 THUAN (CW) & Motor 3 NGUOC (CCW) - Nha nut de dung...");
    }
    // S3: BẤM GIỮ -> CẢ 3 MOTOR XOAY TẠI CHỖ CHIỀU THUẬN (CW)
    if (pressedButtons & (1 << 2)) {
      currentActiveLed = 3;
      runRotateInPlaceCW(0);
    }
    // S4: BẤM GIỮ -> CẢ 3 MOTOR XOAY TẠI CHỖ CHIỀU NGƯỢC (CCW)
    if (pressedButtons & (1 << 3)) {
      currentActiveLed = 4;
      runRotateInPlaceCCW(0);
    }
    // S5: BẤM GIỮ -> CHẠY SANG PHẢI (M1: 2x CW, M2: CW, M3: CCW)
    if (pressedButtons & (1 << 4)) {
      currentActiveLed = 5;
      runRightMove(0);
    }
    // S6: BẤM GIỮ -> CHẠY SANG TRÁI (M1: 2x CCW, M2: CCW, M3: CW)
    if (pressedButtons & (1 << 5)) {
      currentActiveLed = 6;
      runLeftMove(0);
    }
    // S7: Nhấn lần đầu -> Tăng 100 Hz
    if (pressedButtons & (1 << 6)) {
      int newSpd = motor2.speedHz + 100;
      if (newSpd > 25000) newSpd = 25000;
      currentActiveLed = 7;
      tempLedTimerMs = millis() + 500;
      s7HoldStartMs = millis();
      s7LastRepeatMs = millis();
      updateSpeedAll(newSpd, true);
      lastCmdText = String("[S7] TANG +100Hz");
    }
    // S8: Nhấn lần đầu -> Giảm 100 Hz
    if (pressedButtons & (1 << 7)) {
      int newSpd = motor2.speedHz - 100;
      if (newSpd < 100) newSpd = 100;
      currentActiveLed = 8;
      tempLedTimerMs = millis() + 500;
      s8HoldStartMs = millis();
      s8LastRepeatMs = millis();
      updateSpeedAll(newSpd, true);
      lastCmdText = String("[S8] GIAM -100Hz");
    }
  }

  // --- 2. XỬ LÝ KHI NHẢ TAY RA (RELEASED) CHO S1, S2, S3, S4, S5, S6 ---
  if (releasedButtons != 0) {
    if (releasedButtons & (1 << 0)) {
      stopMotor(motor2);
      stopMotor(motor3);
      lastCmdText = "[S1] DA DUNG";
      setLedColor(0, 100, 255);
      Serial.println("👉 [BUTTON S1 NHA]: NHA NUT S1 -> Da DUNG Motor 2 & 3.");
    }
    if (releasedButtons & (1 << 1)) {
      stopMotor(motor2);
      stopMotor(motor3);
      lastCmdText = "[S2] DA DUNG";
      setLedColor(0, 100, 255);
      Serial.println("👉 [BUTTON S2 NHA]: NHA NUT S2 -> Da DUNG Motor 2 & 3.");
    }
    if (releasedButtons & (1 << 2)) {
      stopAll();
      lastCmdText = "[S3] DA DUNG";
      setLedColor(0, 100, 255);
      Serial.println("👉 [BUTTON S3 NHA]: NHA NUT S3 -> Da DUNG ca 3 Motor (Xoay CW).");
    }
    if (releasedButtons & (1 << 3)) {
      stopAll();
      lastCmdText = "[S4] DA DUNG";
      setLedColor(0, 100, 255);
      Serial.println("👉 [BUTTON S4 NHA]: NHA NUT S4 -> Da DUNG ca 3 Motor (Xoay CCW).");
    }
    if (releasedButtons & (1 << 4)) {
      stopAll();
      lastCmdText = "[S5] DA DUNG";
      setLedColor(0, 100, 255);
      Serial.println("👉 [BUTTON S5 NHA]: NHA NUT S5 -> Da DUNG ca 3 Motor (Chay Phai).");
    }
    if (releasedButtons & (1 << 5)) {
      stopAll();
      lastCmdText = "[S6] DA DUNG";
      setLedColor(0, 100, 255);
      Serial.println("👉 [BUTTON S6 NHA]: NHA NUT S6 -> Da DUNG ca 3 Motor (Chay Trai).");
    }
  }

  // --- 3. Xử lý GIỮ NÚT S7 (Tự động tăng liên tục mỗi 100 Hz) ---
  if (currentButtons & (1 << 6)) {
    if (millis() - s7HoldStartMs >= 350) {
      if (millis() - s7LastRepeatMs >= 70) {
        s7LastRepeatMs = millis();
        int newSpd = motor2.speedHz + 100;
        if (newSpd > 25000) newSpd = 25000;
        currentActiveLed = 7;
        tempLedTimerMs = millis() + 300;
        updateSpeedAll(newSpd, false);
        lastCmdText = String("[S7] ") + newSpd + "Hz";
        Serial.printf(">> [HOLD S7] Toc do: %d Hz\r", newSpd);
      }
    }
  }

  // --- 4. Xử lý GIỮ NÚT S8 (Tự động giảm liên tục mỗi 100 Hz) ---
  if (currentButtons & (1 << 7)) {
    if (millis() - s8HoldStartMs >= 350) {
      if (millis() - s8LastRepeatMs >= 70) {
        s8LastRepeatMs = millis();
        int newSpd = motor2.speedHz - 100;
        if (newSpd < 100) newSpd = 100;
        currentActiveLed = 8;
        tempLedTimerMs = millis() + 300;
        updateSpeedAll(newSpd, false);
        lastCmdText = String("[S8] ") + newSpd + "Hz";
        Serial.printf(">> [HOLD S8] Toc do: %d Hz\r", newSpd);
      }
    }
  }
}

// =============================================================================
// CẬP NHẬT MÀN HÌNH LED 7 ĐOẠN & 8 ĐÈN LED ĐỎ TRÊN TM1638
// =============================================================================
void updateTm1638Display() {
  if (isOtaUpdating) return;

  uint8_t digits[8] = {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK};
  uint8_t ledsMask = 0;

  bool runningAny = isAnyMotorRunning();
  bool contAny = motor1.isContinuousMode || motor2.isContinuousMode || motor3.isContinuousMode;

  if (millis() < tempLedTimerMs) {
    ledsMask = (1 << (currentActiveLed - 1));
  }
  else if (runningAny) {
    if (contAny) {
      currentActiveLed = 7;
    }
    ledsMask = (1 << (currentActiveLed - 1));
  } else {
    currentActiveLed = 8;
    ledsMask = (1 << 7);
  }

  if (runningAny) {
    if (contAny) {
      digits[0] = SEG_r;
      digits[1] = SEG_u;
      digits[2] = SEG_n;
      int spd = max(motor1.speedHz, max(motor2.speedHz, motor3.speedHz));
      if (spd >= 10000) {
        for (int i = 7; i >= 3; i--) {
          digits[i] = SEG_DIGITS[spd % 10];
          spd /= 10;
        }
      } else {
        digits[3] = SEG_BLANK;
        for (int i = 7; i >= 4; i--) {
          digits[i] = SEG_DIGITS[spd % 10];
          spd /= 10;
        }
      }
    } else {
      long steps = max(motor1.remainingSteps, max(motor2.remainingSteps, motor3.remainingSteps));
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
    int spd = max(motor1.speedHz, max(motor2.speedHz, motor3.speedHz));
    if (spd >= 10000) {
      digits[0] = SEG_S;
      digits[1] = SEG_P;
      digits[2] = SEG_BLANK;
      for (int i = 7; i >= 3; i--) {
        digits[i] = SEG_DIGITS[spd % 10];
        spd /= 10;
      }
    } else {
      digits[0] = SEG_S;
      digits[1] = SEG_t;
      digits[2] = SEG_o;
      digits[3] = SEG_P;
      for (int i = 7; i >= 4; i--) {
        digits[i] = SEG_DIGITS[spd % 10];
        spd /= 10;
      }
    }
  }

  tm_updateDisplay(digits, ledsMask);
}

// =============================================================================
// HIỂN THỊ MÀN HÌNH LCD 20x4 I2C (HIỂN THỊ CHI TIẾT 3 MOTOR)
// =============================================================================
void updateLcdDisplay() {
  if (!lcdFound || isOtaUpdating) return;

  char l0[21], l1[21], l2[21], l3[21];

  // Dòng 0: M1 (TRỤC CHÍNH)
  const char* m1Status = motor1.isRunning ? (motor1.isContinuousMode ? "CONT" : "RUN ") : "STOP";
  snprintf(l0, sizeof(l0), "M1[CHINH]:%4dHz [%-4s]", motor1.speedHz, m1Status);

  // Dòng 1: M2 (TRỤC PHỤ 1)
  const char* m2Status = motor2.isRunning ? (motor2.isContinuousMode ? "CONT" : "RUN ") : "STOP";
  snprintf(l1, sizeof(l1), "M2[PHU 1]:%4dHz [%-4s]", motor2.speedHz, m2Status);

  // Dòng 2: M3 (TRỤC PHỤ 2)
  const char* m3Status = motor3.isRunning ? (motor3.isContinuousMode ? "CONT" : "RUN ") : "STOP";
  snprintf(l2, sizeof(l2), "M3[PHU 2]:%4dHz [%-4s]", motor3.speedHz, m3Status);

  // Dòng 3: Lệnh thao tác gần nhất + Icon trạng thái
  snprintf(l3, sizeof(l3), "CMD:%-15s", lastCmdText.c_str());

  l0[20] = '\0';
  l1[20] = '\0';
  l2[20] = '\0';
  l3[20] = '\0';

  lcd.setCursor(0, 0);
  lcd.print(l0);

  lcd.setCursor(0, 1);
  lcd.print(l1);

  lcd.setCursor(0, 2);
  lcd.print(l2);

  lcd.setCursor(0, 3);
  lcd.print(l3);
  lcd.setCursor(19, 3);
  lcd.write(isAnyMotorRunning() ? 0 : 1);
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
const char indexHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <title>ESP32-S3 Stepper Controller - Master/Slave</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; text-align: center; margin:0; padding: 20px 10px; background: #0b1329; color: #f8fafc; }
    .container { max-width: 680px; margin: auto; }
    .card { background: #16203c; padding: 20px; border-radius: 16px; margin-bottom: 20px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); border: 1px solid #2d3748; }
    h2 { color: #38bdf8; margin: 0 0 10px 0; font-size: 22px; }
    p { color: #94a3b8; font-size: 13px; margin-bottom: 15px; }
    .status-badge { display: inline-block; padding: 4px 12px; border-radius: 20px; background: #064e3b; color: #34d399; font-size: 12px; font-weight: bold; margin-bottom: 15px; }
    .motor-grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; margin-bottom: 15px; }
    .motor-box { background: #0f172a; padding: 10px; border-radius: 10px; border: 1px solid #334155; text-align: left; }
    .motor-box.master { border-color: #38bdf8; background: #082f49; }
    .motor-title { font-weight: bold; color: #38bdf8; font-size: 12px; margin-bottom: 6px; }
    .motor-info { font-size: 11px; color: #cbd5e1; line-height: 1.4; }
    .btn-group { display: flex; flex-wrap: wrap; gap: 6px; justify-content: center; margin-top: 8px; }
    .btn { background: #0284c7; color: #fff; border: none; padding: 8px 12px; font-weight: bold; border-radius: 6px; cursor: pointer; font-size: 12px; transition: 0.2s; }
    .btn:hover { background: #0369a1; }
    .btn-stop { background: #dc2626; }
    .btn-stop:hover { background: #b91c1c; }
    .btn-run { background: #16a34a; }
    .btn-run:hover { background: #15803d; }
    .btn-warn { background: #d97706; }
    .btn-warn:hover { background: #b45309; }
    .file-box { border: 2px dashed #475569; padding: 15px; border-radius: 10px; margin-bottom: 15px; background: #0f172a; }
    input[type=file] { color: #cbd5e1; font-size: 13px; }
    .input-cmd { background: #0f172a; border: 1px solid #475569; color: #fff; padding: 8px 12px; border-radius: 6px; width: 70%; font-size: 14px; }
  </style>
</head>
<body>
  <div class="container">
    <div class="card">
      <div class="status-badge">&#9679; ESP32-S3 MASTER/SLAVE STEPPER ONLINE</div>
      <h2>HE THONG 3 DONG CO BUOC</h2>
      <p>Motor 1: TRỤC CHÍNH (MASTER) | Motor 2, 3: TRỤC PHỤ (SLAVES)</p>
      
      <div class="motor-grid">
        <div class="motor-box master">
          <div class="motor-title">&#128081; M1: TRUC CHINH</div>
          <div class="motor-info" id="m1-info">Updating...</div>
          <div class="btn-group">
            <button class="btn btn-run" onclick="sendCmd('M1 1600')">1 Vòng</button>
            <button class="btn btn-stop" onclick="sendCmd('M1 STOP')">Dừng</button>
          </div>
        </div>
        <div class="motor-box">
          <div class="motor-title">&#9881; M2: TRUC PHU 1</div>
          <div class="motor-info" id="m2-info">Updating...</div>
          <div class="btn-group">
            <button class="btn btn-run" onclick="sendCmd('M2 1600')">1 Vòng</button>
            <button class="btn btn-stop" onclick="sendCmd('M2 STOP')">Dừng</button>
          </div>
        </div>
        <div class="motor-box">
          <div class="motor-title">&#9881; M3: TRUC PHU 2</div>
          <div class="motor-info" id="m3-info">Updating...</div>
          <div class="btn-group">
            <button class="btn btn-run" onclick="sendCmd('M3 1600')">1 Vòng</button>
            <button class="btn btn-stop" onclick="sendCmd('M3 STOP')">Dừng</button>
          </div>
        </div>
      </div>

      <div style="margin-top:15px; border-top:1px solid #334155; padding-top:15px;">
        <div style="font-weight:bold; margin-bottom:8px; color:#38bdf8;">CHUC NANG TIEN / LUI (M1 DUNG, M2 & M3 QUAY NGUOC CHIEU NHAU):</div>
        <div class="btn-group">
          <button class="btn btn-run" onclick="sendCmd('TIEN 1600')">&#9650; TIẾN (M2:Ngược, M3:Thuận)</button>
          <button class="btn btn-warn" onclick="sendCmd('LUI 1600')">&#9660; LÙI (M2:Thuận, M3:Ngược)</button>
          <button class="btn btn-run" onclick="sendCmd('TIEN RUN')">Tiến Liên Tục</button>
          <button class="btn btn-warn" onclick="sendCmd('LUI RUN')">Lùi Liên Tục</button>
        </div>
      </div>

      <div style="margin-top:15px; border-top:1px solid #334155; padding-top:15px;">
        <div style="font-weight:bold; margin-bottom:8px; color:#a855f7;">CHUC NANG XOAY TAI CHO (CA 3 MOTOR CUNG QUAY):</div>
        <div class="btn-group">
          <button class="btn" style="background:#8b5cf6;" onclick="sendCmd('XOAY THUAN')">&#10227; XOAY THUẬN (CW)</button>
          <button class="btn" style="background:#ec4899;" onclick="sendCmd('XOAY NGUOC')">&#10226; XOAY NGƯỢC (CCW)</button>
          <button class="btn btn-run" onclick="sendCmd('XOAY RUN')">Xoay Thuận Liên Tục</button>
          <button class="btn btn-stop" onclick="sendCmd('STOP')">Dừng Xoay</button>
        </div>
      </div>

      <div style="margin-top:15px; border-top:1px solid #334155; padding-top:15px;">
        <div style="font-weight:bold; margin-bottom:8px; color:#06b6d4;">CHUC NANG CHAY NGANG TRAI / PHAI:</div>
        <div class="btn-group">
          <button class="btn" style="background:#0284c7;" onclick="sendCmd('TRAI 1600')">&#11013; CHẠY TRÁI (1 Vòng)</button>
          <button class="btn" style="background:#0ea5e9;" onclick="sendCmd('PHAI 1600')">&#10145; CHẠY PHẢI (1 Vòng)</button>
          <button class="btn btn-run" onclick="sendCmd('TRAI RUN')">Trái Liên Tục</button>
          <button class="btn btn-run" onclick="sendCmd('PHAI RUN')">Phải Liên Tục</button>
        </div>
      </div>

      <div style="margin-top:15px; border-top:1px solid #334155; padding-top:15px;">
        <div style="font-weight:bold; margin-bottom:8px; color:#94a3b8;">DIEU KHIEN DONG THOI CA 3 MOTOR:</div>
        <div class="btn-group">
          <button class="btn" onclick="sendCmd('1600')">Quay 1 Vòng</button>
          <button class="btn" onclick="sendCmd('3200')">Quay 2 Vòng</button>
          <button class="btn" onclick="sendCmd('400')">Quay 90°</button>
          <button class="btn btn-warn" onclick="sendCmd('D')">Đảo Chiều</button>
          <button class="btn btn-run" onclick="sendCmd('RUN')">Quay Liên Tục</button>
          <button class="btn btn-stop" onclick="sendCmd('STOP')">DỪNG TẤT CẢ</button>
        </div>
      </div>

      <div style="margin-top:15px;">
        <input type="text" id="customCmd" class="input-cmd" placeholder="Nhập lệnh (vd: SPEED 4000, M3 RUN)...">
        <button class="btn" onclick="sendCustom()">Gửi</button>
      </div>
    </div>

    <div class="card">
      <h2>OTA FIRMWARE UPDATE</h2>
      <form method='POST' action='/update' enctype='multipart/form-data'>
        <div class="file-box">
          <input type='file' name='update' required accept=".bin">
        </div>
        <input type='submit' value='TIEN HANH NAP FIRMWARE' class='btn' style="width:100%;">
      </form>
    </div>
  </div>

  <script>
    function sendCmd(c) {
      fetch('/cmd?c=' + encodeURIComponent(c))
        .then(r => r.json())
        .then(data => updateUi(data));
    }
    function sendCustom() {
      var val = document.getElementById('customCmd').value;
      if (val) {
        sendCmd(val);
        document.getElementById('customCmd').value = '';
      }
    }
    function updateUi(d) {
      if (!d) return;
      ['m1', 'm2', 'm3'].forEach(k => {
        if (d[k]) {
          document.getElementById(k + '-info').innerHTML = 
            'Trạng thái: <b>' + (d[k].running ? 'RUNNING' : 'STOP') + '</b><br>' +
            'Tốc độ: ' + d[k].speed + 'Hz (' + d[k].dir + ')<br>' +
            'Còn lại: ' + d[k].remainingSteps;
        }
      });
    }
    setInterval(() => {
      fetch('/status').then(r => r.json()).then(d => updateUi(d)).catch(e => {});
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

String buildStatusJson() {
  String json = "{";
  json += "\"status\":\"ONLINE\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"hostname\":\"" + String(HOSTNAME) + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  
  // Motor 1
  json += "\"m1\":{";
  json += "\"running\":" + String(motor1.isRunning ? "true" : "false") + ",";
  json += "\"speed\":" + String(motor1.speedHz) + ",";
  json += "\"dir\":\"" + String(motor1.currentDir ? "CW" : "CCW") + "\",";
  json += "\"totalSteps\":" + String(motor1.totalExecutedSteps) + ",";
  json += "\"remainingSteps\":" + String(motor1.remainingSteps);
  json += "},";

  // Motor 2
  json += "\"m2\":{";
  json += "\"running\":" + String(motor2.isRunning ? "true" : "false") + ",";
  json += "\"speed\":" + String(motor2.speedHz) + ",";
  json += "\"dir\":\"" + String(motor2.currentDir ? "CW" : "CCW") + "\",";
  json += "\"totalSteps\":" + String(motor2.totalExecutedSteps) + ",";
  json += "\"remainingSteps\":" + String(motor2.remainingSteps);
  json += "},";

  // Motor 3
  json += "\"m3\":{";
  json += "\"running\":" + String(motor3.isRunning ? "true" : "false") + ",";
  json += "\"speed\":" + String(motor3.speedHz) + ",";
  json += "\"dir\":\"" + String(motor3.currentDir ? "CW" : "CCW") + "\",";
  json += "\"totalSteps\":" + String(motor3.totalExecutedSteps) + ",";
  json += "\"remainingSteps\":" + String(motor3.remainingSteps);
  json += "},";

  // Tương thích ngược
  json += "\"running\":" + String(isAnyMotorRunning() ? "true" : "false") + ",";
  json += "\"speed\":" + String(motor1.speedHz) + ",";
  json += "\"dir\":\"" + String(motor1.currentDir ? "CW" : "CCW") + "\",";
  json += "\"unlocked\":" + String(isMotorsUnlocked ? "true" : "false") + ",";
  json += "\"lastCmd\":\"" + lastCmdText + "\",";
  json += "\"totalSteps\":" + String(motor1.totalExecutedSteps + motor2.totalExecutedSteps + motor3.totalExecutedSteps) + ",";
  json += "\"remainingSteps\":" + String(motor1.remainingSteps + motor2.remainingSteps + motor3.remainingSteps);
  json += "}";
  return json;
}

void setupOtaAndWebServer() {
  static bool otaServerInitialized = false;
  if (otaServerInitialized) return;
  otaServerInitialized = true;

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", indexHtml);
  });

  server.on("/status", HTTP_GET, []() {
    server.send(200, "application/json", buildStatusJson());
  });

  server.on("/cmd", HTTP_GET, []() {
    String c = "";
    if (server.hasArg("c")) c = server.arg("c");
    else if (server.hasArg("cmd")) c = server.arg("cmd");
    
    if (c.length() > 0) {
      processCommand(c);
      server.send(200, "application/json", buildStatusJson());
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
      server.send(200, "application/json", buildStatusJson());
    } else {
      server.send(400, "text/plain", "Empty command body");
    }
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    if (Update.hasError()) {
      server.send(500, "text/plain", "UPDATE THAT BAI! Vui long kiem tra file firmware .bin");
      isOtaUpdating = false;
      setLedColor(255, 0, 0);
    } else {
      server.send(200, "text/plain", "UPDATE THANH CONG! DANG KHOI DONG LAI...");
      delay(1000);
      ESP.restart();
    }
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      isOtaUpdating = true;
      stopAll();
      digitalWrite(ENA1_PIN, HIGH);
      digitalWrite(ENA2_PIN, HIGH);
      digitalWrite(ENA3_PIN, HIGH);
      setLedColor(255, 165, 0);
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
        setLedColor(0, 255, 0);
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

  ArduinoOTA.setHostname(HOSTNAME);

  ArduinoOTA.onStart([]() {
    isOtaUpdating = true;
    stopAll();
    digitalWrite(ENA1_PIN, HIGH);
    digitalWrite(ENA2_PIN, HIGH);
    digitalWrite(ENA3_PIN, HIGH);
    setLedColor(255, 165, 0);
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
    setLedColor(0, 255, 0);
    Serial.println("\n[ARDUINO-OTA] Nap thanh cong! Dang khoi dong lai...");
    if (lcdFound) {
      lcd.setCursor(0, 2);
      lcd.print("NAP THANH CONG!");
      lcd.setCursor(0, 3);
      lcd.print("Dang Reset Kit...");
    }
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (total > 0) {
      otaProgressPercent = (unsigned int)((uint64_t)progress * 100 / total);
    }
    Serial.printf("[ARDUINO-OTA] Tien do: %u%%\r", otaProgressPercent);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    isOtaUpdating = false;
    setLedColor(255, 0, 0);
    Serial.printf("\n[ARDUINO-OTA] LOI [%u]\n", error);
  });

  ArduinoOTA.begin();
}

// =============================================================================
// SETUP & LOOP CHÍNH
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // Cấu hình chân Output Motor 1, 2, 3
  pinMode(STEP1_PIN, OUTPUT);
  pinMode(DIR1_PIN, OUTPUT);
  pinMode(ENA1_PIN, OUTPUT);

  pinMode(STEP2_PIN, OUTPUT);
  pinMode(DIR2_PIN, OUTPUT);
  pinMode(ENA2_PIN, OUTPUT);

  pinMode(STEP3_PIN, OUTPUT);
  pinMode(DIR3_PIN, OUTPUT);
  pinMode(ENA3_PIN, OUTPUT);

  pinMode(2, OUTPUT);
  pinMode(48, OUTPUT);

  digitalWrite(STEP1_PIN, HIGH);
  digitalWrite(DIR1_PIN, motor1.currentDir);
  digitalWrite(ENA1_PIN, HIGH);

  digitalWrite(STEP2_PIN, HIGH);
  digitalWrite(DIR2_PIN, motor2.currentDir);
  digitalWrite(ENA2_PIN, HIGH);

  digitalWrite(STEP3_PIN, HIGH);
  digitalWrite(DIR3_PIN, motor3.currentDir);
  digitalWrite(ENA3_PIN, HIGH);

  setLedColor(0, 0, 150);

  tm_init();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);
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

  // Khởi tạo 3 Hardware Timers độc lập
  const esp_timer_create_args_t stepTimerArgs1 = {
    .callback = &onStepTimerCallback1,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "step_timer1",
    .skip_unhandled_events = true
  };
  esp_timer_create(&stepTimerArgs1, &motor1.timerHandle);
  esp_timer_start_periodic(motor1.timerHandle, 1000000ULL / (uint64_t)motor1.speedHz);

  const esp_timer_create_args_t stepTimerArgs2 = {
    .callback = &onStepTimerCallback2,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "step_timer2",
    .skip_unhandled_events = true
  };
  esp_timer_create(&stepTimerArgs2, &motor2.timerHandle);
  esp_timer_start_periodic(motor2.timerHandle, 1000000ULL / (uint64_t)motor2.speedHz);

  const esp_timer_create_args_t stepTimerArgs3 = {
    .callback = &onStepTimerCallback3,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "step_timer3",
    .skip_unhandled_events = true
  };
  esp_timer_create(&stepTimerArgs3, &motor3.timerHandle);
  esp_timer_start_periodic(motor3.timerHandle, 1000000ULL / (uint64_t)motor3.speedHz);

  printHelp();
  Serial.printf(">> Khoi dong SAN SANG 3 MOTOR!\n");
  Serial.printf("   - Motor 1: PUL=%d, DIR=%d, ENA=%d\n", STEP1_PIN, DIR1_PIN, ENA1_PIN);
  Serial.printf("   - Motor 2: PUL=%d, DIR=%d, ENA=%d\n", STEP2_PIN, DIR2_PIN, ENA2_PIN);
  Serial.printf("   - Motor 3: PUL=%d, DIR=%d, ENA=%d\n", STEP3_PIN, DIR3_PIN, ENA3_PIN);
  Serial.printf("   - TM1638: STB=%d, CLK=%d, DIO=%d | LCD 20x4: SDA=%d, SCL=%d (Found=%s)\n", 
                TM_STB_PIN, TM_CLK_PIN, TM_DIO_PIN, I2C_SDA_PIN, I2C_SCL_PIN, lcdFound ? "YES" : "NO");

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf(">> Dang ket noi WiFi: %s ...\n", WIFI_SSID);

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
  }

  Serial.println(">> Nhap so buoc (vd: 1600, M1 1600, M2 RUN, M3 800) hoac BAM NUT S1-S8:\n");
}

void loop() {
  unsigned long currentMs = millis();

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

  checkSerial();

  if (currentMs - lastTmUpdateMs >= 20) {
    lastTmUpdateMs = currentMs;
    handleButtons();
    updateTm1638Display();
  }

  if (currentMs - lastLcdUpdateMs >= 200) {
    lastLcdUpdateMs = currentMs;
    updateLcdDisplay();
  }

  if (!motor1.isRunning && !motor1.completedNotified && !motor1.isContinuousMode && motor1.targetSteps > 0) {
    motor1.completedNotified = true;
    setLedColor(255, 0, 255);
    Serial.printf("\n🎉 [HOAN THANH M1] Motor 1 da quay dung %ld buoc!\n", motor1.targetSteps);
  }

  if (!motor2.isRunning && !motor2.completedNotified && !motor2.isContinuousMode && motor2.targetSteps > 0) {
    motor2.completedNotified = true;
    setLedColor(255, 0, 255);
    Serial.printf("\n🎉 [HOAN THANH M2] Motor 2 da quay dung %ld buoc!\n", motor2.targetSteps);
  }

  if (!motor3.isRunning && !motor3.completedNotified && !motor3.isContinuousMode && motor3.targetSteps > 0) {
    motor3.completedNotified = true;
    setLedColor(255, 0, 255);
    Serial.printf("\n🎉 [HOAN THANH M3] Motor 3 da quay dung %ld buoc!\n", motor3.targetSteps);
  }

  // Nếu đang ở chế độ Mở khóa EN và tất cả motor đều không chạy: tự động nhả ENA để xoay tay tự do
  if (!isAnyMotorRunning() && isMotorsUnlocked) {
    digitalWrite(ENA1_PIN, LOW);
    digitalWrite(ENA2_PIN, LOW);
    digitalWrite(ENA3_PIN, LOW);
  }
}
