import time
import subprocess
import serial.tools.list_ports

pio_exe = r"C:\Users\Admin\.platformio\penv\Scripts\platformio.exe"

print("[INFO] Dang lang nghe va tu dong nap ngay khi cam vao cong USB Native...")
print(">> Vui long RUT cap Type-C va CAM SANG CONG TYPE-C THU 2 (cong in chu 'USB')...")

start = time.time()
while time.time() - start < 45:
    ports = [p.device for p in serial.tools.list_ports.comports()]
    if 'COM10' in ports:
        print("\n[PHAT HIEN] Da nhan dien ESP32-S3 o cong COM10! Tien hanh nap ngay...")
        res = subprocess.run([pio_exe, "run", "--target", "upload"])
        if res.returncode == 0:
            print("\n=======================================================")
            print("🎉 >>> NAP CODE THANH CONG 100%! ESP32-S3 DANG CHAY! <<< 🎉")
            print("=======================================================")
            break
    time.sleep(0.5)
