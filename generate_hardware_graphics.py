import os
from PIL import Image, ImageDraw, ImageFont

img_dir = r"d:\Luu\images"
os.makedirs(img_dir, exist_ok=True)

def get_font(size=14, bold=False):
    # Load TrueType font from Windows supporting full UTF-8 Vietnamese
    font_paths = [
        r"C:\Windows\Fonts\segoeuib.ttf" if bold else r"C:\Windows\Fonts\segoeui.ttf",
        r"C:\Windows\Fonts\arialbd.ttf" if bold else r"C:\Windows\Fonts\arial.ttf",
        r"C:\Windows\Fonts\tahoma.ttf"
    ]
    for p in font_paths:
        if os.path.exists(p):
            try:
                return ImageFont.truetype(p, int(size))
            except Exception:
                pass
    return ImageFont.truetype("arial.ttf", int(size))


def draw_header(draw, width, title, subtitle, category="HARDWARE REFERENCE"):
    # Header bar
    draw.rectangle([0, 0, width, 55], fill="#1E3A8A")
    draw.text((15, 8), category, fill="#93C5FD", font=get_font(11, bold=True))
    draw.text((15, 24), title, fill="#FFFFFF", font=get_font(18, bold=True))
    
    # Subtitle badge
    if subtitle:
        draw.rounded_rectangle([width - 240, 12, width - 15, 42], radius=6, fill="#2563EB")
        draw.text((width - 230, 18), subtitle, fill="#FFFFFF", font=get_font(12, bold=True))

# 1. ESP32-S3 DevKit Card
def make_esp32s3_card():
    w, h = 760, 420
    im = Image.new("RGB", (w, h), "#F8FAFC")
    d = ImageDraw.Draw(im)
    draw_header(d, w, "ESP32-S3 DevKitC-1 (N16R8)", "Logic: 3.3V | Dual Core 240MHz", "MICROCONTROLLER (MCU)")
    
    # Card outer border
    d.rectangle([0, 0, w-1, h-1], outline="#CBD5E1", width=1)
    
    # ESP32 Board representation (Center)
    bx, by, bw, bh = 220, 80, 320, 290
    d.rounded_rectangle([bx, by, bx+bw, by+bh], radius=12, fill="#1E293B", outline="#0EA5E9", width=2)
    
    # SoC Module
    d.rounded_rectangle([bx+50, by+60, bx+bw-50, by+bh-90], radius=6, fill="#0F172A", outline="#94A3B8", width=1)
    d.text((bx+75, by+75), "ESPRESSIF", fill="#E2E8F0", font=get_font(14, bold=True))
    d.text((bx+65, by+100), "ESP32-S3-WROOM-1", fill="#38BDF8", font=get_font(13, bold=True))
    d.text((bx+85, by+125), "Wi-Fi & BLE 5.0", fill="#94A3B8", font=get_font(11))
    
    # Dual Type-C ports
    d.rounded_rectangle([bx+40, by+bh-35, bx+110, by+bh+10], radius=4, fill="#64748B")
    d.text((bx+48, by+bh-25), "USB-OTG", fill="#FFFFFF", font=get_font(10, bold=True))
    
    d.rounded_rectangle([bx+bw-110, by+bh-35, bx+bw-40, by+bh+10], radius=4, fill="#64748B")
    d.text((bx+bw-102, by+bh-25), "UART", fill="#FFFFFF", font=get_font(10, bold=True))
    
    # RGB LED
    d.ellipse([bx+bw//2-10, by+30, bx+bw//2+10, by+50], fill="#10B981", outline="#FFFFFF", width=1)
    d.text((bx+bw//2-25, by+12), "RGB GPIO48", fill="#34D399", font=get_font(9, bold=True))
    
    # Left Pinout (Used in project)
    d.rounded_rectangle([15, 75, 195, 395], radius=8, fill="#FFFFFF", outline="#E2E8F0", width=1)
    d.text((25, 85), "CHÂN ĐIỀU KHIỂN ĐÃ DÙNG", fill="#1E3A8A", font=get_font(11, bold=True))
    
    pins_left = [
        ("3.3V", "Nguồn Dương Chung Opto", "#2563EB"),
        ("GND", "Mass đất hệ thống", "#475569"),
        ("GPIO 4", "Xung Bước -> PUL-", "#16A34A"),
        ("GPIO 5", "Chiều Quay -> DIR-", "#D97706"),
        ("GPIO 6", "Bật/Tắt -> ENA-", "#9333EA"),
        ("GPIO 7", "Báo lỗi <- ALM-", "#DC2626"),
        ("GPIO 10", "Strobe -> TM1638", "#0891B2"),
        ("GPIO 11", "Clock -> TM1638", "#0891B2"),
        ("GPIO 12", "Data -> TM1638", "#0891B2"),
    ]
    
    py = 110
    for p_name, p_desc, p_col in pins_left:
        d.rounded_rectangle([25, py, 78, py+24], radius=4, fill=p_col)
        d.text((30, py+4), p_name, fill="#FFFFFF", font=get_font(10, bold=True))
        d.text((85, py+4), p_desc, fill="#334155", font=get_font(10))
        py += 31

    # Right Box: Safety Warnings
    d.rounded_rectangle([565, 75, 745, 395], radius=8, fill="#FEF2F2", outline="#FCA5A5", width=1)
    d.text((575, 85), "CẢNH BÁO AN TOÀN", fill="#DC2626", font=get_font(12, bold=True))
    
    warns = [
        "1. Điện áp GPIO: Max 3.3V.",
        "   CẤM cấp 5V hoặc 24V",
        "   vào bất kỳ chân nào.",
        "2. Chân Strapping cần tránh:",
        "   GPIO 0, 3, 45, 46.",
        "3. Dòng mỗi chân <= 10mA.",
        "4. Cổng USB kép: dùng cổng",
        "   UART (COM port) để nạp",
        "   và xem Serial Monitor."
    ]
    wy = 115
    for w_line in warns:
        d.text((575, wy), w_line, fill="#7F1D1D" if "1." in w_line or "CẤM" in w_line else "#991B1B", font=get_font(10.5, bold=("CẤM" in w_line)))
        wy += 24

    im.save(os.path.join(img_dir, "esp32s3_card.png"))
    print("Generated esp32s3_card.png")

# 2. Driver Leadshine DM542E Card
def make_dm542e_card():
    w, h = 760, 420
    im = Image.new("RGB", (w, h), "#F8FAFC")
    d = ImageDraw.Draw(im)
    draw_header(d, w, "Leadshine DM542E Driver", "Điện áp: 20-50VDC | Max 4.2A", "STEPPER DRIVER (VÒNG HỞ)")
    d.rectangle([0, 0, w-1, h-1], outline="#CBD5E1", width=1)
    
    # Driver casing (Center)
    dx, dy, dw, dh = 240, 75, 280, 325
    d.rounded_rectangle([dx, dy, dx+dw, dy+dh], radius=10, fill="#1E293B", outline="#475569", width=2)
    
    # Metal heatsink fins
    for fx in range(dx+20, dx+dw-20, 20):
        d.line([(fx, dy+15), (fx, dy+dh-15)], fill="#334155", width=3)
        
    # Front label plate
    d.rounded_rectangle([dx+40, dy+25, dx+dw-40, dy+dh-25], radius=6, fill="#0F172A", outline="#64748B", width=1)
    d.text((dx+55, dy+35), "Leadshine", fill="#FFFFFF", font=get_font(16, bold=True))
    d.text((dx+55, dy+60), "DM542E", fill="#38BDF8", font=get_font(20, bold=True))
    d.text((dx+55, dy+90), "Digital Stepper Driver", fill="#94A3B8", font=get_font(11))
    
    # LED indicator
    d.ellipse([dx+55, dy+120, dx+70, dy+135], fill="#22C55E")
    d.text((dx+80, dy+122), "PWR (Power)", fill="#86EFAC", font=get_font(10, bold=True))
    d.ellipse([dx+55, dy+145, dx+70, dy+160], fill="#EF4444")
    d.text((dx+80, dy+147), "ALM (Alarm)", fill="#FCA5A5", font=get_font(10, bold=True))
    
    # DIP Switch illustration
    d.rounded_rectangle([dx+55, dy+180, dx+dw-55, dy+250], radius=4, fill="#1E293B", outline="#94A3B8")
    d.text((dx+65, dy+190), "DIP SWITCH: SW1 - SW8", fill="#F1F5F9", font=get_font(10, bold=True))
    d.text((dx+65, dy+210), "SW1-3: Dòng điện (1.0A-4.2A)", fill="#CBD5E1", font=get_font(9.5))
    d.text((dx+65, dy+228), "SW5-8: Vi bước (400-25600)", fill="#CBD5E1", font=get_font(9.5))
    
    # Left Terminals: Logic Signals (ESP32-S3)
    d.rounded_rectangle([15, 75, 215, 395], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((25, 85), "CỔNG TÍN HIỆU LOGIC", fill="#1E3A8A", font=get_font(11, bold=True))
    
    term_logic = [
        ("PUL+", "Nối vào 3.3V ESP32", "#2563EB"),
        ("PUL-", "Xung bước (GPIO 4)", "#16A34A"),
        ("DIR+", "Nối vào 3.3V ESP32", "#2563EB"),
        ("DIR-", "Chiều quay (GPIO 5)", "#D97706"),
        ("ENA+", "Nối vào 3.3V (hoặc bỏ trống)", "#94A3B8"),
        ("ENA-", "Bật/Tắt (GPIO 6 / bỏ trống)", "#94A3B8"),
    ]
    ty = 115
    for t_name, t_desc, t_col in term_logic:
        d.rounded_rectangle([25, ty, 75, ty+26], radius=4, fill=t_col)
        d.text((32, ty+5), t_name, fill="#FFFFFF", font=get_font(11, bold=True))
        d.text((82, ty+5), t_desc, fill="#334155", font=get_font(10))
        ty += 43
        
    # Right Terminals: Power & Motor
    d.rounded_rectangle([545, 75, 745, 395], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((555, 85), "NGUỒN & ĐỘNG CƠ", fill="#1E3A8A", font=get_font(11, bold=True))
    
    term_pwr = [
        ("GND", "Mass Nguồn 24V (COM)", "#0F172A"),
        ("+V", "Nguồn Dương 24VDC", "#DC2626"),
        ("A+", "Pha A Động cơ (Dây Đen)", "#1E293B"),
        ("A-", "Pha A Động cơ (Dây Xanh lá)", "#16A34A"),
        ("B+", "Pha B Động cơ (Dây Đỏ)", "#DC2626"),
        ("B-", "Pha B Động cơ (Dây Xanh dương)", "#2563EB"),
    ]
    py = 115
    for p_name, p_desc, p_col in term_pwr:
        d.rounded_rectangle([555, py, 605, py+26], radius=4, fill=p_col)
        d.text((563, py+5), p_name, fill="#FFFFFF", font=get_font(11, bold=True))
        d.text((612, py+5), p_desc, fill="#334155", font=get_font(10))
        py += 43

    im.save(os.path.join(img_dir, "dm542e_card.png"))
    print("Generated dm542e_card.png")

# 3. Driver Best BH57 Closed Loop Card
def make_bh57_card():
    w, h = 760, 440
    im = Image.new("RGB", (w, h), "#F8FAFC")
    d = ImageDraw.Draw(im)
    draw_header(d, w, "Best BH57 (倍斯特智能)", "Dải điện áp: 18-90VDC | Closed-Loop Hybrid Servo", "HYBRID SERVO DRIVER (VÒNG KÍN)")
    d.rectangle([0, 0, w-1, h-1], outline="#CBD5E1", width=1)
    
    # Driver front layout
    dx, dy, dw, dh = 230, 70, 300, 355
    d.rounded_rectangle([dx, dy, dx+dw, dy+dh], radius=10, fill="#111827", outline="#10B981", width=2)
    
    # Brand
    d.text((dx+20, dy+15), "Best 倍斯特智能", fill="#34D399", font=get_font(18, bold=True))
    d.text((dx+20, dy+45), "BH57", fill="#FFFFFF", font=get_font(24, bold=True))
    d.text((dx+200, dy+20), "RS232", fill="#94A3B8", font=get_font(11, bold=True))
    d.ellipse([dx+255, dy+18, dx+268, dy+31], fill="#22C55E")
    d.ellipse([dx+275, dy+18, dx+288, dy+31], fill="#EF4444")
    
    # Terminal group visual (Green connector)
    d.rounded_rectangle([dx+18, dy+85, dx+dw-18, dy+dh-20], radius=6, fill="#064E3B", outline="#10B981")
    d.text((dx+28, dy+95), "CÁC CỔNG GIAO TIẾP TÍCH HỢP:", fill="#A7F3D0", font=get_font(12, bold=True))
    
    cats = [
        ("控制信号 (5~24V):", "PUL+, PUL-, DIR+, DIR-, EN+, EN-"),
        ("报警 / 辅助信号:", "ALM+, ALM-, EX+, EX-"),
        ("编码器 (Encoder):", "VCC, EGND, EB+, EB-, EA+, EA-"),
        ("电源 (Power):", "V+ (+24~90V), V- (0V/COM)"),
        ("电机 (Motor 2-Ph):", "A+, A-, B+, B-")
    ]
    cy = dy + 125
    for c_title, c_val in cats:
        d.text((dx+28, cy), c_title, fill="#FCD34D", font=get_font(11, bold=True))
        d.text((dx+28, cy+18), c_val, fill="#FFFFFF", font=get_font(10.5))
        cy += 42

    # Left: Signal & Encoder Guide
    d.rounded_rectangle([15, 70, 215, 420], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((25, 80), "CÁP ENCODER ĐỘNG CƠ", fill="#1E3A8A", font=get_font(11, bold=True))
    
    enc_pins = [
        ("VCC", "+5V Nuôi mắt quang (Đỏ)", "#DC2626"),
        ("EGND", "0V Mass (Trắng/Đen)", "#0F172A"),
        ("EB+", "Kênh B+ (Dây Vàng)", "#D97706"),
        ("EB-", "Kênh B- (Dây Xanh lá)", "#16A34A"),
        ("EA+", "Kênh A+ (Dây Đen to)", "#1E293B"),
        ("EA-", "Kênh A- (Dây Xanh dương)", "#2563EB"),
    ]
    ey = 108
    for en_name, en_desc, en_col in enc_pins:
        d.rounded_rectangle([25, ey, 75, ey+24], radius=4, fill=en_col)
        d.text((30, ey+4), en_name, fill="#FFFFFF", font=get_font(10, bold=True))
        d.text((82, ey+4), en_desc, fill="#334155", font=get_font(9.5))
        ey += 35
        
    d.text((25, 325), "⚠️ CHÚ Ý QUAN TRỌNG:", fill="#DC2626", font=get_font(10.5, bold=True))
    d.text((25, 345), "Nếu chạy bị giật và đỏ đèn", fill="#991B1B", font=get_font(9.5))
    d.text((25, 362), "FLT: Đảo chỗ 2 dây A+ & A-", fill="#991B1B", font=get_font(9.5, bold=True))
    d.text((25, 380), "để sửa ngược pha Encoder.", fill="#991B1B", font=get_font(9.5))

    # Right: Switch Settings
    d.rounded_rectangle([545, 70, 745, 420], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((555, 80), "CÀI ĐẶT SWITCH SW1-SW8", fill="#1E3A8A", font=get_font(11, bold=True))
    
    sw_info = [
        "SW1-SW4 (Vi bước):",
        "  • 1600: ON, OFF, ON, ON",
        "  • 3200: OFF, OFF, ON, ON",
        "  • 8000: ON, ON, OFF, OFF",
        "SW5 (Chiều quay):",
        "  • OFF = CW, ON = CCW",
        "SW6 (Chế độ nhận xung):",
        "  • OFF = PULSE + DIR",
        "    (Bắt buộc với ESP32)",
        "SW7, SW8 (Lọc trễ xung):",
        "  • ON, ON = 0ms (Trực tiếp)"
    ]
    sy = 108
    for s_line in sw_info:
        is_h = "SW" in s_line and "(" in s_line
        d.text((555, sy), s_line, fill="#1E3A8A" if is_h else "#334155", font=get_font(10 if not is_h else 10.5, bold=is_h))
        sy += 21

    im.save(os.path.join(img_dir, "bh57_card.png"))
    print("Generated bh57_card.png")

# 4. Stepper Motor with Gearbox Card
def make_motor_gearbox_card():
    w, h = 760, 380
    im = Image.new("RGB", (w, h), "#F8FAFC")
    d = ImageDraw.Draw(im)
    draw_header(d, w, "Động cơ bước 42CM06-RD + Hộp số giảm tốc", "NEMA 17 | 1.8°/bước | Planetary Gearbox", "STEPPER MOTOR")
    d.rectangle([0, 0, w-1, h-1], outline="#CBD5E1", width=1)
    
    # Motor illustration (Center)
    mx, my, mw, mh = 250, 75, 260, 280
    # Shaft
    d.rounded_rectangle([mx+mw//2-12, my+10, mx+mw//2+12, my+60], radius=4, fill="#94A3B8", outline="#475569")
    # Gearbox (Silver round)
    d.rounded_rectangle([mx+20, my+60, mx+mw-20, my+150], radius=16, fill="#E2E8F0", outline="#64748B", width=2)
    d.text((mx+55, my+95), "PLANETARY GEARBOX", fill="#475569", font=get_font(12, bold=True))
    # Stepper Body (Black square)
    d.rounded_rectangle([mx+25, my+155, mx+mw-25, my+260], radius=6, fill="#1E293B", outline="#0F172A", width=2)
    d.text((mx+65, my+185), "42CM06-RD", fill="#38BDF8", font=get_font(16, bold=True))
    d.text((mx+65, my+215), "2-Phase 4-Wire 2.5A", fill="#94A3B8", font=get_font(11))
    
    # 4-wire cable coming out
    d.line([(mx+mw-25, my+230), (mx+mw+30, my+230)], fill="#0F172A", width=6)
    
    # Left: Wire Color Legend
    d.rounded_rectangle([15, 75, 225, 360], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((25, 85), "BẢNG MÃ MÀU 4 DÂY ĐỘNG CƠ", fill="#1E3A8A", font=get_font(11, bold=True))
    
    motor_wires = [
        ("A+", "Dây ĐEN (Black)", "Cuộn Pha A (+)", "#1E293B"),
        ("A-", "Dây XANH LÁ (Green)", "Cuộn Pha A (-)", "#16A34A"),
        ("B+", "Dây ĐỎ (Red)", "Cuộn Pha B (+)", "#DC2626"),
        ("B-", "Dây XANH DƯƠNG (Blue)", "Cuộn Pha B (-)", "#2563EB"),
    ]
    my_y = 115
    for p_label, w_color, p_role, c_box in motor_wires:
        d.rounded_rectangle([25, my_y, 65, my_y+24], radius=4, fill=c_box)
        d.text((32, my_y+4), p_label, fill="#FFFFFF", font=get_font(10.5, bold=True))
        d.text((75, my_y+2), w_color, fill="#0F172A", font=get_font(10.5, bold=True))
        d.text((75, my_y+18), p_role, fill="#64748B", font=get_font(9.5))
        my_y += 52

    # Right: Compatibility & Usage
    d.rounded_rectangle([535, 75, 745, 360], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((545, 85), "KHẢ NĂNG TƯƠNG THÍCH", fill="#1E3A8A", font=get_font(11, bold=True))
    
    notes = [
        "🟢 DRIVER TƯƠNG THÍCH:",
        "  • Leadshine DM542E",
        "  • TB6600, DM860",
        "  • TMC2209, A4988",
        "",
        "🔴 CẢNH BÁO:",
        "  • Động cơ này KHÔNG CÓ",
        "    Encoder -> CẤM dùng với",
        "    Best BH57 (sẽ báo lỗi FLT)",
        "  • Tuyệt đối KHÔNG rút dây",
        "    khi driver đang có điện!"
    ]
    ny = 115
    for n in notes:
        is_green = "🟢" in n
        is_red = "🔴" in n
        col = "#16A34A" if is_green else ("#DC2626" if is_red else "#334155")
        d.text((545, ny), n, fill=col, font=get_font(10, bold=(is_green or is_red)))
        ny += 20

    im.save(os.path.join(img_dir, "motor_gearbox_card.png"))
    print("Generated motor_gearbox_card.png")

# 5. TM1638 Display & Key Module Card
def make_tm1638_card():
    w, h = 760, 360
    im = Image.new("RGB", (w, h), "#F8FAFC")
    d = ImageDraw.Draw(im)
    draw_header(d, w, "Module Bàn Phím & Màn Hình TM1638", "8 LED 7 đoạn | 8 LED Đỏ | 8 Nút bấm", "INPUT / OUTPUT MODULE")
    d.rectangle([0, 0, w-1, h-1], outline="#CBD5E1", width=1)
    
    # Board Layout
    bx, by, bw, bh = 30, 75, 700, 160
    d.rounded_rectangle([bx, by, bx+bw, by+bh], radius=10, fill="#1E3A8A", outline="#3B82F6", width=2)
    
    # 8 Red 7-segment displays
    for i in range(8):
        dx = bx + 25 + i * 80
        d.rectangle([dx, by+20, dx+65, by+75], fill="#0F172A", outline="#64748B")
        d.text((dx+20, by+30), str(i+1), fill="#EF4444", font=get_font(28, bold=True))
        
    # 8 Indicator LEDs
    for i in range(8):
        lx = bx + 48 + i * 80
        d.ellipse([lx, by+90, lx+18, by+108], fill="#EF4444", outline="#FFFFFF")
        d.text((lx+2, by+112), f"D{i+1}", fill="#93C5FD", font=get_font(9, bold=True))
        
    # 8 Buttons
    for i in range(8):
        sx = bx + 45 + i * 80
        d.rectangle([sx, by+128, sx+24, by+148], fill="#E2E8F0", outline="#0F172A")
        d.text((sx+4, by+132), f"S{i+1}", fill="#0F172A", font=get_font(9, bold=True))
        
    # Bottom Pinout Guide
    d.rounded_rectangle([30, 250, 730, 345], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((45, 260), "SƠ ĐỒ ĐẤU DÂY VÀO ESP32-S3 (HEADER 5 CHÂN J1):", fill="#1E3A8A", font=get_font(11, bold=True))
    
    tm_pins = [
        ("VCC", "Nối 3.3V (hoặc 5V) ESP32", "#DC2626"),
        ("GND", "Nối GND ESP32", "#0F172A"),
        ("STB", "Nối GPIO 10 ESP32", "#2563EB"),
        ("CLK", "Nối GPIO 11 ESP32", "#0891B2"),
        ("DIO", "Nối GPIO 12 ESP32", "#16A34A"),
    ]
    tx = 45
    for p_lbl, p_exp, p_col in tm_pins:
        d.rounded_rectangle([tx, 290, tx+55, 320], radius=4, fill=p_col)
        d.text((tx+12, 296), p_lbl, fill="#FFFFFF", font=get_font(11, bold=True))
        d.text((tx+62, 296), p_exp, fill="#334155", font=get_font(9.5))
        tx += 138

    im.save(os.path.join(img_dir, "tm1638_card.png"))
    print("Generated tm1638_card.png")

# 6. TMC2209 Module Card
def make_tmc2209_card():
    w, h = 760, 360
    im = Image.new("RGB", (w, h), "#F8FAFC")
    d = ImageDraw.Draw(im)
    draw_header(d, w, "Trinamic TMC2209 v1.3 SilentStepStick", "Logic: 3.3V/5V | Nguồn Động lực VMOT: 4.75-28V", "PCB MODULE DRIVER")
    d.rectangle([0, 0, w-1, h-1], outline="#CBD5E1", width=1)
    
    # Module PCB (Center)
    px, py, pw, ph = 260, 75, 240, 260
    d.rounded_rectangle([px, py, px+pw, py+ph], radius=8, fill="#0284C7", outline="#0369A1", width=2)
    # IC Heatsink
    d.rounded_rectangle([px+50, py+50, px+pw-50, py+ph-50], radius=4, fill="#1E293B", outline="#475569")
    d.text((px+70, py+85), "TRINAMIC", fill="#38BDF8", font=get_font(14, bold=True))
    d.text((px+65, py+115), "TMC2209-LA", fill="#FFFFFF", font=get_font(16, bold=True))
    d.text((px+60, py+145), "StealthChop2 & UART", fill="#94A3B8", font=get_font(10))
    
    # Left: Pinout
    d.rounded_rectangle([15, 75, 235, 345], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((25, 85), "SƠ ĐỒ CHÂN LOGIC & NGUỒN", fill="#1E3A8A", font=get_font(11, bold=True))
    
    tmc_pins = [
        ("VIO", "3.3V (Cấp từ ESP32)", "#2563EB"),
        ("GND", "Mass đất hệ thống", "#0F172A"),
        ("VMOT", "24V Động lực (+ Tụ 100uF)", "#DC2626"),
        ("STEP", "Xung bước (GPIO 4)", "#16A34A"),
        ("DIR", "Chiều quay (GPIO 5)", "#D97706"),
        ("EN", "Bật/Tắt (Mức LOW = BẬT)", "#9333EA"),
    ]
    ty = 110
    for p_name, p_desc, p_col in tmc_pins:
        d.rounded_rectangle([25, ty, 75, ty+24], radius=4, fill=p_col)
        d.text((32, ty+4), p_name, fill="#FFFFFF", font=get_font(10, bold=True))
        d.text((82, ty+4), p_desc, fill="#334155", font=get_font(9.5))
        ty += 36

    # Right: Warnings
    d.rounded_rectangle([525, 75, 745, 345], radius=8, fill="#FEF2F2", outline="#FCA5A5")
    d.text((535, 85), "⚠️ CẢNH BÁO CHỐNG NỔ", fill="#DC2626", font=get_font(11, bold=True))
    
    tmc_warns = [
        "1. BẮT BUỘC TỤ HÓA:",
        "   Hàn tụ >= 100uF (35V)",
        "   sát chân VMOT & GND.",
        "   (Thiếu tụ sẽ nổ chip khi",
        "   cắm nguồn 24V!).",
        "2. VIO nối đúng 3.3V ESP32.",
        "3. Chân DIAG: Sensorless",
        "   Homing (về home không",
        "   cần công tắc hành trình)."
    ]
    tw_y = 110
    for w_line in tmc_warns:
        d.text((535, tw_y), w_line, fill="#7F1D1D" if "1." in w_line or "nổ chip" in w_line else "#991B1B", font=get_font(10, bold=("1." in w_line or "BẮT BUỘC" in w_line)))
        tw_y += 22

    im.save(os.path.join(img_dir, "tmc2209_card.png"))
    print("Generated tmc2209_card.png")

# 7. Closed-Loop Stepper Motor with Encoder Card
def make_closed_loop_motor_card():
    w, h = 760, 380
    im = Image.new("RGB", (w, h), "#F8FAFC")
    d = ImageDraw.Draw(im)
    draw_header(d, w, "Động cơ bước Vòng Kín kèm Encoder (57CME / 42HSE)", "NEMA 23/17 | Optical Encoder 1000PPR (4000CPR)", "CLOSED-LOOP STEPPER MOTOR")
    d.rectangle([0, 0, w-1, h-1], outline="#CBD5E1", width=1)
    
    # Motor illustration (Center)
    mx, my, mw, mh = 250, 75, 260, 280
    # Shaft
    d.rounded_rectangle([mx+mw//2-12, my+10, mx+mw//2+12, my+60], radius=4, fill="#94A3B8", outline="#475569")
    # Stepper Body
    d.rounded_rectangle([mx+25, my+60, mx+mw-25, my+195], radius=8, fill="#1E293B", outline="#0F172A", width=2)
    d.text((mx+65, my+80), "Leadshine", fill="#FFFFFF", font=get_font(13, bold=True))
    d.text((mx+65, my+105), "57CME23", fill="#38BDF8", font=get_font(18, bold=True))
    d.text((mx+65, my+135), "2-Phase Closed Loop", fill="#94A3B8", font=get_font(10))
    d.text((mx+65, my+155), "Torque: 2.3 N.m | 4.0A", fill="#CBD5E1", font=get_font(10))
    
    # Encoder Box (Rear)
    d.rounded_rectangle([mx+35, my+200, mx+mw-35, my+265], radius=6, fill="#047857", outline="#10B981", width=2)
    d.text((mx+50, my+215), "OPTICAL ENCODER", fill="#A7F3D0", font=get_font(11, bold=True))
    d.text((mx+55, my+238), "1000 Line / 4000 CPR", fill="#FFFFFF", font=get_font(10))
    
    # Left: Motor Cable
    d.rounded_rectangle([15, 75, 225, 360], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((25, 85), "CHÙM 1: DÂY ĐỘNG LỰC (4 DÂY TO)", fill="#1E3A8A", font=get_font(10, bold=True))
    
    wires = [
        ("A+", "Dây ĐEN (Black)", "#1E293B"),
        ("A-", "Dây XANH LÁ (Green)", "#16A34A"),
        ("B+", "Dây ĐỎ (Red)", "#DC2626"),
        ("B-", "Dây XANH DƯƠNG (Blue)", "#2563EB"),
    ]
    wy = 115
    for p_n, w_c, c_b in wires:
        d.rounded_rectangle([25, wy, 65, wy+24], radius=4, fill=c_b)
        d.text((32, wy+4), p_n, fill="#FFFFFF", font=get_font(10.5, bold=True))
        d.text((75, wy+4), w_c, fill="#0F172A", font=get_font(10.5, bold=True))
        wy += 42
        
    d.text((25, 290), "🟢 Nối vào cọc A+, A-, B+, B-", fill="#16A34A", font=get_font(9.5, bold=True))
    d.text((25, 310), "của Driver CL57T / Best BH57", fill="#16A34A", font=get_font(9.5))

    # Right: Encoder Cable
    d.rounded_rectangle([535, 75, 745, 360], radius=8, fill="#FFFFFF", outline="#CBD5E1")
    d.text((545, 85), "CHÙM 2: CÁP ENCODER (6 DÂY NHỎ)", fill="#1E3A8A", font=get_font(9.5, bold=True))
    
    enc_w = [
        ("VCC", "ĐỎ (+5V)", "#DC2626"),
        ("EGND", "TRẮNG (0V)", "#0F172A"),
        ("EA+", "ĐEN (Kênh A+)", "#1E293B"),
        ("EA-", "XANH DƯƠNG (A-)", "#2563EB"),
        ("EB+", "VÀNG (Kênh B+)", "#D97706"),
        ("EB-", "XANH LÁ (B-)", "#16A34A"),
    ]
    ew_y = 112
    for en_l, en_c, en_bg in enc_w:
        d.rounded_rectangle([545, ew_y, 595, ew_y+22], radius=4, fill=en_bg)
        d.text((550, ew_y+3), en_l, fill="#FFFFFF", font=get_font(9.5, bold=True))
        d.text((605, ew_y+3), en_c, fill="#0F172A", font=get_font(9.5))
        ew_y += 36

    im.save(os.path.join(img_dir, "closed_loop_motor_card.png"))
    print("Generated closed_loop_motor_card.png")

if __name__ == "__main__":
    make_esp32s3_card()
    make_dm542e_card()
    make_bh57_card()
    make_motor_gearbox_card()
    make_tm1638_card()
    make_tmc2209_card()
    make_closed_loop_motor_card()
    print("All 7 hardware cards created successfully.")

