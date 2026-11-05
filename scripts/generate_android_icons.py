import os
from PIL import Image

def generate_android_icons(src_icon, res_dir):
    sizes = {
        "mipmap-mdpi": (48, 48),
        "mipmap-hdpi": (72, 72),
        "mipmap-xhdpi": (96, 96),
        "mipmap-xxhdpi": (144, 144),
        "mipmap-xxxhdpi": (192, 192),
    }

    img = Image.open(src_icon).convert("RGBA")

    for folder, size in sizes.items():
        out_folder = os.path.join(res_dir, folder)
        os.makedirs(out_folder, exist_ok=True)
        out_path = os.path.join(out_folder, "ic_launcher.png")
        resized = img.resize(size, Image.Resampling.LANCZOS)
        resized.save(out_path, format="PNG")
        print(f"Generated {out_path} ({size[0]}x{size[1]})")

if __name__ == "__main__":
    generate_android_icons("app_icon.png", "android/res")
