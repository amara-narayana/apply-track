# APPLY TRACK - Native Windows Job Tracker

A fast, lightweight, and 100% native Windows desktop application with **Glassmorphism**, **Light and Dark Mode**, a **minimal C++ backend (Winsock2 + SQLite3)**, **normal SQL**, and **Windows Internal Storage (`%APPDATA%`)**.

> **Direct Download**: Download the standalone single executable [**`ApplyTrack.exe`**](https://github.com/amara-narayana/apply-track/releases/download/v1.0.0/ApplyTrack.exe) directly from the GitHub Release! No installer, no zip extraction, no DLLs, and zero Node.js needed.

---

## 💎 Features & Capabilities

- **Direct Single Executable (`ApplyTrack.exe`)**:
  - 100% standalone portable binary with zero external dependencies.
  - Statically linked C++ runtime and embedded SQLite3 engine.
  - All web assets (HTML5, CSS3, JavaScript, icons, branding banners) are embedded directly inside the `.exe`.
- **100% Native & Zero Node.js**:
  - No `node_modules`, no npm, no JavaScript runtime overhead.
  - Minimal memory footprint (< 15 MB RAM).
- **Glassmorphic Light & Dark Mode**:
  - **Dark Mode**: Deep atmospheric obsidian glass, glowing teal and orange accents, frosted panels.
  - **Light Mode**: Frosted ice glassmorphism with crisp high-contrast text and translucent white panels.
  - **Toggle**: One-click theme toggle in the titlebar with persistent settings.
- **Dropdown Controls**:
  - Custom SVG chevron arrows with theme-aware styling.
  - Explicit option backgrounds (`#0f172a` in dark mode, `#ffffff` in light mode) guaranteeing 100% visibility and legibility.
- **Normal SQL Relational Database**:
  - Real SQLite 3 database stored in Windows Internal Storage: `%APPDATA%\JobApplicationTracker\jobtracker.db`.
  - Normal SQL operations: `CREATE TABLE`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`.
- **Zero Predefined Fake Data**:
  - Starts 100% clean and pristine. Zero mock records.
- **Custom Application Icon & Brand**:
  - Custom icon extracted from user logo (clipboard, checklist, and magnifying growth arrow) embedded into the `.exe` binary.

---

## 🚀 How to Run

### Direct Standalone Binary (Recommended)
1. Download [**`ApplyTrack.exe`**](https://github.com/amara-narayana/apply-track/releases/download/v1.0.0/ApplyTrack.exe).
2. Double-click `ApplyTrack.exe` anywhere on your computer.
3. The app will launch immediately with full glassmorphism UI connected to your internal SQLite database!

### Build from Source
```powershell
# 1. Embed renderer assets (optional, header included in repo)
python scripts/embed_assets.py

# 2. Compile Windows resource & executable
windres -I . -i src/app_icon.rc -O coff -o src/app_icon.res
g++ -O3 -I src -static -static-libgcc -static-libstdc++ src/backend.cpp src/app_icon.res -lsqlite3 -lws2_32 -o ApplyTrack.exe
```

---

## 📁 Project Structure

```text
d:\job-tracker\
├── app_icon.ico                 # Multi-resolution Windows icon (256, 128, 64, 48, 32, 16)
├── app_icon.png                 # 512x512 Master high-resolution icon
├── JobTrack.lnk                 # Windows desktop/folder shortcut
├── jobtracker_backend.exe       # Precompiled native C++ SQLite backend
├── Launch-JobTracker.bat        # Double-click launcher
├── libsqlite3-0.dll             # SQLite runtime DLL
├── libstdc++-6.dll              # C++ runtime DLL
├── libgcc_s_seh-1.dll           # GCC runtime DLL
├── libwinpthread-1.dll          # POSIX thread DLL
├── src\
│   ├── backend.cpp              # Minimal C++ HTTP & SQLite backend
│   ├── app_icon.rc              # Windows resource script for executable icon
│   └── renderer\
│       ├── index.html           # Pure HTML5 layout
│       ├── styles.css           # Glassmorphic CSS3 with Dark/Light themes
│       ├── app.js               # Pure Vanilla JavaScript frontend
│       ├── icons.js             # Self-contained embedded SVG icons
│       └── app_icon.png         # UI branding logo
└── test\
    └── run-tests.ps1            # Pure PowerShell automated test suite
```

---

## 🚀 How to Run

### Option 1: Double-Click
Double-click [`JobTrack.lnk`](file:///d:/job-tracker/JobTrack.lnk) or [`Launch-JobTracker.bat`](file:///d:/job-tracker/Launch-JobTracker.bat):
```powershell
.\Launch-JobTracker.bat
```

### Option 2: Run Native Integration Tests
To run the automated test suite (verifying HTML, CSS glassmorphism, normal SQL CRUD, and 0 fake records):
```powershell
powershell -ExecutionPolicy Bypass -File test/run-tests.ps1
```
