import sys
import time
import threading
import serial
import serial.tools.list_ports

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
if hasattr(sys.stdin, 'reconfigure'):
    sys.stdin.reconfigure(encoding='utf-8', errors='replace')

def find_port():
    for p in serial.tools.list_ports.comports():
        if '1001' in (p.hwid or '') or p.device == 'COM10':
            return p.device
    return 'COM10'

def main():
    port = find_port()
    print(f"[CONNECTING] Dang ket noi toi ESP32-S3 ({port})...")
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
        ser.dtr = True
        ser.rts = True
    except Exception as e:
        print(f"[ERROR] Khong the mo cong {port}: {e}")
        print(">> Kiem tra xem co chuong trinh nao khac dang mo cong khong.")
        input("\nNhan Enter de thoat...")
        sys.exit(1)

    time.sleep(0.3)
    print(f"[CONNECTED] Da ket noi {port}! Go lenh (vd: 1600, RUN, R, SPEED 500, STOP) roi Enter:")
    print("=" * 65)

    # Gửi lệnh HELP ban đầu
    ser.write(b"HELP\n")

    running = True

    def reader():
        while running:
            try:
                if ser.in_waiting:
                    data = ser.read(ser.in_waiting)
                    if data:
                        print(data.decode('utf-8', errors='ignore'), end='', flush=True)
                else:
                    time.sleep(0.01)
            except Exception:
                break

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    try:
        while True:
            cmd = input()
            if cmd.strip().lower() in ['exit', 'quit']:
                break
            ser.write((cmd.strip() + "\r\n").encode('utf-8'))
            time.sleep(0.05)
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

