# CƠ SỞ DỮ LIỆU PHẦN CỨNG & TƯƠNG THÍCH (HARDWARE DATABASE & COMPATIBILITY)

> 📌 **Mục đích:** Tài liệu tra cứu tập trung về thông số kỹ thuật, phương pháp đấu nối, ma trận tương thích và các cảnh báo nguy hiểm khi kết hợp giữa các dòng **Vi điều khiển (MCU)**, **Driver**, **Động cơ (Motors)**, **Bộ mã hóa vị trí (Encoders)** và **Thiết bị ngoại vi**.
>
> 💡 *File được thiết kế theo dạng module hóa, có sẵn **Khung mẫu (Template)** ở cuối file để bạn dễ dàng bổ sung thêm các dòng linh kiện mới trong tương lai.*

---

## ⚡ CẤU HÌNH PHẦN CỨNG ĐANG HOẠT ĐỘNG TRONG DỰ ÁN (BASELINE HARDWARE)
*(Mọi thiết bị mới thêm vào sẽ được tự động đối chiếu tương thích với cấu hình này)*:
- **Vi điều khiển (MCU):** `ESP32-S3 DevKitC-1` (Mức Logic `3.3V`, cấp nguồn 5V USB/VIN).
- **Driver Động cơ:** `Leadshine DM542E` (Đang đấu kiểu Common Anode 3.3V).
- **Động cơ bước:** `42CM06-RD` (Động cơ bước 2 pha 4 dây, góc bước $1.8^\circ$, dòng định mức 2.5A).
- **Nguồn cấp động lực:** `Mean Well LRS-100N2-24` (24VDC, 4.5A).
- **Module hiển thị & phím bấm:** `TM1638` (8 LED đỏ, 8 LED 7 đoạn, 8 nút bấm, giao tiếp 3 chân DIO/CLK/STB).

---


## MỤC LỤC
1. [Bảng Ma Trận Tương Thích Nhanh (Compatibility Matrix)](#1-bảng-ma-trận-tương-thích-nhanh-compatibility-matrix)
   - [1.1. Ma trận MCU $\leftrightarrow$ Driver](#11-ma-trận-ghép-nối-mcu--driver)
   - [1.2. Ma trận Driver $\leftrightarrow$ Động Cơ & Encoder](#12-ma-trận-ghép-nối-driver--động-cơ--encoder)
2. [Cơ Sở Dữ Liệu Vi Điều Khiển (MCU)](#2-cơ-sở-dữ-liệu-vi-điều-khiển-mcu)
   - [2.1. ESP32-S3 DevKit](#21-esp32-s3-devkitc-1--các-bản-esp32-s3)
3. [Cơ Sở Dữ Liệu Driver Động Cơ Bước (Drivers)](#3-cơ-sở-dữ-liệu-driver-động-cơ-bước)
   - **Nhóm 1: Driver Công nghiệp Vòng Hở (Open-Loop Optocoupler)**: [Leadshine DM542/DM542E](#31-driver-leadshine-dm542--dm542e), [TB6600](#32-driver-tb6600--tb6560), [DM860/DM860H](#33-driver-leadshine-dm860--dm860h)
   - **Nhóm 2: Driver Vòng Kín / Hybrid Servo (Closed-Loop with Encoder)**: [Leadshine CL57T / CL86T / JMC 2HSS57 / MKS SERVO42C](#34-driver-vòng-kín-hybrid-servo-leadshine-cl57t-cl86t-jmc-2hss57), [Best BH57](#342-driver-vòng-kín-best-bh57-倍斯特智能)
   - **Nhóm 3: Driver Dạng Module Nhỏ (PCB StepStick)**: [TMC2209 / TMC2208](#35-driver-trinamic-tmc2209--tmc2208-silentstepstick), [A4988 / DRV8825](#36-driver-allegro-a4988--ti-drv8825)
4. [Cơ Sở Dữ Liệu Động Cơ & Bộ Mã Hóa Vòng Quay (Motors & Encoders)](#4-cơ-sở-dữ-liệu-động-cơ--bộ-mã-hóa-vòng-quay-motors--encoders)
   - [4.1. Động cơ bước 2 pha vòng hở (4 dây, 6 dây, 8 dây)](#41-động-cơ-bước-2-pha-vòng-hở-open-loop-stepper-motor)
   - [4.2. Động cơ bước vòng kín kèm Encoder (Closed-Loop Stepper / Easy Servo)](#42-động-cơ-bước-vòng-kín-kèm-encoder-closed-loop-stepper--hybrid-servo)
   - [4.3. Động cơ bước 3 pha (3-Phase Stepper)](#43-động-cơ-bước-3-pha-3-phase-stepper-motor)
   - [4.4. Bộ mã hóa vòng quay độc lập (Standalone Optical / Magnetic Encoder)](#44-bộ-mã-hóa-vòng-quay-độc-lập-standalone-optical--magnetic-encoder)
   - [4.5. Động cơ DC Servo / BLDC Servo](#45-động-cơ-dc-servo--bldc-servo)
5. [Các Chuẩn Ghép Nối & Sơ Đồ Tiêu Chuẩn](#5-các-chuẩn-ghép-nối--sơ-đồ-tiêu-chuẩn)
6. [CẢNH BÁO: Các Kết Nối Không Tương Thích & Lỗi Chết Linh Kiện](#6-cảnh-báo-những-kết-nối-không-tương-thích--lỗi-chết-linh-kiện)
7. [Khung Mẫu Bổ Sung Linh Kiện Mới (Template)](#7-khung-mẫu-bổ-sung-linh-kiện-mới-vào-file)

---

## 1. Bảng Ma Trận Tương Thích Nhanh (Compatibility Matrix)

### 1.1. Ma trận Ghép nối MCU $\leftrightarrow$ Driver:

| Driver / MCU | ESP32-S3 (3.3V) | ESP32 WROOM (3.3V) | STM32 (3.3V / 5V Tol.) | Arduino Uno/Mega (5V) | RP2040 (3.3V) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Leadshine DM542 / DM542E** | 🟢 Tốt (Common Anode 3.3V) | 🟢 Tốt (Common Anode 3.3V) | 🟢 Tốt | 🟢 Rất tốt (Chuẩn 5V) | 🟢 Tốt (Common Anode 3.3V) |
| **Leadshine CL57T / 2HSS57 (Closed-Loop)** | 🟢 Tốt (Common Anode 3.3V) | 🟢 Tốt (Common Anode 3.3V) | 🟢 Tốt | 🟢 Rất tốt | 🟢 Tốt (Common Anode 3.3V) |
| **Best BH57 (Closed-Loop 18-90V)** | 🟢 Tốt (Common Anode 3.3V / Cần đệm 5V nếu xung nhanh) | 🟢 Tốt | 🟢 Tốt | 🟢 Rất tốt (Chuẩn 5V) | 🟢 Tốt |
| **TB6600 (Vỏ đen)** | 🟡 Khá *(Cần tín hiệu rõ)* | 🟡 Khá | 🟡 Khá | 🟢 Rất tốt (5V) | 🟡 Khá |
| **DM860 / DM860H** | 🟢 Tốt (Common Anode 3.3V) | 🟢 Tốt | 🟢 Tốt | 🟢 Rất tốt | 🟢 Tốt |
| **TMC2209 / TMC2208** | 🟢 Rất tốt (VIO = 3.3V) | 🟢 Rất tốt (VIO = 3.3V) | 🟢 Rất tốt | 🟡 Cần VIO=5V hoặc chia áp | 🟢 Rất tốt (VIO = 3.3V) |
| **A4988 / DRV8825** | 🟢 Tốt (VDD = 3.3V) | 🟢 Tốt (VDD = 3.3V) | 🟢 Tốt | 🟢 Tốt (VDD = 5V) | 🟢 Tốt (VDD = 3.3V) |
| **Servo / Driver Công nghiệp (PLC 24V)** | 🔴 **KHÔNG** *(Cần Opto 24V)* | 🔴 **KHÔNG** *(Cần Opto 24V)* | 🔴 **KHÔNG** *(Cần Opto 24V)* | 🔴 **KHÔNG** *(Cần Opto)* | 🔴 **KHÔNG** |

---

### 1.2. Ma trận Ghép nối Driver $\leftrightarrow$ Động Cơ & Encoder:

| Loại Động cơ / Driver | DM542 / TB6600 / DM860 (Vòng hở 2 pha) | CL57T / 2HSS57 / HSS86 (Vòng kín 2 pha) | 3DM580 / 3HSS2208 (3 pha) | TMC2209 / A4988 / DRV8825 (Module 2 pha) | ODrive / VESC (BLDC / DC Servo) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Động cơ bước 2 pha thường (4 dây: 42CM, 57HS...)** | 🟢 **Hoàn hảo** | 🔴 **KHÔNG** *(Driver báo lỗi Alarm thiếu Encoder)* | 🔴 **KHÔNG** *(Cháy Driver/Motor)* | 🟢 **Hoàn hảo** (Nema 14/17, dòng $\le 1.5A$) | 🔴 **KHÔNG** |
| **Động cơ bước vòng kín (Có Encoder: 57CME, 42HSE...)** | 🟡 **Chạy được** *(Nhưng bỏ phí Encoder, chạy như động cơ thường)* | 🟢 **Hoàn hảo** *(Đấu đủ 4 dây Pha + Cáp Encoder)* | 🔴 **KHÔNG** *(Sai số pha)* | 🟡 **Chạy được** *(Bỏ phí Encoder)* | 🔴 **KHÔNG** |
| **Động cơ bước 3 pha (3 dây: U-V-W)** | 🔴 **CẤM** *(Cháy cuộn dây / Cháy Driver)* | 🔴 **CẤM** | 🟢 **Hoàn hảo** | 🔴 **CẤM** | 🔴 **KHÔNG** |
| **Động cơ DC Servo / BLDC không chổi than** | 🔴 **CẤM** | 🔴 **CẤM** | 🔴 **CẤM** | 🔴 **CẤM** | 🟢 **Hoàn hảo** |
| **Encoder độc lập (NPN / Line Driver)** | ⚪ *Không cắm vào Driver, nối về MCU* | 🟢 Cắm trực tiếp vào cổng Encoder Driver | 🟢 Cắm trực tiếp vào cổng Encoder Driver | ⚪ *Nối về MCU* | 🟢 Nối vào cổng Encoder của Driver |

---

## 2. Cơ Sở Dữ Liệu Vi Điều Khiển (MCU)

### 2.1. ESP32-S3 (DevKitC-1 / các bản ESP32-S3)

#### A. Thông số Kỹ thuật Cốt lõi:
- **Điện áp Logic (I/O Voltage):** `3.3V` *(Không chịu được 5V - Non-5V Tolerant)*.
- **Điện áp cấp nguồn (VIN/5V pin):** `5V DC` (qua IC giảm áp AMS1117/ME6211 xuống 3.3V).
- **Dòng ra cực đại trên mỗi GPIO:** Khuyến nghị $\le 10\text{mA}$ (Tối đa $40\text{mA}$ ngắn hạn).
- **Tổng dòng ra của toàn bộ GPIO:** Khuyến nghị $\le 120\text{mA}$.
- **Bộ tạo xung & đọc xung phần cứng:**
  - `MCPWM` / `LEDC` / `RMT`: Phát xung bước cực kỳ ổn định, không delay hệ thống.
  - `PCNT` (Pulse Counter): Đọc tín hiệu xung từ **Encoder AB** tốc độ lên đến hàng triệu xung/giây hoàn toàn bằng phần cứng mà không tốn CPU.

#### B. Phân loại Chân GPIO trên ESP32-S3 (Rất quan trọng):
| Nhóm chân | Danh sách GPIO | Lưu ý khi sử dụng |
| :--- | :--- | :--- |
| **GPIO An toàn nhất (Khuyên dùng)** | **GPIO 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21** | Dùng thoải mái cho PUL, DIR, ENA, SPI, I2C, UART, Nút bấm, Kênh Encoder A/B. |
| **Chân Strapping (Khởi động)** | **GPIO 0, GPIO 45, GPIO 46, GPIO 3** | ⚠️ **Cẩn thận:** Tránh kéo cưỡng bức xuống GND hoặc lên 3.3V lúc khởi động vì sẽ làm ESP32 rơi vào Download Mode hoặc sai điện áp Flash/PSRAM. |
| **Chân Nối Flash/PSRAM nội** | **GPIO 26 đến GPIO 32** (tùy module Octal SPI) | ⛔ **CẤM DÙNG:** Sử dụng các chân này sẽ làm treo chip tức thì. |
| **Chân USB CDC / JTAG** | **GPIO 19 (D-), GPIO 20 (D+)** | Dùng cho cổng USB Native. Không nên dùng cho Step/Dir nếu dùng USB Serial. |
| **Chân ADC2 (Analog)** | **GPIO 11 -> 20** | Trên ESP32-S3, ADC2 **có thể** dùng song song với Wi-Fi (khắc phục nhược điểm của ESP32 đời đầu). |

#### C. Lưu ý khi ghép nối:
1. **Tuyệt đối không cấp quá 3.3V vào bất kỳ chân GPIO nào.**
2. Khi dùng mạch chia áp / cảm biến 5V hoặc 24V (như cảm biến tiệm cận NPN/PNP Endstop): **Bắt buộc** qua mạch optocoupler cách ly hoặc mạch cầu phân áp ($R_1 = 2k\Omega, R_2 = 1k\Omega$).
3. Nên dùng chân `3.3V` của ESP32 làm Anode chung (`PUL+`, `DIR+`, `ENA+`) khi nối với Driver DM542/TB6600/CL57T.

---

## 3. Cơ Sở Dữ Liệu Driver Động Cơ Bước

---

### NHÓM 1: DRIVER CÔNG NGHIỆP VÒNG HỞ (OPEN-LOOP OPTOCOUPLER)

### 3.1. Driver Leadshine DM542 / DM542E

#### A. Thông số Kỹ thuật:
- **Điện áp nguồn động lực (VCC / +V):** `20V - 50V DC` (Khuyến nghị `24V` hoặc `36V`).
- **Dòng điện ngõ ra (Peak Current):** `1.0A - 4.2A` (Cài đặt qua DIP switch SW1, SW2, SW3).
- **Vi bước (Microstepping):** `400` đến `25,600` xung/vòng (SW5 - SW8).
- **Tần số xung vào tối đa:** `200 kHz`.
- **Mạch tín hiệu ngõ vào:** Sử dụng **Optocoupler cách ly quang**, điện trở hạn dòng nội **$270\Omega$** (chuẩn 5V).

#### B. Phân tích Tương thích Logic 3.3V (ESP32-S3):
- Dòng kích opto khi dùng mức 3.3V: $I = \frac{3.3V - 1.2V}{270\Omega} \approx 7.8\text{ mA}$.
- Nằm hoàn hảo trong dải làm việc ($7\text{mA} - 16\text{mA}$), hoạt động hoàn toàn ổn định với ESP32-S3 ở tần số $\le 100\text{kHz}$.

#### C. Sơ đồ Đấu nối Khuyên dùng với ESP32-S3 (Common Anode - Dương Chung):
```
ESP32-S3 (3.3V) ------------+---> PUL+ (DM542)
                            +---> DIR+ (DM542)
                            +---> ENA+ (DM542)

ESP32-S3 GPIO 4 (Xung)   -------> PUL- (DM542)  [Kéo LOW để kích sáng Opto]
ESP32-S3 GPIO 5 (Chiều)  -------> DIR- (DM542)  [LOW = Thuận, HIGH = Nghịch]
ESP32-S3 GPIO 6 (Enable) -------> ENA- (DM542)  [Bỏ trống hoặc LOW=Khóa, HIGH=Thả]
```

---

### 3.2. Driver TB6600 / TB6560

#### A. Thông số Kỹ thuật:
- **Điện áp nguồn động lực:** `9V - 40V DC` (Khuyến nghị `12V - 24V`).
- **Dòng ra:** `0.5A - 4.0A`.
- **Tín hiệu đầu vào:** Optocoupler cách ly quang trở trong $330\Omega - 510\Omega$.

#### B. Khả năng Tương thích với ESP32-S3 (3.3V):
- ⚠️ **LƯU Ý:** Các bản TB6600 clone giá rẻ dùng trở hạn dòng lớn ($510\Omega - 1k\Omega$). Khi kích bằng 3.3V, dòng qua Opto chỉ đạt $\approx 3-4\text{mA}$ $\rightarrow$ có thể bị **mất bước ở tốc độ cao**.
- **Giải pháp:** Nếu bị chập chờn, sử dụng mạch đệm chuyển mức logic `3.3V -> 5V` (IC `74HCT245` hoặc `74HCT14`).

---

### 3.3. Driver Leadshine DM860 / DM860H

#### A. Thông số Kỹ thuật:
- **Điện áp nguồn:** `24V - 80V DC` (DM860) hoặc `18V - 80V AC / 24V - 110V DC` (DM860H).
- **Dòng cực đại:** `7.2A` (Chuyên dụng cho động cơ Nema 34, Nema 42).
- **Tín hiệu điều khiển:** Tương tự DM542, hỗ trợ đấu nối Common Anode với ESP32-S3 ở mức 3.3V.

---

### NHÓM 2: DRIVER VÒNG KÍN / HYBRID SERVO (CLOSED-LOOP WITH ENCODER)

### 3.4. Driver Vòng Kín Leadshine CL57T / CL86T / JMC 2HSS57

#### A. Nguyên lý Hoạt động:
- Kết hợp giữa sự đơn giản của Động cơ bước và độ chính xác của Động cơ Servo.
- Driver nhận tín hiệu phản hồi vị trí liên tục từ **Encoder (1000 - 4000 xung/vòng)** gắn ở đuôi động cơ.
- **Không bao giờ bị mất bước:** Nếu bị kẹt tải, driver tự tăng dòng bù bước; nếu kẹt cứng vượt ngưỡng, driver lập tức ngắt động lực và kích hoạt chân cảnh báo `ALARM / FAULT` báo về MCU.

#### B. Giao diện Cổng kết nối trên Driver Vòng kín:
1. **Cổng Tín hiệu Logic (Từ MCU):**
   - `PUL+`, `PUL-`: Xung bước.
   - `DIR+`, `DIR-`: Chiều quay.
   - `ENA+`, `ENA-`: Bật/Tắt động lực.
   - `ALM+`, `ALM-`: Ngõ ra báo lỗi (Optocoupler output $\rightarrow$ nối về chân ngắt GPIO của ESP32-S3 để dừng hệ thống khẩn cấp).
   - `PEND+`, `PEND-`: Báo đã chạy đến đúng vị trí (Position In-place).
2. **Cổng Cáp Encoder (Từ Đuôi Động Cơ Bước):**
   - `EA+`, `EA-`: Kênh xung A vi sai.
   - `EB+`, `EB-`: Kênh xung B vi sai.
   - `EZ+`, `EZ-`: Kênh xung Z (Vị trí gốc 0 - Index pulse, tùy loại).
   - `VCC` (+5V) & `EGND` (0V): Nguồn nuôi mắt đọc Encoder quang do chính Driver cấp ra.
3. **Cổng Nguồn & Cuộn dây:**
   - `+V`, `GND`: Nguồn 24V - 50V DC.
   - `A+`, `A-`, `B+`, `B-`: 4 đầu dây của động cơ bước 2 pha.

---

### 3.4.2. Driver Vòng Kín Best BH57 (倍斯特智能)

#### A. Thông số Kỹ thuật & Cổng Giao tiếp:
- **Điện áp nguồn động lực (V+, V-):** `18V - 90V DC` (Khuyến nghị `24V`, `36V` hoặc `48V DC`).
- **Loại động cơ tương thích:** Động cơ bước 2 pha có gắn Encoder (`A+`, `A-`, `B+`, `B-`).
- **Cổng tín hiệu Encoder (编码器信号):** `VCC` (+5V), `EGND` (0V), `EA+`, `EA-`, `EB+`, `EB-`.
- **Cổng tín hiệu điều khiển (控制信号):** Hỗ trợ dải điện áp `5V - 24V DC` (`PUL+`, `PUL-`, `DIR+`, `DIR-`, `EN+`, `EN-`).
- **Cổng tín hiệu cảnh báo:** `ALM+`, `ALM-` (Báo lỗi khi kẹt tải hoặc mất tín hiệu Encoder).
- **Cổng tín hiệu phụ trợ:** `EX+`, `EX-` (Auxiliary expansion).
- **Cổng giao tiếp nạp thông số:** Cổng RS232.
- **Đèn LED chỉ báo:** `PWR` (Đèn xanh báo nguồn), `FLT` (Đèn đỏ báo lỗi Fault/Alarm).

#### B. Hướng dẫn Cài đặt DIP Switch (SW1 - SW8):
- **SW1 $\rightarrow$ SW4 (Vi bước - Microstepping):**
  - Cài đặt từ `400` đến `51,200` xung/vòng (hoặc `1,000` đến `40,000` xung/vòng).
- **SW5 (Chiều quay động cơ):** `OFF` = Thuận chiều kim đồng hồ (CW), `ON` = Ngược chiều kim đồng hồ (CCW).
- **SW6 (Chế độ phát xung):** `OFF` = Chế độ Xung + Chiều (PULSE + DIR - chuẩn với ESP32), `ON` = Chế độ 2 luồng xung (CW/CCW).
- **SW7, SW8 (Bộ lọc trễ xung / Filter Delay):** `0ms` (ON, ON), `4ms` (OFF, ON), `20ms` (ON, OFF), `40ms` (OFF, OFF).
#### C. Đánh giá Tương thích & Lưu ý Đấu nối với ESP32-S3:
- 🟢 **Điện áp Nguồn:** Nguồn 24V Mean Well hiện tại hoàn toàn phù hợp với dải 18-90V của BH57.
- 🟡 **Tín hiệu Logic 3.3V:** Cổng tín hiệu vào của driver thiết kế cho dải `5-24V`. Có thể đấu Anode chung `3.3V` từ ESP32 vào `PUL+/DIR+/EN+`. Nếu chạy tần số cao bị sụt xung, khuyến nghị dùng IC đệm chuyển mức logic `74HCT245` (3.3V $\rightarrow$ 5V).
- ⛔ **CẢNH BÁO QUAN TRỌNG:** Tuyệt đối **KHÔNG** cắm động cơ bước thường (như 42CM06-RD) vào driver này mà không có cáp Encoder, driver sẽ báo lỗi đèn đỏ `FLT` và ngắt bảo vệ.

#### D. Bảng Chi Tiết Tác Dụng Từng Chân Trên Driver Best BH57:

| Cụm chân | Tên chân in trên vỏ | Tên đầy đủ / Chức năng | Chi tiết tác dụng & Cách sử dụng |
| :--- | :---: | :--- | :--- |
| **Tín hiệu Điều khiển Logic (5~24V)** | **`PUL+`** | Pulse Positive (Xung dương) | Cực dương của ngõ vào tín hiệu phát xung bước (Nối vào `3.3V` của ESP32 khi đấu Dương chung). |
| | **`PUL-`** | Pulse Negative (Xung âm) | Cực âm ngõ vào tín hiệu phát xung bước. Mỗi xung kích mức LOW từ ESP32 (`GPIO 4`) sẽ làm động cơ quay 1 vi bước. |
| | **`DIR+`** | Direction Positive (Chiều dương) | Cực dương của tín hiệu đảo chiều quay (Nối vào `3.3V` của ESP32). |
| | **`DIR-`** | Direction Negative (Chiều âm) | Cực âm của tín hiệu đảo chiều quay (Nối vào `GPIO 5` của ESP32). `LOW` = Quay thuận, `HIGH` = Quay nghịch. |
| | **`EN+`** | Enable Positive (Kích hoạt dương) | Cực dương của tín hiệu bật/tắt driver (Nối vào `3.3V` của ESP32). |
| | **`EN-`** | Enable Negative (Kích hoạt âm) | Cực âm của tín hiệu bật/tắt driver (Nối `GPIO 6` hoặc **bỏ trống**). Mặc định để trống driver luôn BẬT khóa trục, kéo LOW/HIGH để ngắt lực thả tự do. |
| **Tín hiệu Báo lỗi & Mở rộng** | **`ALM+`** | Alarm Positive (Báo lỗi dương) | Cực dương ngõ ra công tắc báo lỗi Optocoupler (Nối lên nguồn `3.3V`). |
| | **`ALM-`** | Alarm Negative (Báo lỗi âm) | Cực âm ngõ ra báo lỗi (Nối vào `GPIO 7` của ESP32). Khi kẹt tải, mất tín hiệu Encoder hoặc quá dòng, chân này sẽ đóng mạch báo về MCU để dừng khẩn cấp. |
| | **`EX+`** | Expansion Positive (Phụ trợ dương) | Cực dương tín hiệu mở rộng ngoại vi (tùy chỉnh tính năng qua phần mềm RS232, bình thường để trống). |
| | **`EX-`** | Expansion Negative (Phụ trợ âm) | Cực âm tín hiệu mở rộng ngoại vi (bình thường để trống). |
| **Tín hiệu Encoder (Từ đuôi motor)** | **`VCC`** | Encoder Power (+5V) | Nguồn dương **+5V** do Driver cấp ra để nuôi mắt đọc quang/từ tính của Encoder (Dây Đỏ của Encoder). |
| | **`EGND`** | Encoder Ground (0V) | Nguồn âm Mass (0V) của mạch Encoder (Dây Trắng/Đen nhỏ của Encoder). |
| | **`EB+`** | Encoder Channel B+ | Kênh tín hiệu xung B pha dương trả về từ Encoder (Dây Vàng). |
| | **`EB-`** | Encoder Channel B- | Kênh tín hiệu xung B pha âm vi sai chống nhiễu (Dây Xanh lá). |
| | **`EA+`** | Encoder Channel A+ | Kênh tín hiệu xung A pha dương trả về từ Encoder (Dây Đen to/Nâu). |
| | **`EA-`** | Encoder Channel A- | Kênh tín hiệu xung A pha âm vi sai chống nhiễu (Dây Xanh dương). |
| **Nguồn Cấp Công Suất (18-90VDC)** | **`V+`** | DC Power Positive | Cực dương nguồn điện động lực DC (Nối vào `V+` của nguồn Mean Well 24V/36V/48V). |
| | **`V-`** | DC Power Negative (GND) | Cực âm nguồn điện động lực DC (Nối vào `V-` / `COM` của nguồn Mean Well). |
| **Động Lực Cuộn Dây Động Cơ** | **`A+`** | Motor Phase A+ | Đầu dương cuộn dây Pha A của động cơ bước (Dây Đen của động cơ). |
| | **`A-`** | Motor Phase A- | Đầu âm cuộn dây Pha A của động cơ bước (Dây Xanh lá của động cơ). |
| | **`B+`** | Motor Phase B+ | Đầu dương cuộn dây Pha B của động cơ bước (Dây Đỏ của động cơ). |
| | **`B-`** | Motor Phase B- | Đầu âm cuộn dây Pha B của động cơ bước (Dây Xanh dương của động cơ). |
| **Giao Tiếp & Đèn Báo** | **`RS232`** | Cổng giao tiếp nối tiếp | Dùng cáp chuyển đổi RS232 $\rightarrow$ USB cắm máy tính để nạp firmware, chỉnh thông số PID và cấu hình nâng cao. |
| | **`PWR`** | Đèn LED Xanh (Power) | Sáng liên tục báo nguồn hoạt động tốt / Chớp chậm ở chế độ Standby tiết kiệm điện. |
| | **`FLT`** | Đèn LED Đỏ (Fault) | Đèn báo lỗi (Sáng/Chớp khi mất tín hiệu Encoder, sai pha, quá dòng, quá nhiệt, quá áp). |

---

### NHÓM 3: DRIVER DẠNG MODULE NHỎ (STEPSTICK / PCB MODULE)

### 3.5. Driver Trinamic TMC2209 / TMC2208 (SilentStepStick)

#### A. Thông số Kỹ thuật:
- **Điện áp động lực (VMOT):** `4.75V - 28V DC` (TMC2209) / `36V` (TMC2208).
- **Điện áp logic (VIO):** `3.3V - 5V DC`.
- **Dòng điện liên tục (RMS):** `1.4A - 2.0A` (Cần tản nhiệt + quạt làm mát).
- **Công nghệ nổi bật:** `StealthChop2` (chạy siêu êm), `SpreadCycle` (mô-men xoắn cao), `StallGuard4` (phát hiện kẹt tải / Sensorless Homing).

#### B. Khả năng Tương thích với ESP32-S3:
- 🟢 **RẤT TỐT:** TMC2209 hỗ trợ chân `VIO` độc lập $\rightarrow$ Cấp `VIO` = **3.3V** từ ESP32-S3. Toàn bộ `STEP`, `DIR`, `EN`, `UART`, `DIAG` hoạt động ở chuẩn **3.3V Logic**.
- ⛔ **Tụ hóa bảo vệ VMOT:** Bắt buộc có tụ hóa **$\ge 100\mu\text{F}$ (35V-50V)** đặt ngay sát chân `VMOT` và `GND`. Thiếu tụ sẽ nổ chip khi cắm nguồn 24V!

---

### 3.6. Driver Allegro A4988 / TI DRV8825

#### A. Thông số Kỹ thuật:
- **A4988:** Nguồn `8V - 35V`, dòng thực tế `1A - 1.2A`, vi bước max `1/16`.
- **DRV8825:** Nguồn `8.2V - 45V`, dòng thực tế `1.5A`, vi bước max `1/32`.
- **Điện áp Logic (VDD):** `3.0V - 5.5V` (Cấp vào chân 3.3V của ESP32-S3).

---

## 4. Cơ Sở Dữ Liệu Động Cơ & Bộ Mã Hóa Vòng Quay (Motors & Encoders)

---

### 4.1. Động cơ bước 2 pha vòng hở (Open-Loop Stepper Motor)

#### A. Các dòng phổ biến:
- Kích thước chuẩn NEMA: **NEMA 14 (35mm), NEMA 17 (42mm - như 42CM06), NEMA 23 (57mm - 57HS), NEMA 34 (86mm)**.
- Góc bước chuẩn: `1.8°/bước` (200 bước/vòng) hoặc `0.9°/bước` (400 bước/vòng).

#### B. Phân loại theo số dây ra & Cách xác định cuộn dây:
| Số dây ra | Cách đấu dây vào Driver (A+, A-, B+, B-) | Cách xác định cặp pha |
| :--- | :--- | :--- |
| **4 Dây** (Phổ biến nhất) | Nối thẳng 2 dây pha A vào `A+/A-`, 2 dây pha B vào `B+/B-`. | Dùng đồng hồ VOM đo thông mạch (điện trở vài Ohms = 1 cặp). Hoặc chập 2 dây bất kỳ rồi quay trục bằng tay: nếu trục nặng/khựng lại $\rightarrow$ đó là 1 pha. |
| **6 Dây** (Có dây Center Tap) | Bỏ trống 2 dây giữa (Center Tap), chỉ lấy 2 đầu ngoài của mỗi pha để chạy Bipolar. | Đo điện trở: Cặp có điện trở lớn nhất chính là 2 đầu cuộn dây cần dùng. |
| **8 Dây** | Có thể đấu nối tiếp (Series) để tăng mô-men ở tốc độ thấp, hoặc đấu song song (Parallel) để chạy tốc độ cao. | Tra datasheet màu dây của hãng sản xuất. |

#### C. Khả năng kết nối Driver:
- 🟢 Tương thích: **DM542, DM542E, TB6600, DM860, TMC2209, A4988, DRV8825**.
- 🔴 **CẤM:** Không được cắm vào Driver bước 3 pha (như 3DM580) hoặc Driver BLDC.

---

### 4.2. Động cơ bước vòng kín kèm Encoder (Closed-Loop Stepper / Hybrid Servo)

#### A. Các dòng phổ biến:
- **Leadshine:** `57CME13`, `57CME23`, `86CME45`, `86CME85`...
- **JMC / MKS:** `42HSE0.6N`, `57J1880EC`, `MKS SERVO42C`...

#### B. Cấu tạo:
- Gồm thân động cơ bước 2 pha + **Bộ mã hóa Encoder quang/từ tính** tích hợp sẵn ở đuôi (Độ phân giải thường là `1000 PPR` / `4000 CPR`).
- Động cơ đưa ra **2 chùm dây riêng biệt**:
  1. **Chùm dây động lực (4 dây lớn):** Cuộn dây `A+`, `A-`, `B+`, `B-`.
  2. **Chùm dây Encoder (5 hoặc 6 dây nhỏ có bọc giáp):** `EA+`, `EA-`, `EB+`, `EB-`, `VCC (+5V)`, `EGND`.

#### C. Bảng Quy chuẩn Màu dây Encoder (Leadshine Standard):
| Tín hiệu Encoder | Màu dây tiêu chuẩn | Chân trên Driver CL57T / 2HSS57 |
| :--- | :--- | :--- |
| **`EA+`** (Kênh A Dương) | Đen (Black) | Chân `EA+` |
| **`EA-`** (Kênh A Âm) | Xanh dương (Blue) | Chân `EA-` |
| **`EB+`** (Kênh B Dương) | Vàng (Yellow) | Chân `EB+` |
| **`EB-`** (Kênh B Âm) | Xanh lá (Green) | Chân `EB-` |
| **`VCC`** (Nguồn 5V) | Đỏ (Red) | Chân `VCC` (+5V) |
| **`EGND`** (Mass 0V) | Trắng (White) | Chân `EGND` (GND) |

---

### 4.3. Động cơ bước 3 pha (3-Phase Stepper Motor)

#### A. Đặc điểm nhận dạng:
- Thường có mã **573S, 863S, 1103S**.
- Chỉ có **3 đầu dây ra: U, V, W** (hoặc A, B, C).
- Góc bước: `1.2°/bước` (chạy êm hơn và ít cộng hưởng rung hơn động cơ 2 pha).

#### B. Khả năng tương thích:
- 🟢 **Chỉ tương thích với Driver bước 3 pha:** `3DM580`, `3DM860`, `3DM2283`, `3HSS2208`.
- 🔴 **CẤM TUYỆT ĐỐI:** Không bao giờ được cắm động cơ 3 pha vào Driver 2 pha (DM542, TB6600, TMC2209) vì sẽ gây chập cuộn dây và nổ mạch cầu H của driver.

---

### 4.4. Bộ mã hóa vòng quay độc lập (Standalone Optical / Magnetic Encoder)

Dùng khi bạn muốn gắn thêm Encoder rời vào trục máy để MCU tự đọc vị trí thực tế:

| Loại Encoder | Giao thức / Tín hiệu ngõ ra | Điện áp | Cách ghép nối với ESP32-S3 | Lưu ý quan trọng |
| :--- | :--- | :--- | :--- | :--- |
| **Encoder Quang Công Nghiệp (Omron E6B2-CWZ6C)** | Xung ABZ pha lệch $90^\circ$, Ngõ ra **NPN Cực thu hở (Open Collector)** | `5V - 24V DC` | Nối chân ngõ ra qua **Điện trở kéo lên (Pull-up) $2.2k\Omega$ vào nguồn 3.3V của ESP32**, nối chân Mass chung. | ⚠️ **CẤM kéo trở lên 5V hay 24V**, sẽ làm cháy GPIO ESP32! Dùng phần cứng `PCNT` để đọc. |
| **Encoder Vi sai (Line Driver E6B2-CWZ1X)** | Tín hiệu vi sai RS422 (`A+`, `A-`, `B+`, `B-`) | `5V DC` | Cần qua IC chuyển đổi **MAX485 / DS26LS32 / 74HCT14** trước khi vào ESP32. | Chống nhiễu cực tốt khi truyền xa $> 5\text{ mét}$ trong môi trường công nghiệp. |
| **Encoder Từ Tính Tuyệt Đối (AS5600 / AS5048A)** | Giao tiếp số **$I^2C$** hoặc **SPI** (12-bit / 14-bit) | `3.3V - 5V DC` | Nối trực tiếp `SDA`, `SCL` (hoặc `MOSI`, `MISO`, `SCK`, `CS`) vào chân GPIO 3.3V của ESP32-S3. | Cần đặt nam châm xuyên tâm (Diametric Magnet) cách mặt chip $1-2\text{mm}$. |

---

### 4.5. Động cơ DC Servo / BLDC Servo

- Gồm các dòng: Động cơ DC chổi than có Encoder, Động cơ không chổi than (Brushless DC) kết hợp Cảm biến Hall + Encoder.
- 🟢 **Driver tương thích:** Các bộ điều khiển Vector FOC như `ODrive v3.6/Pro`, `SimpleFOC Shield`, `VESC 6.0`, `MKS BASE FOC`.
- 🔴 **CẤM:** Không dùng được bất kỳ driver động cơ bước nào (DM542, TB6600, CL57T) cho loại động cơ này.

---

## 5. Các Chuẩn Ghép Nối & Sơ Đồ Tiêu Chuẩn

### Chuẩn A: Sơ đồ Ghép nối Toàn diện: ESP32-S3 $\rightarrow$ Driver Vòng kín CL57T / DM542E $\rightarrow$ Động cơ có Encoder

```
+---------------------------------------------------------------------------------------+
|                                    HỆ THỐNG ĐIỀU KHIỂN                                 |
+---------------------------------------------------------------------------------------+

   [ESP32-S3 DevKit]                             [Driver Vòng Kín CL57T / DM542E]
 +-------------------+                         +---------------------------------+
 |              3.3V |------------------------>| PUL+                            |
 |                   |           +------------>| DIR+                            |
 |                   |           | +---------->| ENA+                            |
 |            GPIO 4 |-----------+ | |         | PUL- (Xung Step)                |
 |            GPIO 5 |-------------+ |         | DIR- (Chiều Direction)          |
 |            GPIO 6 |---------------+         | ENA- (Bật/Tắt Enable)           |
 |            GPIO 7 |<------------------------| ALM- (Tín hiệu Báo lỗi / Fault) |
 |                   |    (Kéo trở 3.3V)       | ALM+ (Nối 3.3V)                 |
 |               GND |------------------------>| GND (Logic)                     |
 +-------------------+                         +---------------------------------+
                                                 |   |   |   |   |   |   |   |
                                                 | CÁP ENCODER   | CÁP ĐỘNG LỰC
                                                 | (5-6 DÂY NHỎ) | (4 DÂY TO)
                                                 v   v   v   v   v   v   v   v
                                               +---------------------------------+
                                               |     ĐỘNG CƠ BƯỚC CÓ ENCODER     |
                                               |        (Ví dụ: 57CME23)         |
                                               |  [Cuộn A/B]  +  [Encoder Đuôi]  |
                                               +---------------------------------+
```

---

## 6. CẢNH BÁO: Những Kết Nối KHÔNG TƯƠNG THÍCH & Lỗi Chết Linh Kiện

| STT | Lỗi kết nối / Thao tác sai | Hậu quả | Cách phòng ngừa chuẩn |
| :---: | :--- | :--- | :--- |
| 🔴 **1** | **Cấp 5V vào PUL+/DIR+/ENA+ rồi nối PUL-/DIR- vào GPIO ESP32-S3** | Khi GPIO ở mức HIGH (3.3V), chênh áp `5V - 3.3V = 1.7V` vẫn chạy ngược vào làm **cháy chân GPIO và hỏng ESP32-S3**. | Chỉ dùng **3.3V làm Anode chung**, hoặc dùng qua Opto/Level Shifter riêng biệt. |
| 🔴 **2** | **Rút hoặc cắm dây động cơ bước khi Driver đang có điện nguồn** | Xung cảm ứng điện từ (Back-EMF hàng trăm Volts) phóng ngược về làm **cháy tức thì MOSFET công suất** của Driver. | **Luôn tắt nguồn điện 24V/12V trước khi chạm vào dây động cơ.** |
| 🔴 **3** | **Đấu ngược pha hoặc ngược kênh Encoder (A/B) trên Driver Vòng Kín** | Động cơ bị **giật rung bần bật, kêu hú dữ dội rồi nhảy Alarm ngắt mạch** do hồi tiếp dương. | Đảo vị trí 2 dây của pha A (`A+` sang `A-`) hoặc đảo chiều gạt switch Encoder. |
| 🔴 **4** | **Cắm Động cơ bước 3 pha (3 dây U-V-W) vào Driver 2 pha (DM542, TB6600)** | Chập cuộn dây, **nổ cầu H MOSFET của Driver**. | Động cơ 3 pha bắt buộc phải dùng Driver 3 pha chuyên dụng (`3DM580`). |
| 🔴 **5** | **Đảo ngược cực nguồn động lực (+V và GND / VMOT và GND)** | Chập diode bảo vệ, **nổ tụ điện và cháy toàn bộ mạch công suất**. | Luôn dùng đồng hồ VOM đo đúng cực tính (+ Đỏ, - Đen) trước khi bật nguồn. |
| 🔴 **6** | **Module TMC2209/A4988 thiếu tụ điện VMOT (100uF)** | Xung áp nhọn (LC voltage spike) khi bật nguồn đột ngột sẽ **đánh thủng chip Driver**. | Hàn tụ hóa $\ge 100\mu\text{F}$ $35\text{V}-50\text{V}$ ngay sát 2 chân VMOT & GND. |
| 🔴 **7** | **Kéo tín hiệu Encoder NPN công nghiệp (5V - 24V) thẳng vào GPIO** | Điện áp vượt 3.3V làm **cháy lập tức chân ngắt / chân đọc xung của ESP32-S3**. | Chỉ dùng **trở kéo lên nguồn 3.3V của ESP32**, không kéo lên nguồn ngoài. |

---

## 7. Khung Mẫu Bổ Sung Linh Kiện Mới Vào File (Template)

> 📋 *Khi bạn cần thêm một MCU, Driver, Động Cơ hoặc Cảm Biến mới vào file này, chỉ cần sao chép khung mẫu bên dưới và điền thông tin:*

```markdown
### X.X. Tên Thiết Bị Mới (MCU / Driver / Motor / Encoder)

#### A. Thông số Kỹ thuật:
- **Loại thiết bị:** `(MCU / Driver 2 pha / Driver 3 pha / Động cơ / Encoder)`
- **Điện áp hoạt động / Điện áp Logic:** `...`
- **Điện áp ngõ vào / Nguồn cấp động lực:** `...`
- **Dòng điện làm việc định mức / cực đại:** `...`
- **Chuẩn giao tiếp / Tín hiệu hỗ trợ:** `(Step/Dir, NPN/PNP, RS422, I2C, SPI, UART, v.v.)`

#### B. Khả năng Tương thích:
- **Tương thích với ESP32-S3:** `🟢 Tốt / 🟡 Có lưu ý / 🔴 Cấm cắm trực tiếp`
- **Driver / Động cơ ghép nối phù hợp:** `...`
- **Loại KHÔNG ĐƯỢC kết nối cùng:** `...`

#### C. Sơ đồ Đấu nối Khuyên dùng:
- `Chân Thiết bị 1 ...` ---> `Chân Thiết bị 2 ...`
- `Chân Thiết bị 1 ...` ---> `Chân Thiết bị 2 ...`

#### D. Cảnh báo & Lưu ý Đặc thù:
- ⚠️ `Lưu ý vận hành...`
- ⛔ `Cảnh báo nguy hiểm chống cháy nổ...`
```

---
*Tài liệu được cập nhật toàn diện phục vụ thiết kế hệ thống điều khiển tự động hóa chính xác cao.*
