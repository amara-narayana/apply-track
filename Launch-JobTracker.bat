@echo off
title APPLY TRACK - Glassmorphic Windows Job Tracker
cd /d "%~dp0"

echo ==========================================================
echo   Starting APPLY TRACK (Pure C++ & SQLite Desktop App)
echo   Storage: %%APPDATA%%\JobApplicationTracker\jobtracker.db
echo ==========================================================

start "" jobtracker_backend.exe
