# .NAPSE Deployment Guide

Follow these steps to host your dashboard on GitHub and enable automated builds for your forensic scanner.

## 1. Create a GitHub Repository
1. Go to [github.com/new](https://github.com/new).
2. Name your repository (e.g., `napse-forensic`).
3. Set it to **Public**.
4. Do NOT initialize with a README (you already have one).
5. Click **Create Repository**.

## 2. Push your Code
Run these commands in your project folder (`Esp Line free fire`):
```bash
git init
git add .
git commit -m "Initialize Napse Forensic Dashboard"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/napse-forensic.git
git push -u origin main
```
*(Replace YOUR_USERNAME with your real GitHub username)*

## 3. Enable GitHub Pages
1. In your GitHub repository, go to **Settings** > **Pages**.
2. Under **Build and deployment** > **Source**, select **Deploy from a branch**.
3. Under **Branch**, select `main` and the `/docs` folder.
4. Click **Save**.
5. Your site will be live at `https://YOUR_USERNAME.github.io/napse-forensic/`.

## 4. Automated EXE Builds
Every time you push code to GitHub, the **GitHub Actions** will automatically:
1. Build your C++ code.
2. Generate the "real works" `Scanner.exe`.
3. You can find the built EXE in the **Actions** tab of your repository under the latest "Build Scanner EXE" run.

## 5. Connecting the Download Link
Once your site is live, you can update the `handleDownload` function in `docs/app.js` to point to your hosted `.exe` or a release link.

---
**Administrator Credentials:**
- **User:** `admin`
- **Pass:** `123`
*(You can change these in `docs/app.js`)*
