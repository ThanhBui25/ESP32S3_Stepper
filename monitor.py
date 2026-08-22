import sys
import time
import winreg
import threading
import serial

# Thiết lập UTF-8 cho console Windows
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
if hasattr(sys.stdin, 'reconfigure'):
    sys.stdin.reconfigure(encoding='utf-8', errors='replace')

def find_esp32_device_path():
    """Tìm đường dẫn phần cứng thiết bị ESP32-S3 USB CDC"""
    try:
        k = winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE,
            r'SYSTEM\CurrentControlSet\Control\DeviceClasses\{86e0d1e0-8089-11d0-9ce4-08003e301f73}'
        )
        count = winreg.QueryInfoKey(k)[0]
        for i in range(count):
            sub = winreg.EnumKey(k, i)
            if 'VID_303A' in sub.upper() and 'PID_1001' in sub.upper():
                return r'\\?\\' + sub[4:]
    except Exception as e:
        print(f"[ERROR] Khong tim thay ESP32-S3: {e}")
    return None

def main():
    device_path = find_esp32_device_path()
    if not device_path:
        print("[ERROR] Khong tim thay kit ESP32-S3 dang cam!")
        sys.exit(1)

    print("[CONNECTING] Dang ket noi toi ESP32-S3...")
    try:
        ser = serial.Serial(device_path, 115200, timeout=0.1)
        ser.dtr = True
        ser.rts = False
    except Exception as e:
        print(f"[ERROR] Khong the mo cong Serial: {e}")
        sys.exit(1)

    time.sleep(0.5)
    print("[CONNECTED] Da ket noi thanh cong! Nhap lenh (F, R, 1, 5, S, +, -, ?) va nhan Enter:")
    print("-" * 60)

    # Gửi lệnh '?' để lấy menu ban đầu
    ser.write(b'?\n')

    running = True

    def reader():
        while running:
            try:
                data = ser.read(ser.in_waiting or 1)
                if data:
                    print(data.decode('utf-8', errors='ignore'), end='', flush=True)
            except Exception:
                break

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    try:
        while True:
            cmd = input()
            if cmd.strip().lower() in ['exit', 'quit']:
                break
            ser.write((cmd.strip() + '\n').encode('utf-8'))
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        running = False
        try:
            ser.close()
        except Exception:
            pass
        print("\n[DISCONNECTED] Da dong ket noi Serial.")

if __name__ == '__main__':
    main()
