import os
import shutil
import json

def prepare_assets():
    dest_dir = "android/assets/www"
    os.makedirs(dest_dir, exist_ok=True)
    
    src_files = [
        "src/renderer/index.html",
        "src/renderer/styles.css",
        "src/renderer/app.js",
        "src/renderer/icons.js",
        "src/renderer/app_icon.png",
        "src/renderer/apply_track_banner.png"
    ]
    
    for f in src_files:
        shutil.copy2(f, os.path.join(dest_dir, os.path.basename(f)))
        print(f"Copied {f} -> {dest_dir}")
        
    manifest = {
        "name": "APPLY TRACK",
        "short_name": "ApplyTrack",
        "description": "Glassmorphic Job Application Tracker with SQLite storage",
        "start_url": "index.html",
        "display": "standalone",
        "orientation": "portrait",
        "background_color": "#0a0f1d",
        "theme_color": "#0a0f1d",
        "icons": [
            {
                "src": "app_icon.png",
                "sizes": "192x192 512x512",
                "type": "image/png"
            }
        ]
    }
    
    with open(os.path.join(dest_dir, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
    print("Created manifest.json")

    # Also put manifest.json into src/renderer so desktop/PWA also has it!
    with open("src/renderer/manifest.json", "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

if __name__ == "__main__":
    prepare_assets()
