import os
import urllib.request
import docx
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH

# Create images folder
img_dir = r"d:\Luu\images"
os.makedirs(img_dir, exist_ok=True)

# List of hardware images from reliable open CDNs / repositories
image_sources = {
    "esp32s3.jpg": "https://raw.githubusercontent.com/espressif/esp-dev-kits/master/docs/en/esp32s3/esp32-s3-devkitc-1/_static/esp32-s3-devkitc-1-v1.1-isometric.png",
    "dm542e.jpg": "https://m.media-amazon.com/images/I/61N9p7X8S+L._AC_SL1000_.jpg",
    "bh57_closed_loop.jpg": "https://m.media-amazon.com/images/I/61Q6q2oG8YL._AC_SL1000_.jpg",
    "stepper_gearbox.jpg": "https://m.media-amazon.com/images/I/61Z7Z7l0vBL._AC_SL1000_.jpg",
    "closed_loop_motor.jpg": "https://m.media-amazon.com/images/I/61gJ6S0hJGL._AC_SL1000_.jpg",
    "tb6600.jpg": "https://m.media-amazon.com/images/I/61PZ6e1nL-L._AC_SL1000_.jpg",
    "tmc2209.jpg": "https://m.media-amazon.com/images/I/61fI3w4fQeL._AC_SL1000_.jpg",
    "tm1638.jpg": "https://m.media-amazon.com/images/I/61yD4k9R+NL._AC_SL1000_.jpg",
    "e6b2_encoder.jpg": "https://m.media-amazon.com/images/I/51w9h6E7pLL._AC_SL1000_.jpg"
}

headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}

for fname, url in image_sources.items():
    fpath = os.path.join(img_dir, fname)
    if not os.path.exists(fpath) or os.path.getsize(fpath) < 1000:
        try:
            print(f"Downloading {fname}...")
            req = urllib.request.Request(url, headers=headers)
            with urllib.request.urlopen(req, timeout=10) as response, open(fpath, 'wb') as out_file:
                out_file.write(response.read())
            print(f"Saved {fname} ({os.path.getsize(fpath)} bytes)")
        except Exception as e:
            print(f"Error downloading {fname}: {e}")

print("Image download complete.")
