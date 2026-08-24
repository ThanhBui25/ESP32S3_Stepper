# Sơ đồ và Hướng dẫn Đấu dây (WIRING GUIDE)

## 1. Danh sách Thiết bị Phần cứng
- **Vi điều khiển**: ESP32-S3 DevKitC-1 (Logic 3.3V)
- **Driver Động cơ bước**: Leadshine DM542E
- **Động cơ bước (Stepper Motor)**: 42CM06-RD (4 dây, bước 1.8°)
- **Nguồn cấp công suất**: Mean Well LRS-100N2-24 (24VDC, ~4.5A)

---

## 2. Sơ đồ Đấu dây Chi tiết

### A. Nguồn công suất 24V (Mean Well LRS-100N2-24 -> Driver DM542E)
| Nguồn LRS-100N2-24 | Driver Leadshine DM542E | Chức năng | Ghi chú |
| :--- | :--- | :--- | :--- |
| **V+** (+24VDC) | **+V** | Cấp nguồn dương động lực | Dây đỏ / tiết diện ≥ 0.75mm² |
| **V-** (0VDC / COM) | **GND** | Nguồn âm động lực | Dây đen / tiết diện ≥ 0.75mm² |

> ⚠️ **CẢNH BÁO QUAN TRỌNG VỀ ĐIỆN ÁP:**
> - Tuyệt đối **KHÔNG ĐƯỢC** nối nguồn 24V vào bất kỳ chân nào của ESP32-S3.
> - ESP32-S3 chỉ hoạt động ở mức điện áp **3.3V**. Điện áp vượt quá 3.3V sẽ làm cháy vi điều khiển ngay lập tức.

---

### B. Đấu dây Động cơ bước 42CM06-RD -> Driver DM542E
| Màu dây động cơ 42CM06-RD | Chân trên DM542E | Pha động cơ |
| :--- | :--- | :--- |
| **Đen (Black)** | **A+** | Cuộn dây Pha A (+) |
| **Xanh lá (Green)** | **A-** | Cuộn dây Pha A (-) |
| **Đỏ (Red)** | **B+** | Cuộn dây Pha B (+) |
| **Xanh dương (Blue)** | **B-** | Cuộn dây Pha B (-) |

---

### C. Tín hiệu Điều khiển Logic (ESP32-S3 -> Driver DM542E)
Hệ thống sử dụng kiểu đấu **Cực Dương Chung (Common Anode)** cho **1 Động cơ bước**:

| Chân ESP32-S3 | Chân Driver DM542E | Chức năng | Mô tả tín hiệu |
| :--- | :--- | :--- | :--- |
| **3.3V (Nguồn Dương)** | **PUL+ / DIR+ / ENA+** | Nối Dương Chung | Nối chung 3 chân dương lại và cắm vào chân **3.3V** của ESP32-S3 |
| **GPIO 4** | **PUL-** | Xung bước (STEP/PUL) | Kéo LOW để kích xung quay |
| **GPIO 5** | **DIR-** | Chiều quay (DIR) | HIGH/LOW để đổi chiều quay |
| **GPIO 6** | **ENA-** | Bật/Tắt Driver (ENABLE) | HIGH = Bật Driver, LOW = Thả tự do (hoặc bỏ trống không cắm) |

---

### D. Tín hiệu Module Bàn phím & Màn hình TM1638 (LED & KEY)

| Chân trên TM1638 | Chân trên ESP32-S3 | Chức năng |
| :--- | :--- | :--- |
| **`VCC`** | **`3.3V`** *(hoặc 5V)* | Nguồn dương nuôi mạch |
| **`GND`** | **`GND`** | Nguồn âm đất |
| **`STB`** | **`GPIO 10`** | Chân chọn chip Strobe |
| **`CLK`** | **`GPIO 11`** | Chân xung đồng hồ Clock |
| **`DIO`** | **`GPIO 12`** | Chân dữ liệu 2 chiều Data I/O |

#### Bảng Chức Năng 8 Đèn LED Đỏ ($D_1 \rightarrow D_8$) & 8 Nút Bấm ($S_1 \rightarrow S_8$):

| Cặp Nút & LED | Chức năng Nút bấm | Trạng thái Đèn LED Đỏ hiển thị |
| :---: | :--- | :--- |
| **`S1` / `D1`** | Bấm quay **1 VÒNG** ($8000$ bước) | **`D1` sáng duy nhất** khi đang quay 1 vòng (xong tự tắt) |
| **`S2` / `D2`** | Bấm quay **2 VÒNG** ($16000$ bước) | **`D2` sáng duy nhất** khi đang quay 2 vòng (xong tự tắt) |
| **`S3` / `D3`** | Bấm quay **90 ĐỘ** ($2000$ bước) | **`D3` sáng duy nhất** khi đang quay 90 độ (xong tự tắt) |
| **`S4` / `D4`** | Bấm **ĐẢO CHIỀU** (Thuận / Ngược) | **`D4` sáng** khi Chiều Thuận (F), **`D4` tắt** khi Chiều Ngược (R) |
| **`S5` / `D5`** | Bấm **TĂNG TỐC** ($+200\text{ Hz}$) | **`D5` chớp sáng** xác nhận vừa tăng tốc độ |
| **`S6` / `D6`** | Bấm **GIẢM TỐC** ($-200\text{ Hz}$) | **`D6` chớp sáng** xác nhận vừa giảm tốc độ |
| **`S7` / `D7`** | Bấm quay **LIÊN TỤC (`RUN`)** | **`D7` sáng duy nhất** trong suốt quá trình quay liên tục |
| **`S8` / `D8`** | Bấm **DỪNG KHẨN CẤP (`STOP`)** | **`D8` sáng duy nhất** khi động cơ đang ở trạng thái DỪNG |

---

## 3. Lưu ý Kỹ thuật về Tín hiệu Điều khiển 3.3V và Driver DM542E

1. **Điện áp cách ly quang (Optocoupler Input) của DM542E:**
   - Các cổng tín hiệu vào của DM542E (PUL, DIR, ENA) sử dụng optocoupler cách ly quang tích hợp sẵn điện trở hạn dòng ~270Ω (chuẩn 5V).
   - Khi đấu trực tiếp với mức **3.3V** của ESP32-S3, dòng kích qua LED opto rơi vào khoảng `(3.3V - 1.2V) / 270Ω ≈ 7.8mA`, thường đủ để kích hoạt optocoupler ở tần số thấp và trung bình.
2. **Khi nào cần Mạch chuyển đổi mức (Level Shifter / Buffer / Transistor)?**
   - Nếu động cơ bị mất bước ở tốc độ cao hoặc driver không nhận tín hiệu ổn định do dòng từ GPIO 3.3V yếu.
   - Khi muốn dùng nguồn tín hiệu chuẩn **5V** cho PUL+/DIR+/ENA+ của DM542E: **BẮT BUỘC** phải dùng IC đệm chuyển mức logic 3.3V -> 5V (như `74HCT245`, `74HCT14` hoặc module Level Shifter/Optocoupler cách ly trung gian), **KHÔNG ĐƯỢC** cấp 5V vào PUL+ rồi nối PUL- trực tiếp vào GPIO của ESP32-S3 mà không có mạch đệm bảo vệ.

---

## 4. Bảng Lệnh Điều khiển qua Serial Monitor & Trạng thái Màu Đèn LED RGB

Mở Serial Monitor trên máy tính (baud 115200) và gửi các lệnh sau:

| Lệnh nhập | Ý nghĩa | Màu Đèn LED RGB trên ESP32 | Ví dụ |
| :--- | :--- | :---: | :--- |
| **`1600`** hoặc **`STEP 1600`** | Chạy đúng số bước rồi **TỰ ĐỘNG DỪNG** | 🩵 **Xanh ngọc (Cyan)** $\rightarrow$ 🟣 **Tím khi xong** | Gõ `1600` (chạy 1600 bước rồi dừng), `3200` |
| **`CONT`** hoặc **`RUN`** | Quay LIÊN TỤC theo tốc độ xung/giây | 🟢 **Xanh lá cây (Green)** | Gõ `RUN` hoặc `CONT` |
| **`STOP`** hoặc **`DUNG`** | Dừng khẩn cấp động cơ (khóa trục) | 🔴 **Đỏ (Red)** | Gõ `STOP` |
| **`SPEED <xung/giây>`** | Đổi tốc độ trực tiếp (Xung/s) | 🩵 **Xanh lam (Sky Blue)** | Gõ `SPEED 1600`, `SPEED 800` |
| **`F`** hoặc **`THUAN`** | Quay theo chiều THUẬN | 🟡 **Vàng (Yellow)** | Gõ `F` |
| **`R`** hoặc **`NGUOC`** | Quay theo chiều NGƯỢC | 🟠 **Cam (Orange)** | Gõ `R` |
| **`D`** hoặc **`DAO`** | Đảo chiều quay | ⚪ **Trắng (White)** | Gõ `D` |
| **`HELP`** hoặc **`?`** | Xem lại bảng menu hướng dẫn | ⚪ **Trắng (White)** | Gõ `HELP` |

---

## 5. Cài đặt Switch trên DM542E (Tham khảo)
- **SW1, SW2, SW3 (Cài đặt dòng điện - Output Current):**
  - Động cơ 42CM06-RD có dòng định mức khoảng 2.5A. Cài đặt SW1-SW3 trên DM542E ở mức RMS 2.0A - 2.5A (hoặc Peak 2.8A - 3.5A) phù hợp để tránh motor quá nóng.
- **SW4 (Chế độ Standstill Current):**
  - `OFF`: Giảm một nửa dòng khi đứng yên (Half Current - Khuyên dùng để giảm nhiệt khi motor dừng).
  - `ON`: Giữ nguyên dòng đầy đủ (Full Current).
- **SW5, SW6, SW7, SW8 (Cài đặt Vi bước - Microstep):**
  - Tùy chỉnh số xung/vòng (Pulse/rev).

