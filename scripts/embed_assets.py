import os

def embed_files(files, output_header):
    output = []
    output.append("// Auto-generated embedded assets header for ApplyTrack standalone Windows executable\n")
    output.append("#pragma once\n")
    output.append("#include <cstddef>\n#include <string>\n\n")

    for file_path, var_prefix in files.items():
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"Asset not found: {file_path}")
        with open(file_path, "rb") as f:
            data = f.read()
        print(f"Embedding {file_path}: {len(data)} bytes -> {var_prefix}")
        hex_bytes = [f"0x{b:02x}" for b in data]
        lines = []
        for i in range(0, len(hex_bytes), 16):
            lines.append("    " + ", ".join(hex_bytes[i:i+16]))
        array_content = ",\n".join(lines)
        output.append(f"// Embedded asset: {os.path.basename(file_path)} ({len(data)} bytes)\n")
        output.append(f"const unsigned char ASSET_{var_prefix}_DATA[] = {{\n{array_content}\n}};\n")
        output.append(f"const size_t ASSET_{var_prefix}_LEN = {len(data)};\n\n")

    with open(output_header, "w", encoding="utf-8") as f:
        f.writelines(output)
    print(f"Generated {output_header} successfully.")

if __name__ == "__main__":
    assets = {
        "src/renderer/index.html": "INDEX_HTML",
        "src/renderer/styles.css": "STYLES_CSS",
        "src/renderer/app.js": "APP_JS",
        "src/renderer/icons.js": "ICONS_JS",
        "src/renderer/app_icon.png": "APP_ICON_PNG",
        "src/renderer/apply_track_banner.png": "BANNER_PNG"
    }
    embed_files(assets, "src/embedded_assets.h")
