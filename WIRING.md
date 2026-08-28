# Sơ đồ và Hướng dẫn Đấu dây (TRIPLE MOTOR WIRING GUIDE - 3 TRỤC)

## 1. Danh sách Thiết bị & Vai trò Hệ thống
- **Vi điều khiển**: ESP32-S3 DevKitC-1 (Logic 3.3V)
- **MOTOR 1 (TRỤC CHÍNH / MASTER)**: Driver Leadshine DM542E + Motor 42CM06-RD (PUL=GPIO 4, DIR=GPIO 5, ENA=GPIO 6).
- **MOTOR 2 (TRỤC PHỤ 1 / SLAVE 1)**: Driver Leadshine DM542E + Motor 42CM06-RD (PUL=GPIO 15, DIR=GPIO 16, ENA=GPIO 17).
- **MOTOR 3 (TRỤC PHỤ 2 / SLAVE 2)**: Driver Leadshine DM542E + Motor 42CM06-RD (PUL=GPIO 7, DIR=GPIO 18, ENA=GPIO 13).
- **Nguồn cấp công suất**: Mean Well LRS-100N2-24 (24VDC, ~4.5A, 108W cấp song song cho 3 Driver).
- **Module Bàn phím & LED**: TM1638 (8 phím nhấn, 8 LED 7 đoạn, 8 LED đỏ).
- **Màn hình hiển thị**: LCD 20x4 kèm Module I2C PCF8574.

---

## 2. Sơ đồ Đấu dây Chi tiết

### A. Nguồn công suất 24V (Mean Well LRS-100N2-24 -> 3 Driver DM542E)
Đấu nguồn 24V song song từ bộ nguồn Mean Well tới cả 3 Driver DM542E:

| Nguồn LRS-100N2-24 | Driver 1 (DM542E - M1) | Driver 2 (DM542E - M2) | Driver 3 (DM542E - M3) | Chức năng | Tiết diện dây |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **V+** (+24VDC) | **+V** | **+V** | **+V** | Nguồn dương động lực 24V | Dây đỏ $\ge 0.75\text{mm}^2$ |
| **V-** (0VDC / COM) | **GND** | **GND** | **GND** | Nguồn âm Mass động lực 0V | Dây đen $\ge 0.75\text{mm}^2$ |

> ⚠️ **CẢNH BÁO NGUY HIỂM VỀ ĐIỆN ÁP:**
> - Tuyệt đối **KHÔNG ĐƯỢC** nối nguồn 24V vào bất kỳ chân nào của ESP32-S3.
> - ESP32-S3 chỉ hoạt động ở mức điện áp **3.3V**.

---

### B. Tín hiệu Điều khiển Logic: ESP32-S3 $\rightarrow$ 3 Driver DM542E
Hệ thống sử dụng kiểu đấu **Cực Dương Chung (Common Anode 3.3V)**:

| Chân ESP32-S3 | Driver 1 (Motor 1) | Driver 2 (Motor 2) | Driver 3 (Motor 3) | Chức năng | Mô tả tín hiệu |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **3.3V** | **PUL1+ / DIR1+** | **PUL2+ / DIR2+** | **PUL3+ / DIR3+** | Dương Chung | Nối chung tất cả chân dương vào **3.3V** của ESP32 |
| **GPIO 4** | **PUL1-** | - | - | Xung bước Motor 1 | Kéo LOW kích xung M1 |
| **GPIO 5** | **DIR1-** | - | - | Chiều quay Motor 1 | HIGH/LOW đảo chiều M1 |
| **GPIO 6** | **ENA1-** | - | - | Enable Driver 1 | *(Có thể bỏ trống)* |
| **GPIO 15** | - | **PUL2-** | - | Xung bước Motor 2 | Kéo LOW kích xung M2 |
| **GPIO 16** | - | **DIR2-** | - | Chiều quay Motor 2 | HIGH/LOW đảo chiều M2 |
| **GPIO 17** | - | **ENA2-** | - | Enable Driver 2 | *(Có thể bỏ trống)* |
| **GPIO 7** | - | - | **PUL3-** | Xung bước Motor 3 | Kéo LOW kích xung M3 |
| **GPIO 18** | - | - | **DIR3-** | Chiều quay Motor 3 | HIGH/LOW đảo chiều M3 |
| **GPIO 13** | - | - | **ENA3-** | Enable Driver 3 | *(Có thể bỏ trống)* |

> 💡 **Mẹo:** Các chân `ENA-` và `ENA+` trên cả 3 driver có thể **RÚT RA BỎ TRỐNG**, Driver sẽ luôn luôn ở trạng thái kích hoạt (Enable) ổn định nhất.

---

### C. Đấu dây 3 Động cơ bước 42CM06-RD $\rightarrow$ 3 Driver DM542E

| Màu dây động cơ 42CM06-RD | Chân trên Driver tương ứng | Pha động cơ |
| :--- | :--- | :--- |
| **Đen (Black)** | **A+** | Cuộn dây Pha A (+) |
| **Xanh lá (Green)** | **A-** | Cuộn dây Pha A (-) |
| **Đỏ (Red)** | **B+** | Cuộn dây Pha B (+) |
| **Xanh dương (Blue)** | **B-** | Cuộn dây Pha B (-) |

---

### D. Tín hiệu Module TM1638 & LCD 20x4 I2C

| Thiết bị | Chân thiết bị | Chân ESP32-S3 | Chức năng |
| :--- | :--- | :--- | :--- |
| **TM1638** | `VCC` / `GND` | `3.3V` / `GND` | Nguồn nuôi module phím |
| | `STB` / `CLK` / `DIO` | `GPIO 10` / `GPIO 11` / `GPIO 12` | Giao tiếp 3 dây |
| **LCD 20x4 I2C** | `VCC` / `GND` | `5V` (hoặc `VIN`) / `GND` | Nguồn nuôi LCD (5V nét chữ) |
| | `SDA` / `SCL` | `GPIO 8` / `GPIO 9` | I2C Fast 400kHz |

---

### E. Cài đặt Switch DIP trên 3 Driver DM542E
- **SW1=ON, SW2=OFF, SW3=ON:** Cài dòng định mức 2.0A - 2.5A.
- **SW4=OFF:** Giảm 50% dòng khi dừng (Half Current) để mát động cơ.
- **SW5=ON, SW6=ON, SW7=OFF, SW8=ON:** Vi bước 1600 xung/vòng.

---

## 3. Bảng Lệnh Điều khiển (Serial / Monitor / Web / TM1638)

| Lệnh / Phím | Ý nghĩa | Hành động chi tiết |
| :--- | :--- | :--- |
| **BẤM GIỮ `[S1]`** / **`TIEN`** | **TIẾN LÊN (JOG)** | **Motor 1 ĐỨNG YÊN**, **Motor 2 quay NGƯỢC (CCW)** & **Motor 3 quay THUẬN (CW)**. Nhả tay dừng ngay! |
| **BẤM GIỮ `[S2]`** / **`LUI`** | **LÙI LẠI (JOG)** | **Motor 1 ĐỨNG YÊN**, **Motor 2 quay THUẬN (CW)** & **Motor 3 quay NGƯỢC (CCW)**. Nhả tay dừng ngay! |
| **BẤM GIỮ `[S3]`** / **`XOAY THUAN`** | **XOAY TẠI CHỖ THUẬN** | **Cả 3 Motor (M1, M2, M3)** cùng quay **CHIỀU THUẬN (CW)**. Nhả tay dừng ngay! |
| **BẤM GIỮ `[S4]`** / **`XOAY NGUOC`** | **XOAY TẠI CHỖ NGƯỢC** | **Cả 3 Motor (M1, M2, M3)** cùng quay **CHIỀU NGƯỢC (CCW)**. Nhả tay dừng ngay! |
| **BẤM GIỮ `[S5]`** / **`PHAI`** | **CHẠY SANG PHẢI** | **M1 (Tốc độ x2 CW)**, **M2 quay CW**, **M3 quay CCW**. Nhả tay dừng ngay! |
| **BẤM GIỮ `[S6]`** / **`TRAI`** | **CHẠY SANG TRÁI** | **M1 (Tốc độ x2 CCW)**, **M2 quay CCW**, **M3 quay CW**. Nhả tay dừng ngay! |
| **BẤM/GIỮ `[S7]`** / **`SPEED +`** | **TĂNG TỐC ĐỘ** | Tăng **+100 Hz** mỗi lần bấm (Bấm giữ tự động tăng liên tục). |
| **BẤM/GIỮ `[S8]`** / **`SPEED -`** | **GIẢM TỐC ĐỘ** | Giảm **-100 Hz** mỗi lần bấm (Bấm giữ tự động giảm liên tục). |
| **`1600`** / **`STEP 1600`** | Chạy 1600 bước | **Cả 3 Motor** (Trục chính kéo theo 2 trục phụ) |
| **`RUN`** / **`STOP`** | Chạy liên tục / Dừng | **Cả 3 Motor** |
| **`SPEED 4000`** | Đổi tốc độ | Đổi tốc độ cả 3 motor |
| **`M1 <lệnh>`** / **`MAIN <lệnh>`** | Điều khiển riêng Trục Chính | **Chỉ Motor 1** |
| **`M2 <lệnh>`** / **`SUB1 <lệnh>`** | Điều khiển riêng Trục Phụ 1 | **Chỉ Motor 2** |
| **`M3 <lệnh>`** / **`SUB2 <lệnh>`** | Điều khiển riêng Trục Phụ 2 | **Chỉ Motor 3** |
| **`SUBS <lệnh>`** / **`PHU <lệnh>`** | Điều khiển 2 Trục Phụ | **Motor 2 + Motor 3** |

---

## 4. Sơ Đồ Đấu Nối Động Cơ Servo RDS51150-270 (150kg.cm) + Nguồn Tổ Ong 12V + ESP32-S3

### A. Bảng Đấu Dây Chi Tiết:

| Đầu Cáp Servo RDS51150-270 | Chân Nguồn Tổ Ong 12V | Chân Vi Điều Khiển ESP32-S3 | Chức Năng & Quy Chuẩn |
| :--- | :--- | :--- | :--- |
| **Dây ĐỎ (VCC)** | **`V+` (+12VDC)** | *(KHÔNG NỐI vào ESP32)* | Nguồn dương động lực 12V (Tiết diện $\ge 0.75\text{mm}^2$) |
| **Dây ĐEN / NÂU (GND)** | **`COM` / `V-` (0VDC)** | **`GND` (Mass tín hiệu)** | **NỐI CHUNG MASS (Common Ground 0V)** |
| **Dây TRẮNG / VÀNG (PWM)** | *(Không nối nguồn)* | **`GPIO 4`** (hoặc GPIO 5, 6, 7...) | Tín hiệu xung PWM $50\text{Hz}$ (Mức logic 3.3V) |

---

### B. Sơ Đồ Khối Đấu Nối Trực Quan:

```text
  [NGUỒN ĐIỆN LƯỚI AC 220V]
             |
       +-----+-----+
       | L       N |
       v         v
+-----------------------------+
| NGUỒN TỔ ONG 12V MEAN WELL  |
|  [L]   [N]   [PE] [COM] [V+]|
+---------------------+----+--+
                       |    |
   +-------------------+    +----------------------+
   | (Mass 0V - Nối chung)                         | (+12V DC Nguồn động lực)
   v                                               v
+-------------------------+            +----------------------------+
| ESP32-S3 DevKitC-1      |            | SERVO RDS51150-270 (150kg) |
|                         |            |                            |
|             [GND]-------+----------->| Dây ĐEN / NÂU (GND - 0V)   |
|                         |            |                            |
|  [GPIO 4 (Xung PWM)]----+----------->| Dây TRẮNG / VÀNG (Signal)  |
|                         |            |                            |
| [USB 5V (Nguồn nuôi MCU)]|           | Dây ĐỎ (VCC - +12V) <------+
+-------------------------+            +----------------------------+
```

---

### C. Cảnh Báo & Quy Tắc Kỹ Thuật Quan Trọng:
1. ⛔ **CẤM CẤP NGUỒN 12V VÀO ESP32-S3:** Dây Đỏ 12V chỉ cấp riêng vào Dây Đỏ của Servo. Tuyệt đối không cắm 12V vào chân 5V, 3.3V hay bất kỳ chân GPIO nào của ESP32-S3.
2. 🔗 **BẮT BỘC NỐI CHUNG MASS GND (Common Ground):** Chân `COM / V-` của Nguồn 12V, chân `GND` của ESP32-S3 và Dây Đen/Nâu của Servo **bắt buộc phải gom chung vào 1 mối**. Nếu không nối chung Mass, Servo sẽ bị giật/rung lắc không kiểm soát được góc.
3. ⚡ **DÒNG ĐỈNH KHỞI ĐỘNG LỚN (Stall Current):** Servo RDS51150 tiêu thụ dòng đỉnh tới $5\text{A} - 8\text{A}$ khi tải nặng. Bộ nguồn tổ ong 12V phải chọn công suất tối thiểu **12V - 5A ($60\text{W}$)** hoặc **12V - 10A ($120\text{W}$)**.

