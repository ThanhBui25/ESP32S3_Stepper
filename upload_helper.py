import os
import sys
import time
import subprocess

pio_exe = r"C:\Users\Admin\.platformio\penv\Scripts\platformio.exe"
print("[INFO] Dang cho ket noi nap xuong ESP32-S3...")
print("[HUONG DAN] Neu kit chua tu dong ket noi:")
print("  -> Nhan GIU nut BOOT (hoac nut 0)")
print("  -> Bam NHA nut RST (hoac EN) 1 lan")
print("  -> THA nut BOOT ra")
print("--------------------------------------------------")

for attempt in range(1, 6):
    print(f"\n[INFO] Thu nap lan {attempt}/5 vao COM5...")
    res = subprocess.run([pio_exe, "run", "--target", "upload"], capture_output=False)
    if res.returncode == 0:
        print("\n==================================================")
        print(">>> NAP CODE THANH CONG 100%! ESP32-S3 DANG CHAY! <<<")
        print("==================================================")
        sys.exit(0)
    time.sleep(1)

print("\n[ERROR] Chua the nap duoc. Vui long kiem tra nut BOOT/RST hoac doi cong USB.")
sys.exit(1)
