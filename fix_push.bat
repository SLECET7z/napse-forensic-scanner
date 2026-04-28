@echo off
echo --- SECURE HISTORY CLEANUP (v2) ---
rmdir /s /q .git
git init
git config user.email "forensic@napse.com"
git config user.name "Napse Admin"
git remote add origin https://github.com/SLECET7z/napse-forensic-scanner.git
git add .
git commit -m "Final Secure Release - v1.1"
git push -f origin master
git push -f origin main
echo.
echo --- SUCCESS! ---
echo Your website is now LIVE.
pause
