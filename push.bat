@echo off
echo --- NAPSE SYNC SYSTEM ---
cp NapseDashboard/* docs/
git add .
git commit -m "Finalize v1.1 - Cloud View, Self-Delete, and Integrated UI"
git push origin main
echo.
echo --- SYNC COMPLETE ---
echo GitHub is now building your v1.1 release...
pause
