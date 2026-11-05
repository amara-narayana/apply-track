import os
import math
from PIL import Image, ImageDraw, ImageFilter

def create_glassmorphic_job_icon():
    # Render at 1024x1024 for supersampling (crisp anti-aliasing)
    SIZE = 1024
    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. Background Ambient Glow (Iridescent Aura)
    glow = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow)
    
    # Cyan and Magenta/Purple ambient glows
    glow_draw.ellipse([200, 150, 824, 774], fill=(99, 102, 241, 140)) # Indigo glow
    glow_draw.ellipse([450, 300, 950, 850], fill=(0, 242, 254, 110))  # Cyan glow
    glow_draw.ellipse([100, 350, 600, 850], fill=(236, 72, 153, 100)) # Magenta glow
    glow = glow.filter(ImageFilter.GaussianBlur(80))
    img.alpha_composite(glow)

    # 2. Outer Glassmorphic Rounded Base Squircle (Windows 11 Tile Style)
    squircle_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    sq_draw = ImageDraw.Draw(squircle_layer)

    x0, y0, x1, y1 = 112, 112, 912, 912
    radius = 190

    # Drop shadow for the tile
    shadow_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    sh_draw = ImageDraw.Draw(shadow_layer)
    sh_draw.rounded_rectangle([x0 + 10, y0 + 28, x1 + 10, y1 + 38], radius=radius, fill=(0, 0, 0, 180))
    shadow_layer = shadow_layer.filter(ImageFilter.GaussianBlur(32))
    img.alpha_composite(shadow_layer)

    # Base gradient fill (deep cosmic obsidian/navy glass)
    # We can create a vertical gradient
    tile_mask = Image.new('L', (SIZE, SIZE), 0)
    ImageDraw.Draw(tile_mask).rounded_rectangle([x0, y0, x1, y1], radius=radius, fill=255)

    base_gradient = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    for y in range(y0, y1):
        ratio = (y - y0) / (y1 - y0)
        # Gradient from rich dark slate-violet to deep obsidian
        r = int(18 * (1 - ratio) + 8 * ratio)
        g = int(24 * (1 - ratio) + 11 * ratio)
        b = int(48 * (1 - ratio) + 20 * ratio)
        a = 238
        ImageDraw.Draw(base_gradient).line([(x0, y), (x1, y)], fill=(r, g, b, a))

    base_gradient.putalpha(tile_mask)
    img.alpha_composite(base_gradient)

    # Subtle inner border reflection (glass rim)
    rim_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    rim_draw = ImageDraw.Draw(rim_layer)
    rim_draw.rounded_rectangle([x0, y0, x1, y1], radius=radius, outline=(255, 255, 255, 60), width=4)
    # Top highlight on rim
    rim_draw.arc([x0, y0, x0 + radius * 2, y0 + radius * 2], 180, 270, fill=(255, 255, 255, 140), width=6)
    rim_draw.line([(x0 + radius, y0), (x1 - radius, y0)], fill=(255, 255, 255, 160), width=6)
    img.alpha_composite(rim_layer)

    # 3. Handle of the Briefcase (Behind the Document)
    handle_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    h_draw = ImageDraw.Draw(handle_layer)
    
    hx0, hy0, hx1, hy1 = 380, 220, 644, 390
    h_rad = 54
    # Outer arch
    h_draw.rounded_rectangle([hx0, hy0, hx1, hy1], radius=h_rad, outline=(0, 242, 254, 230), width=28)
    # Inner glow on handle
    h_draw.rounded_rectangle([hx0 + 4, hy0 + 4, hx1 - 4, hy1 - 4], radius=h_rad, outline=(255, 255, 255, 200), width=8)
    img.alpha_composite(handle_layer)

    # 4. Job Application / Resume Document (Emerging from Briefcase)
    doc_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    d_draw = ImageDraw.Draw(doc_layer)

    dx0, dy0, dx1, dy1 = 330, 290, 694, 690
    d_rad = 32

    # Doc shadow
    doc_shadow = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    ImageDraw.Draw(doc_shadow).rounded_rectangle([dx0, dy0 + 12, dx1, dy1 + 12], radius=d_rad, fill=(0, 0, 0, 140))
    doc_shadow = doc_shadow.filter(ImageFilter.GaussianBlur(16))
    img.alpha_composite(doc_shadow)

    # Document Glass Body (Frosted white/cyan glass)
    d_draw.rounded_rectangle([dx0, dy0, dx1, dy1], radius=d_rad, fill=(245, 250, 255, 220), outline=(255, 255, 255, 240), width=4)

    # Document content lines (representing application details)
    # Header line (cyan accent)
    d_draw.rounded_rectangle([380, 340, 520, 362], radius=8, fill=(99, 102, 241, 230))
    # Body lines
    d_draw.rounded_rectangle([380, 385, 644, 403], radius=6, fill=(148, 163, 184, 180))
    d_draw.rounded_rectangle([380, 425, 610, 443], radius=6, fill=(148, 163, 184, 180))
    d_draw.rounded_rectangle([380, 465, 560, 483], radius=6, fill=(148, 163, 184, 180))
    d_draw.rounded_rectangle([380, 505, 620, 523], radius=6, fill=(148, 163, 184, 180))

    img.alpha_composite(doc_layer)

    # 5. Front Briefcase Body (Frosted Glass with Gradient)
    case_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    c_draw = ImageDraw.Draw(case_layer)

    bx0, by0, bx1, by1 = 220, 490, 804, 820
    b_rad = 54

    # Briefcase drop shadow
    case_sh = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    ImageDraw.Draw(case_sh).rounded_rectangle([bx0, by0 + 16, bx1, by1 + 24], radius=b_rad, fill=(0, 0, 0, 160))
    case_sh = case_sh.filter(ImageFilter.GaussianBlur(24))
    img.alpha_composite(case_sh)

    # Briefcase gradient (vibrant frosted glass indigo to purple/cyan)
    case_mask = Image.new('L', (SIZE, SIZE), 0)
    ImageDraw.Draw(case_mask).rounded_rectangle([bx0, by0, bx1, by1], radius=b_rad, fill=255)

    case_grad = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    for y in range(by0, by1):
        ratio = (y - by0) / (by1 - by0)
        # Interpolate from deep indigo to electric cyan/purple
        r = int(99 * (1 - ratio) + 20 * ratio)
        g = int(102 * (1 - ratio) + 140 * ratio)
        b = int(241 * (1 - ratio) + 254 * ratio)
        a = 210  # Translucent glass!
        ImageDraw.Draw(case_grad).line([(bx0, y), (bx1, y)], fill=(r, g, b, a))
    
    case_grad.putalpha(case_mask)
    img.alpha_composite(case_grad)

    # Briefcase Glass Border & Highlight
    case_rim = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    cr_draw = ImageDraw.Draw(case_rim)
    cr_draw.rounded_rectangle([bx0, by0, bx1, by1], radius=b_rad, outline=(255, 255, 255, 120), width=5)
    # Specular top edge
    cr_draw.line([(bx0 + b_rad, by0), (bx1 - b_rad, by0)], fill=(255, 255, 255, 210), width=6)
    img.alpha_composite(case_rim)

    # 6. Center Clasp / Badge (Windows Glass Lock Button)
    clasp_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    cl_draw = ImageDraw.Draw(clasp_layer)
    
    cx, cy = 512, 535
    cw, ch = 84, 64
    cl_draw.rounded_rectangle([cx - cw//2, cy - ch//2, cx + cw//2, cy + ch//2], radius=16, fill=(15, 23, 42, 230), outline=(0, 242, 254, 255), width=5)
    # Inner latch in glowing cyan
    cl_draw.rounded_rectangle([cx - 18, cy - 8, cx + 18, cy + 12], radius=6, fill=(0, 242, 254, 240))
    img.alpha_composite(clasp_layer)

    # 7. Success Verified Badge (Bottom Right Glowing Shield/Checkmark)
    badge_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    b_draw = ImageDraw.Draw(badge_layer)

    sx, sy = 710, 710
    s_radius = 82

    # Badge glow
    badge_glow = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    ImageDraw.Draw(badge_glow).ellipse([sx - s_radius - 16, sy - s_radius - 16, sx + s_radius + 16, sy + s_radius + 16], fill=(16, 185, 129, 140))
    badge_glow = badge_glow.filter(ImageFilter.GaussianBlur(24))
    img.alpha_composite(badge_glow)

    # Emerald Badge Circle
    b_draw.ellipse([sx - s_radius, sy - s_radius, sx + s_radius, sy + s_radius], fill=(16, 185, 129, 240), outline=(255, 255, 255, 230), width=6)

    # Crisp Checkmark inside badge
    check_coords = [
        (sx - 36, sy - 2),
        (sx - 10, sy + 26),
        (sx + 36, sy - 28)
    ]
    b_draw.line(check_coords, fill=(255, 255, 255, 255), width=18, joint='round')
    img.alpha_composite(badge_layer)

    # 8. Specular Diagonal Glass Glare Reflection across the entire icon
    glare_layer = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    gl_draw = ImageDraw.Draw(glare_layer)
    
    # Diagonal polygon across top-right
    gl_points = [
        (x0 + 100, y0),
        (x1 - 50, y0),
        (x0, y0 + 400),
        (x0, y0 + 150)
    ]
    gl_draw.polygon(gl_points, fill=(255, 255, 255, 16))
    glare_layer.putalpha(tile_mask)
    img.alpha_composite(glare_layer)

    # 9. Downscale to master 512x512 with high-quality Lanczos resampling
    icon_512 = img.resize((512, 512), Image.Resampling.LANCZOS)
    return icon_512

def save_icons(icon_512, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    
    # Save master PNG
    png_path = os.path.join(out_dir, 'app_icon.png')
    icon_512.save(png_path, 'PNG')
    print(f"Saved master PNG: {png_path} (512x512)")

    # Also save in renderer directory for web/UI use
    renderer_png = os.path.join(out_dir, 'src', 'renderer', 'app_icon.png')
    icon_512.save(renderer_png, 'PNG')
    print(f"Saved renderer icon: {renderer_png}")

    # Generate multi-resolution Windows ICO (256, 128, 64, 48, 32, 16)
    ico_path = os.path.join(out_dir, 'app_icon.ico')
    icon_512.save(
        ico_path,
        format='ICO',
        sizes=[(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)]
    )
    print(f"Saved Windows ICO: {ico_path} (with 6 resolutions)")

if __name__ == '__main__':
    out_directory = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    icon = create_glassmorphic_job_icon()
    save_icons(icon, out_directory)
    print("Job Application Windows Icon design generated successfully!")
