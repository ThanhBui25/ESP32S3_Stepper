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

import serial.tools.list_ports

def find_esp32_port():
    """Find the active COM port for ESP32-S3 (VID: 303A, PID: 1001)"""
    for p in serial.tools.list_ports.comports():
        if (p.vid == 0x303A and p.pid == 0x1001) or "ESP32" in (p.description or "").upper() or "USB SERIAL" in (p.description or "").upper():
            return p.device
    for p in serial.tools.list_ports.comports():
        if "COM" in p.device and p.device not in ("COM1", "COM2", "COM3", "COM4"):
            return p.device
    return None

def flash():
    port_to_use = find_esp32_port()
    if not port_to_use:
        print("[ERROR] ESP32-S3 device not found in COM ports.")
        sys.exit(1)
    
    print(f"[INFO] Found ESP32-S3 at port: {port_to_use}")
    print("[INFO] Putting ESP32-S3 into ROM Bootloader / Download Mode...")

    # Open serial and send USB-JTAG-Serial reset sequence into download mode
    s = serial.Serial(port_to_use, 115200, timeout=0.5, write_timeout=None)
    reset_strat = USBJTAGSerialReset(s)
    reset_strat()
    time.sleep(0.3)
    s.close()
    time.sleep(0.3)

    build_dir = os.path.join(os.path.dirname(__file__), '.pio', 'build', 'esp32-s3-devkitc-1')
    bootloader_bin = os.path.join(build_dir, 'bootloader.bin')
    partitions_bin = os.path.join(build_dir, 'partitions.bin')
    firmware_bin = os.path.join(build_dir, 'firmware.bin')
    boot_app0_bin = os.path.expanduser(r'~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin')

    flash_args = [
        '--chip', 'esp32s3',
        '--port', port_to_use,
        '--baud', '921600',
        '--before', 'no_reset',
        '--after', 'hard_reset',
        'write_flash', '-z',
        '--flash_mode', 'dio',
        '--flash_freq', '80m',
        '--flash_size', '8MB',
        '0x0000', bootloader_bin,
        '0x8000', partitions_bin,
        '0xe000', boot_app0_bin,
        '0x10000', firmware_bin
    ]

    print("[INFO] Writing firmware to ESP32-S3 flash...")
    sys.argv = ['esptool.py'] + flash_args
    esptool._main()
    print("\n[SUCCESS] Firmware uploaded and ESP32-S3 restarted successfully!")

if __name__ == '__main__':
    flash()
