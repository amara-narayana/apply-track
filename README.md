# APPLY TRACK - Cross-Platform Job Application Tracker

A fast, lightweight, and privacy-first application with **Glassmorphism**, **Light and Dark Mode**, a **native C++ Windows backend**, a **native Android app**, **normal SQL**, and **offline SQLite Storage**.

> **Direct Downloads**:
> - 📱 **Android App**: [**`ApplyTrack.apk`**](https://github.com/amara-narayana/apply-track/releases/download/v1.0.0/ApplyTrack.apk) *(445 KB)* - Installs directly and automatically prompts to add to your Home Screen!
> - 💻 **Windows Desktop**: [**`ApplyTrack.exe`**](https://github.com/amara-narayana/apply-track/releases/download/v1.0.0/ApplyTrack.exe) *(5.25 MB)* - Standalone portable binary, no installer, no DLLs, zero Node.js.

---

## 💎 Features & Capabilities

- **Android Mobile Application (`ApplyTrack.apk`)**:
  - **Auto Home Screen Shortcut**: Built-in Android `ShortcutManager` automatically prompts to pin the official **APPLY TRACK** icon to the user's home screen.
  - **Offline SQLite Storage**: Pure Android internal sandbox SQLite database (`jobtracker.db`).
  - **Native Intent Sharing**: One-tap CSV export with native Android share sheet (Gmail, WhatsApp, Drive, etc.).
  - **Ultra-Compact**: Lightweight signed APK under 460 KB.
- **Direct Windows Executable (`ApplyTrack.exe`)**:
  - 100% standalone portable binary with zero external dependencies.
  - Statically linked C++ runtime and embedded SQLite3 engine.
  - All web assets (HTML5, CSS3, JavaScript, icons, branding banners) are embedded directly inside the `.exe`.
  - **Custom Database Location**: Change and save your SQLite database to any directory, external drive, or sync folder directly from the app.
- **100% Native & Zero Node.js**:
  - No `node_modules`, no npm, no JavaScript runtime overhead.
  - Minimal memory footprint (< 15 MB RAM on Windows, instant start on Android).
- **Glassmorphic Light & Dark Mode**:
  - **Dark Mode**: Deep atmospheric obsidian glass, glowing teal and orange accents, frosted panels.
  - **Light Mode**: Frosted ice glassmorphism with crisp high-contrast text and translucent white panels.
  - **Toggle**: One-click theme toggle in the titlebar with persistent settings.
- **Normal SQL Relational Database**:
  - Real SQLite 3 database with prepared statements and transactions.
  - Normal SQL operations: `CREATE TABLE`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`.
- **Zero Predefined Fake Data**:
  - Starts 100% clean and pristine. Zero mock records.

---

## 🚀 How to Run & Install

### Android Mobile
1. Download [**`ApplyTrack.apk`**](https://github.com/amara-narayana/apply-track/releases/download/v1.0.0/ApplyTrack.apk).
2. Tap the APK to install on your Android device.
3. Open **APPLY TRACK** — the app will automatically prompt you to pin the shortcut directly to your **Home screen**!

### Windows Desktop (Recommended)
1. Download [**`ApplyTrack.exe`**](https://github.com/amara-narayana/apply-track/releases/download/v1.0.0/ApplyTrack.exe).
2. Double-click `ApplyTrack.exe` anywhere on your computer.
3. The app will launch immediately with full glassmorphism UI connected to your internal SQLite database!

### Build Windows from Source
```powershell
python scripts/embed_assets.py
windres -I . -i src/app_icon.rc -O coff -o src/app_icon.res
g++ -O3 -I src -static -static-libgcc -static-libstdc++ src/backend.cpp src/app_icon.res -lsqlite3 -lws2_32 -lshell32 -lole32 -o ApplyTrack.exe
```

### Build Android APK from Source
```powershell
powershell -ExecutionPolicy Bypass -File scripts/build_android_apk.ps1
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
