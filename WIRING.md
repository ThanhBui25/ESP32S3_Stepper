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
Hệ thống sử dụng kiểu đấu **Cực Dương Chung (Common Anode)**:

| Chân ESP32-S3 | Chân Driver DM542E | Chức năng | Mô tả tín hiệu |
| :--- | :--- | :--- | :--- |
| **3.3V (Nguồn Dương)** | **PUL+ / DIR+ / ENA+** | Nối Dương Chung | Nối chung 3 chân dương lại và cắm vào chân **3.3V** của ESP32-S3 |
| **GPIO 4** | **PUL-** | Xung bước (STEP/PUL) | Kéo LOW để kích xung quay |
| **GPIO 5** | **DIR-** | Chiều quay (DIR) | HIGH/LOW để đổi chiều quay |
| **GPIO 6** | **ENA-** | Bật/Tắt Driver (ENABLE) | HIGH = Bật Driver, LOW = Thả tự do (hoặc bỏ trống không cắm) |

---

## 3. Lưu ý Kỹ thuật về Tín hiệu Điều khiển 3.3V và Driver DM542E

1. **Điện áp cách ly quang (Optocoupler Input) của DM542E:**
   - Các cổng tín hiệu vào của DM542E (PUL, DIR, ENA) sử dụng optocoupler cách ly quang tích hợp sẵn điện trở hạn dòng ~270Ω (chuẩn 5V).
   - Khi đấu trực tiếp với mức **3.3V** của ESP32-S3, dòng kích qua LED opto rơi vào khoảng `(3.3V - 1.2V) / 270Ω ≈ 7.8mA`, thường đủ để kích hoạt optocoupler ở tần số thấp và trung bình.
2. **Khi nào cần Mạch chuyển đổi mức (Level Shifter / Buffer / Transistor)?**
   - Nếu động cơ bị mất bước ở tốc độ cao hoặc driver không nhận tín hiệu ổn định do dòng từ GPIO 3.3V yếu.
   - Khi muốn dùng nguồn tín hiệu chuẩn **5V** cho PUL+/DIR+/ENA+ của DM542E: **BẮT BUỘC** phải dùng IC đệm chuyển mức logic 3.3V -> 5V (như `74HCT245`, `74HCT14` hoặc module Level Shifter/Optocoupler cách ly trung gian), **KHÔNG ĐƯỢC** cấp 5V vào PUL+ rồi nối PUL- trực tiếp vào GPIO của ESP32-S3 mà không có mạch đệm bảo vệ.

---

## 4. Bảng Lệnh Điều khiển qua Serial Monitor (Baud: 115200)

Mở Serial Monitor trên máy tính (baud 115200) và gửi các lệnh sau để điều khiển động cơ trong thời gian thực:

| Lệnh nhập | Ý nghĩa | Ví dụ |
| :--- | :--- | :--- |
| **`1600`** hoặc **`STEP 1600`** | Chạy đúng số bước rồi **TỰ ĐỘNG DỪNG** | Gõ `1600` (quay 1 vòng nếu gạt 1600 vi bước), gõ `3200` (quay 2 vòng) |
| **`SPEED <số>`** | Đổi tốc độ quay theo micro giây ($\mu s$) | Gõ `SPEED 500` (rất nhanh), `SPEED 800` (nhanh), `SPEED 1500` (chuẩn) |
| **`F`** hoặc **`THUAN`** | Chọn chiều quay thuận (Forward) | Gõ `F` |
| **`R`** hoặc **`NGUOC`** | Chọn chiều quay ngược (Reverse) | Gõ `R` |
| **`D`** hoặc **`DAO`** | Tự động đảo chiều quay | Gõ `D` |
| **`CONT`** hoặc **`RUN`** | Quay LIÊN TỤC không dừng | Gõ `CONT` hoặc `RUN` |
| **`STOP`** hoặc **`DUNG`** | Dừng khẩn cấp động cơ (trục vẫn khóa) | Gõ `STOP` |
| **`HELP`** hoặc **`?`** | Xem lại bảng menu hướng dẫn | Gõ `HELP` |

---

## 5. Cài đặt Switch trên DM542E (Tham khảo)
- **SW1, SW2, SW3 (Cài đặt dòng điện - Output Current):**
  - Động cơ 42CM06-RD có dòng định mức khoảng 2.5A. Cài đặt SW1-SW3 trên DM542E ở mức RMS 2.0A - 2.5A (hoặc Peak 2.8A - 3.5A) phù hợp để tránh motor quá nóng.
- **SW4 (Chế độ Standstill Current):**
  - `OFF`: Giảm một nửa dòng khi đứng yên (Half Current - Khuyên dùng để giảm nhiệt khi motor dừng).
  - `ON`: Giữ nguyên dòng đầy đủ (Full Current).
- **SW5, SW6, SW7, SW8 (Cài đặt Vi bước - Microstep):**
  - Tùy chỉnh số xung/vòng (Pulse/rev).

