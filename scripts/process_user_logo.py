import os
from PIL import Image, ImageFilter
import numpy as np

def process_logo():
    src_path = r'C:/Users/Dell/.gemini/antigravity/brain/3bbd1f75-7ff9-40d5-96e3-c7fd505105fc/.user_uploaded/media_1793890876844_283cad1e.jpg'
    out_dir = r'd:/job-tracker'
    renderer_dir = r'd:/job-tracker/src/renderer'
    
    img = Image.open(src_path)
    
    # 1. Extract the full "APPLY TRACK" banner logo
    # Box for icon + text
    full_banner = img.crop((280, 70, 744, 520))
    full_banner.save(os.path.join(renderer_dir, 'apply_track_banner.png'))
    print("Saved banner logo.")

    # 2. Extract the squircle icon
    # The squircle is centered at X=514, Y=272, size around 410x410
    cx, cy = 514, 272
    half = 205
    icon_crop = img.crop((cx - half, cy - half, cx + half, cy + half))
    
    # Let's convert icon to RGBA and make the surrounding flat dark background transparent
    arr = np.array(icon_crop.convert('RGBA'))
    
    # Corner color is the background color
    corners = np.array([arr[0, 0, :3], arr[0, -1, :3], arr[-1, 0, :3], arr[-1, -1, :3]])
    bg = np.mean(corners, axis=0)
    print("Detected corner background:", bg)

    # Color distance from corner background
    diff = np.sqrt(np.sum((arr[:, :, :3].astype(float) - bg)**2, axis=2))
    
    # Smooth thresholding for anti-aliasing the outer edge
    # The squircle has a drop shadow / glow. We want to preserve the squircle and glow,
    # and smoothly feather out into transparency at the outer boundary.
    # Radial mask from center to keep everything inside radius ~180
    H, W = arr.shape[:2]
    Y, X = np.ogrid[:H, :W]
    dist_from_center = np.sqrt((X - W/2)**2 + (Y - H/2)**2)
    
    # Feather alpha based on diff and distance from center
    # Pixels close to background color at the edges become transparent
    alpha = np.ones((H, W), dtype=float) * 255.0
    
    # Outer edge feathering
    outer_mask = dist_from_center > 175
    edge_diff = diff[outer_mask]
    # Where difference is small at outer edge, make transparent
    outer_alpha = np.clip((diff - 6.0) / 16.0 * 255.0, 0, 255.0)
    alpha[outer_mask] = np.minimum(alpha[outer_mask], outer_alpha[outer_mask])
    
    # Very outer corners (> 195)
    corner_mask = dist_from_center > 195
    corner_alpha = np.clip((205 - dist_from_center) / 10.0 * 255.0, 0, 255.0)
    alpha[corner_mask] = np.minimum(alpha[corner_mask], corner_alpha[corner_mask])

    arr[:, :, 3] = alpha.astype(np.uint8)
    
    icon_rgba = Image.fromarray(arr)
    
    # Create master 512x512 icon
    master_512 = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
    # Resize icon to fit nicely with padding (e.g. 460x460 centered)
    icon_resized = icon_rgba.resize((460, 460), Image.Resampling.LANCZOS)
    offset = ((512 - 460) // 2, (512 - 460) // 2)
    master_512.paste(icon_resized, offset, icon_resized)

    # Save 512x512 PNGs
    master_512.save(os.path.join(out_dir, 'app_icon.png'), 'PNG')
    master_512.save(os.path.join(renderer_dir, 'app_icon.png'), 'PNG')
    print("Saved 512x512 app_icon.png")

    # Save Windows multi-resolution .ico
    ico_path = os.path.join(out_dir, 'app_icon.ico')
    master_512.save(
        ico_path,
        format='ICO',
        sizes=[(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)]
    )
    print("Saved Windows multi-resolution app_icon.ico")

if __name__ == '__main__':
    process_logo()
