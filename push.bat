@echo off
echo --- FINAL NAPSE LAUNCH SYSTEM ---
echo Updating production files...
git add .
git commit -m "FINAL v1.1 RELEASE - Cloud Integrated Forensic System"
git push origin main
echo.
echo --- SUCCESS! ---
echo 1. Your website is now updated to the new v1.1 logic.
echo 2. GitHub is currently building your new xereca.exe (v1.1).
echo.
echo Wait 2 minutes, then visit your site to test the full flow!
pause
