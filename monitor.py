import sys
import time
import threading
import serial
import serial.tools.list_ports

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
if hasattr(sys.stdin, 'reconfigure'):
    sys.stdin.reconfigure(encoding='utf-8', errors='replace')

def get_available_port():
    for p in serial.tools.list_ports.comports():
        if (p.vid == 0x1A86 and p.pid == 0x55D3) or (p.vid == 0x303A and p.pid == 0x1001) or "CH343" in (p.description or "").upper() or "ESP32" in (p.description or "").upper():
            return p.device
    ports = [p.device for p in serial.tools.list_ports.comports()]
    if 'COM7' in ports:
        return 'COM7'
    if 'COM10' in ports:
        return 'COM10'
    if 'COM5' in ports:
        return 'COM5'
    return ports[0] if ports else 'COM7'

def main():
    port = get_available_port()
    print(f"\n[CONNECTING] Dang mo ket noi Serial toi {port}...")
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.2)
        ser.dtr = True
        ser.rts = True
    except Exception as e:
        print(f"[ERROR] Khong the mo cong {port}: {e}")
        print(">> Hay dam bao ban da cam cap ESP32 vao may tinh va dong cac tab Serial khac.")
        input("\nNhan Enter de thoat...")
        sys.exit(1)

    time.sleep(0.5)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    # Gui lenh HELP de lay menu
    ser.write(b"HELP\r\n")
    ser.flush()

    running = True

    def serial_reader():
        while running:
            try:
                if ser.in_waiting > 0:
                    raw = ser.read(ser.in_waiting)
                    if raw:
                        text = raw.decode('utf-8', errors='ignore')
                        sys.stdout.write(text)
                        sys.stdout.flush()
                else:
                    time.sleep(0.01)
            except Exception:
                break

    t = threading.Thread(target=serial_reader, daemon=True)
    t.start()

    print(f"\n[CONNECTED] DA KET NOI THANH CONG TOI {port}!")
    print("=" * 65)
    print("Go lenh (vd: 1600, RUN, STOP, F, R, D, SPEED 2000) roi nhan ENTER:")
    print("=" * 65 + "\n")

    try:
        while True:
            cmd = input()
            if not cmd:
                continue
            if cmd.strip().lower() in ['exit', 'quit']:
                break
            try:
                ser.write((cmd.strip() + "\r\n").encode('utf-8'))
                ser.flush()
            except Exception as e:
                print(f"\n[LOI GUI LENH]: {e}")
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
