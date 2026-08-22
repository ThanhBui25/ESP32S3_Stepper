import os
import sys
import time
import winreg
import serial

# Add esptool to path from platformio packages
pio_tool_path = os.path.expanduser(r'~/.platformio/packages/tool-esptoolpy')
if os.path.exists(pio_tool_path):
    sys.path.insert(0, pio_tool_path)

import esptool
from esptool.reset import USBJTAGSerialReset

def find_esp32_device_path():
    """Find the physical ESP32-S3 USB CDC Device Interface path"""
    try:
        k = winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE,
            r'SYSTEM\CurrentControlSet\Control\DeviceClasses\{86e0d1e0-8089-11d0-9ce4-08003e301f73}'
        )
        count = winreg.QueryInfoKey(k)[0]
        for i in range(count):
            sub = winreg.EnumKey(k, i)
            if 'VID_303A' in sub.upper() and 'PID_1001' in sub.upper():
                # Convert registry entry ##?#... to \\?\...
                path = r'\\?\\' + sub[4:]
                return path
    except Exception as e:
        print(f"[ERROR] Could not query registry for ESP32 path: {e}")
    return None

def flash():
    device_path = find_esp32_device_path()
    if not device_path:
        print("[ERROR] ESP32-S3 device not found in Windows device classes.")
        sys.exit(1)
    
    print(f"[INFO] Found ESP32-S3 hardware at: {device_path}")
    print("[INFO] Putting ESP32-S3 into ROM Bootloader / Download Mode...")

    # Open serial and send USB-JTAG-Serial reset sequence into download mode
    s = serial.Serial(device_path, 115200, timeout=0.5, write_timeout=None)
    reset_strat = USBJTAGSerialReset(s)
    reset_strat()
    time.sleep(0.3)
    s.close()
    time.sleep(0.2)

    build_dir = os.path.join(os.path.dirname(__file__), '.pio', 'build', 'esp32-s3-devkitc-1')
    bootloader_bin = os.path.join(build_dir, 'bootloader.bin')
    partitions_bin = os.path.join(build_dir, 'partitions.bin')
    firmware_bin = os.path.join(build_dir, 'firmware.bin')
    boot_app0_bin = os.path.expanduser(r'~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin')

    flash_args = [
        '--chip', 'esp32s3',
        '--port', device_path,
        '--baud', '921600',
        '--before', 'no_reset',
        '--after', 'hard_reset',
        'write_flash',
        '-z',
        '--flash_mode', 'dio',
        '--flash_freq', '80m',
        '--flash_size', '8MB',
        '0x0', bootloader_bin,
        '0x8000', partitions_bin,
        '0xe000', boot_app0_bin,
        '0x10000', firmware_bin,
    ]

    print("[INFO] Writing firmware to ESP32-S3 flash...")
    sys.argv = ['esptool.py'] + flash_args
    esptool._main()
    print("\n[SUCCESS] Firmware uploaded and ESP32-S3 restarted successfully!")

if __name__ == '__main__':
    flash()
