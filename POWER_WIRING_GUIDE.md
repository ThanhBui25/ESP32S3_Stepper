# CẨM NANG KỸ THUẬT: QUY TRÌNH XỬ LÝ NGUỒN, DÂY DẪN & BẤM COS TỦ ĐIỆN TỰ ĐỘNG HÓA

> 📌 **Mục đích:** Hướng dẫn toàn diện quy trình kiểm tra xử lý nguồn điện (AC 220V, DC công nghiệp, Khối Pin/Battery), kỹ thuật chọn dây, tuốt dây, bấm các loại đầu cos (Ferrules/Lugs) và quy chuẩn đi dây an toàn, gọn gàng, chống nhiễu trong hệ thống tự động hóa.

---

## 1. QUY TRÌNH & KỸ THUẬT XỬ LÝ CÁC LOẠI NGUỒN ĐIỆN

### 1.1. Xử lý Nguồn Điện Lưới AC (220VAC / 110VAC)
Nguồn AC 220V là nguồn điện động lực nguy hiểm, yêu cầu tuân thủ nghiêm ngặt quy trình an toàn điện.

* **Thiết bị bảo vệ đầu vào:**
  * **Aptomat Chống Rò / Chống Giật (ELCB / RCCB):** Chọn dòng rò ngắt $30\text{mA}$, thời gian ngắt $< 0.1\text{s}$.
  * **Cầu chì bảo vệ (Fuse):** Đặt cầu chì gốm nấc $5A - 10A$ ngay sau Aptomat để bảo vệ ngắn mạch cho các bộ nguồn xung.
* **Quy chuẩn màu dây AC:**
  * **Pha Nóng / Pha Lửa (Line - L):** Dây màu **ĐỎ** hoặc **NÂU** (Tiết diện $\ge 1.5\text{mm}^2$).
  * **Pha Nguội / Trung Tính (Neutral - N):** Dây màu **XANH DƯƠNG** hoặc **ĐEN** (Tiết diện $\ge 1.5\text{mm}^2$).
  * **Nối Đất Bảo Vệ (Protection Earth - PE / ⏚):** Dây màu **VÀNG-SỌC XANH LÁ** (Bắt buộc nối vào vỏ tủ kim loại và cọc tiếp địa).
* **Quy trình tiếp địa chống nhiễu (Star Grounding):**
  * Gom toàn bộ dây tiếp địa $PE$ từ: Vỏ nguồn Mean Well, Vỏ tủ điện, Vỏ máy CNC/Robot, Vỏ Spindle, Vỏ Card điều khiển về **1 Thanh Đồng Tiếp Địa Trung Tâm (Star Ground Bar)**.
  * Không nối tiếp địa kiểu nối tiếp nối đuôi (Daisy Chain) để tránh tạo vòng lặp dòng rò (Ground Loop).

---

### 1.2. Xử lý Nguồn Một Chiều DC Công Nghiệp (Mean Well 5V / 12V / 24V / 48V)
Nguồn DC cấp điện cho các mạch vi điều khiển (ESP32-S3, Raspberry Pi), Driver động cơ và cảm biến.

* **Phân tách 2 tuyến nguồn DC riêng biệt:**
  * **Nguồn DC Động Lực (24V / 48V):** Cấp riêng cho các Driver động cơ bước (DM542E, Best BH57), van điện từ. Dùng dây tiết diện lớn $\ge 0.75\text{mm}^2 - 1.5\text{mm}^2$.
  * **Nguồn DC Tín Hiệu / Logic (5V / 3.3V):** Cấp riêng cho ESP32-S3, Raspberry Pi 4, màn hình LCD, cảm biến.
* **Quy trình Nối Chung Mass (Common Grounding):**
  * **BẮT BỘC:** Nối tất cả các cọc âm `COM` / `-V` / `GND` của bộ nguồn 5V, 12V và 24V về **Một Thanh Đồng Mass Chung**.
  * **Tác dụng:** Giúp tất cả các tín hiệu logic ($PUL, DIR, SDA, SCL, UART$) có cùng một mức điện thế chuẩn $0\text{V}$, tránh hiện tượng trượt xung hoặc sai lệch dữ liệu.
* **Chỉnh điện áp ra qua chiết áp `+V ADJ`:**
  * Dùng đồng hồ VOM đo tại cọc nguồn: Chỉnh bộ nguồn 5V lên **$5.1\text{V} - 5.2\text{V}$** để bù sụt áp trên đường cáp nuôi Raspberry Pi 4 / ESP32. Chỉnh nguồn 24V đúng chuẩn **$24.0\text{V}$**.

---

### 1.3. Xử lý Nguồn Pin & Khối Pin sạc (Li-Ion 18650, LiFePO4, LiPo)
Dành cho các hệ thống robot di động (AGV/AMR), thiết bị đo đạc cầm tay.

* **Bắt buộc có Mạch Quản Lý Sạc Xả BMS (Battery Management System):**
  * Mạch BMS tự động cân bằng điện áp các cell pin, bảo vệ quá sạc ($> 4.25\text{V/cell}$), quá xả ($< 2.8\text{V/cell}$), quá dòng và ngắn mạch.
* **Kiểm tra dòng xả cực đại ($C\text{-Rating}$):**
  * Tính toán tổng dòng điện tiêu thụ của động cơ: Ví dụ 3 động cơ ăn $6\text{A}$, khối pin $5\text{Ah}$ phải chọn loại pin có dòng xả tối thiểu $2C$ ($5\text{Ah} \times 2 = 10\text{A}$).
* **Mạch bảo vệ chống ngược cực (Reverse Polarity Protection):**
  * Đặt 1 Diode Schottky công suất cao (như MBR20100) hoặc mạch MOSFET P-Channel ở đầu vào pin để tránh cháy mạch khi vô tình cắm ngược cực $(+) (-)$.
* **Cầu chì tự phục hồi (Resettable Fuse / PTC):**
  * Gắn cầu chì PTC nối tiếp đầu ra dương của khối pin để tự động ngắt điện khi có sự cố chập mạch cơ khí.

---

## 2. KỸ THUẬT CHỌN DÂY, TUỐT DÂY & BẤM CÁC LOẠI COS (CRIMPING TERMINALS)

### 2.1. Bảng Chọn Tiết Diện Dây Dẫn Theo Dòng Điện Tiêu Thụ

| Loại Đường Dây / Tải | Dòng Điện Max ($I$) | Tiết Diện Dây Khuyên Dùng ($S$) | Chuẩn AWG tương đương | Màu Dây Quy Chuẩn |
| :--- | :--- | :--- | :--- | :--- |
| **Nguồn Điện AC 220V** | Up to $10\text{A}$ | **$1.5\text{ mm}^2$** | AWG 16 | Pha L: Đỏ/Nâu, Pha N: Xanh/Đen |
| **Tiếp Địa PE Bảo Vệ** | Dòng rò rơ-le | **$1.5 - 2.5\text{ mm}^2$** | AWG 14 - 16 | Vàng-Sọc Xanh |
| **Nguồn Động Lực DC 24V/48V** | Up to $8\text{A}$ | **$0.75 - 1.0\text{ mm}^2$** | AWG 18 | Dương: Đỏ, Âm: Đen |
| **Cuộn Dây Động Cơ Bước (A, B)** | Up to $4\text{A}$ | **$0.5 - 0.75\text{ mm}^2$** | AWG 18 - 20 | Đen, Xanh lá, Đỏ, Xanh dương |
| **Nguồn Logic 5V / 3.3V** | Up to $3\text{A}$ | **$0.3 - 0.5\text{ mm}^2$** | AWG 20 - 22 | Dương: Cam/Hồng, Âm: Đen |
| **Tín Hiệu Cảm Biến / I2C / SPI** | Up to $100\text{mA}$ | **$0.14 - 0.2\text{ mm}^2$** | AWG 24 - 26 | Vàng, Trắng, Tím, Xanh |

---

### 2.2. Kỹ Thuật Bấm Các Loại Đầu Cos (Crimping Terminals)

#### A. Cos Kim Đơn & Cos Kim Kép (Cord End Ferrules - E Series / TE Series)
* **Ứng dụng:** Đấu nối vào cọc gài vít Terminal Block, Driver DM542E / BH57, cọc nguồn Mean Well.
* **Dụng cụ bấm:** Kìm bấm cos kim tự động 4 cạnh hoặc 6 cạnh (**HSC8 6-4 / HSC8 6-6**).
* **Quy trình thao tác:**
  1. Tuốt vỏ nhựa dây điện một đoạn chính xác **$8\text{mm} - 10\text{mm}$** (không làm đứt/xước sợi đồng nhỏ).
  2. Xoắn nhẹ lõi đồng, đút toàn bộ lõi đồng vào ống kim cos sao cho vỏ nhựa dây vừa chạm khít đai nhựa của cos.
  3. Đưa đầu cos vào ngàm kìm HSC8, bóp chặt kìm hết cỡ cho đến khi ngàm tự động nhả nấc Ratchet.
  4. Kiểm tra: Giật nhẹ dây điện, cos phải ôm chặt lõi đồng không bị trượt rời.

#### B. Cos Chữ Y / Chữ U (Fork / Spade Terminals - SV Series)
* **Ứng dụng:** Đấu nối vào cọc ốc siết vặn vít M3/M4 của nguồn Mean Well, Aptomat.
* **Dụng cụ bấm:** Kìm bấm cos bọc nhựa kiểu ép dẹp (**HS-301J / AN-03C**).
* **Quy trình thao tác:** Đút lõi đồng vào đai cos, đặt đúng nấc màu (Đỏ: 0.5-1.5mm², Xanh: 1.5-2.5mm²) trên ngàm kìm, ép chặt.

#### C. Cos Tròn (Ring Terminals - RV Series)
* **Ứng dụng:** Đấu nối cọc tiếp địa PE tủ điện, cọc ốc cọc acquy.
* **Quy trình:** Tương tự cos chữ Y nhưng cho khả năng khóa ốc chắc chắn chống sút dây 100%.

#### D. Cos Gim Dẹp Đực / Cái (Spade / Bullet Connectors - MDD / FDD Series)
* **Ứng dụng:** Đấu nối vào công tắc nguồn tổng tủ điện, chân Rơ-le kiếng 8 chân, rơ-le bán dẫn SSR.

---

### 2.3. Xử Lý Cáp Bọc Giáp Chống Nhiễu (Shielded Cable)
Dành cho cáp Encoder, cáp RS485, cáp tín hiệu UWB, SPI, I2C khi đi dây trong môi trường nhiễu công nghiệp.

* **Quy tắc Tiếp Địa 1 Đầu (One-end Grounding):**
  * Tách lớp lưới bọc kim loại (Braided Shield) ở **ĐẦU TỦ ĐIỆN**, xoắn thành 1 dây nhỏ, bọc gen co nhiệt và nối về **Thanh Đồng Tiếp Địa PE**.
  * Đầu cáp ở phía Cảm biến / Động cơ: **CẮT BỎ VÀ BỌC CÁCH ĐIỆN LỚP LƯỚI** (không nối đất đầu này).
  * **Lý do:** Tránh tạo đường vòng lặp dòng điện chạy qua lớp giáp (Ground Loop Current) làm méo tín hiệu.

---

## 3. QUY CHUẨN ĐI ĐƯỜNG DÂY & ĐỊNH HÌNH TỦ ĐIỆN AN TOÀN, GỌN GÀNG

### 3.1. Phân Tách 3 Luồng Dây Độc Lập (Triệt Nhiễu EMI)
Trong tủ điện, tuyệt đối **KHÔNG BÓ CHUNG** dây nguồn và dây tín hiệu. Bắt buộc phân tách 3 tuyến máng gen riêng biệt cách nhau tối thiểu **$10\text{cm}$**:

```text
+-------------------------------------------------------------------------+
| [TỦ ĐIỆN TỰ ĐỘNG HÓA]                                                   |
|                                                                         |
|  [MÁNG 1 - NGUỒN AC 220V]   ===> Cáp Nguồn 220V, Aptomat, Contactor     |
|                                                                         |
|  [MÁNG 2 - NGUỒN ĐỘNG LỰC]  ===> Nguồn 24V/48V, Dây Cuộn Pha Động Cơ    |
|                                                                         |
|  [MÁNG 3 - TÍN HIỆU LOGIC]  ===> Cáp Cảm Biến, I2C, SPI, UART, Encoder |
+-------------------------------------------------------------------------+
```

* Nếu 2 đường dây bắt buộc phải cắt nhau: **Phải cho cắt nhau theo góc vuông $90^\circ$** để triệt tiêu hiện tượng cảm ứng điện từ chéo.

---

### 3.2. Dụng Cụ Định Hình & Bảo Vệ Đường Dây
1. **Máng Gen Lỗ Nắp Đậy (Slotted Cable Trunking Duct):** Gắn cố định quanh mép tủ điện để giấu dây ngầm gọn gàng.
2. **Dây Xoắn Ruột Gà (Spiral Wrapping Band):** Dùng để gom bó các cụm dây đi ra ngoài khung máy chuyển động liên tục (tránh cọ xát cơ khí làm đứt vỏ dây).
3. **Ống Gen Co Nhiệt (Heat Shrink Tubing):** Luồn vào tất cả các mối hàn hoặc cổ đai cos, dùng khò nhiệt co chặt để cách điện $100\%$.
4. **Nhãn Dán Đánh Số Dây (Wire Label Tubes / EC-1, EC-2):**
   * Đánh số thứ tự (ví dụ: `24V`, `GND`, `PUL1`, `DIR1`, `A+`, `A-`...) ở cả 2 đầu dây.
   - Giúp kỹ thuật viên dễ dàng tra cứu, bảo trì và sửa chữa sự cố sau này.

---

## 4. CHECKLIST KIỂM TRA AN TOÀN TRƯỚC KHI BẬT NGUỒN (POWER-ON CHECKLIST)

- [ ] **1. Kiểm tra Cực tính (+/-):** Dùng đồng hồ VOM đo thông mạch đảm bảo nguồn $V+$ không bị chập với $V-$ / $GND$.
- [ ] **2. Kiểm tra Mức Điện áp:** Đảm bảo không cắm nhầm nguồn 24V vào đường nguồn 5V hay 3.3V của ESP32/Pi 4.
- [ ] **3. Kiểm tra Tiếp Địa (PE):** Đo điện trở giữa vỏ kim loại tủ điện và cọc PE $< 4\Omega$.
- [ ] **4. Kiểm tra Độ Chặt Đầu Cos:** Giật nhẹ từng dây điện trên Terminal Block để đảm bảo ốc siết chặt, không bị lỏng lẻo chập chờn spark điện.
- [ ] **5. Kiểm tra Ăng-ten RF:** Đảm bảo module LoRa / Wi-Fi râu đã được vặn chặt Ăng-ten trước khi cấp điện.

---
*Tài liệu chuẩn hóa quy trình thi công đấu nối hệ thống điều khiển tự động hóa công nghiệp.*
