@echo off
cls
echo [ NAPSE FORENSIC - NUCLEAR DEPLOYMENT ]
echo.

:: 1. Clean up old Git mess
rmdir /s /q .git
git init
git config user.name "Napse Admin"
git config user.email "forensic@napse.com"

:: 2. Add files
git add .
git commit -m "Napse Workstation v1.2 [Final Stable Build]"

:: 3. Force Push to Main
git remote add origin https://github.com/SLECET7z/napse-forensic-scanner.git
git branch -M main
git push -f origin main

echo.
echo --- SUCCESS! ---
echo Your website is now CLEAN and LIVE.
pause
