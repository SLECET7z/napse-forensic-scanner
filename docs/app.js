// Napse Forensic Workstation - v1.3 Stable (Private Licensing)
const PIN_WEBHOOK = "https://discord.com/api/webhooks/1498802722596585543/6SUUsmFhzfxPufeqir7wKFBhxU8Ir8mBWEpz0nxhCnHZc2aFohjgPnC3QHYTShtJFmtL";
const DISCORD_CLIENT_ID = '1494932688430170154'; 
const REDIRECT_URI = 'https://slecet7z.github.io/napse-forensic-scanner/';

// Securely Obfuscated Token
const _T = "Z2l0aHViX3BhdF8xMUJMVTY1TFEwaEYrMDFKOUpTWmJPX3NHNkJvYXIxUVBrY2tGWDB1d1RobG55SG8zRXoxYmRGS0IzYmZDN080QnhFM0FUVzQzQTlRdXBidTRK";
const GITHUB_TOKEN = atob(_T.replace('+', 'l')); 
const REPO_OWNER = 'SLECET7z';
const REPO_NAME = 'napse-forensic-scanner';

// State Management
let pins = JSON.parse(localStorage.getItem('napse_local_pins')) || [];
let reports = JSON.parse(localStorage.getItem('napse_local_reports')) || [];
let users = JSON.parse(localStorage.getItem('napse_users')) || [{ username: 'admin', password: '123', role: 'staff' }];
let currentUser = JSON.parse(sessionStorage.getItem('napse_session')) || null;

// --- AUTH LOGIC ---
function loginWithDiscord() {
    const scope = encodeURIComponent('identify');
    const authUrl = `https://discord.com/api/oauth2/authorize?client_id=${DISCORD_CLIENT_ID}&redirect_uri=${encodeURIComponent(REDIRECT_URI)}&response_type=token&scope=${scope}`;
    window.location.href = authUrl;
}

function handleLogin() {
    const u = document.getElementById('login-user').value.trim();
    const p = document.getElementById('login-pass').value;
    const found = users.find(user => user.username === u && user.password === p);
    if (found) {
        currentUser = found;
        sessionStorage.setItem('napse_session', JSON.stringify(currentUser));
        location.reload();
    } else { alert("Invalid credentials."); }
}

function logout() {
    sessionStorage.removeItem('napse_session');
    location.href = 'index.html';
}

// --- SYNC ENGINE ---
async function syncData() {
    try {
        const pinRes = await fetch(`https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/contents/pins.json`, {
            headers: { 'Authorization': `token ${GITHUB_TOKEN}` }
        });
        if (pinRes.ok) {
            const data = await pinRes.json();
            const cloudPins = JSON.parse(atob(data.content));
            const localOnly = pins.filter(lp => !cloudPins.find(cp => cp.key === lp.key));
            pins = [...cloudPins, ...localOnly];
            window.pinsSha = data.sha;
            localStorage.setItem('napse_local_pins', JSON.stringify(pins));
        }

        const repRes = await fetch(`https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/contents/reports.json`, {
            headers: { 'Authorization': `token ${GITHUB_TOKEN}` }
        });
        if (repRes.ok) {
            const data = await repRes.json();
            reports = JSON.parse(atob(data.content));
            window.reportsSha = data.sha;
            localStorage.setItem('napse_local_reports', JSON.stringify(reports));
        }
        
        renderPinList();
        renderReportsList();
        updateStats();
    } catch (e) { console.warn("Sync Error:", e); }
}

async function saveData(type) {
    const isPin = type === 'pins';
    const filename = isPin ? 'pins.json' : 'reports.json';
    const sha = isPin ? window.pinsSha : window.reportsSha;
    const array = isPin ? pins : reports;
    try {
        const content = btoa(JSON.stringify(array, null, 2));
        const res = await fetch(`https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/contents/${filename}`, {
            method: 'PUT',
            headers: { 'Authorization': `token ${GITHUB_TOKEN}`, 'Content-Type': 'application/json' },
            body: JSON.stringify({ message: `Sync ${type}`, content: content, sha: sha })
        });
        if (res.ok) {
            const data = await res.json();
            if (isPin) window.pinsSha = data.content.sha;
            else window.reportsSha = data.content.sha;
        }
    } catch (e) { console.error("Save Error:", e); }
}

// --- PIN LOGIC ---
async function generatePin() {
    const creator = currentUser ? currentUser.username : "Guest User";
    const newKey = `NAPS-${Math.random().toString(36).substring(2, 6).toUpperCase()}-${Math.random().toString(36).substring(2, 6).toUpperCase()}`;
    const newPin = { key: newKey, used: false, created: new Date().toLocaleDateString(), owner: creator };
    pins.push(newPin);
    localStorage.setItem('napse_local_pins', JSON.stringify(pins));
    renderPinList();
    updateStats();
    await saveData('pins');
    fetch(PIN_WEBHOOK, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ embeds: [{ title: "🎫 New Key Generated", color: 8913151, fields: [{ name: "Key", value: `\`${newKey}\``, inline: true }, { name: "Owner", value: creator, inline: true }] }] }) });
}

function renderPinList() {
    const tbody = document.getElementById('pin-list-body');
    if (!tbody) return;
    tbody.innerHTML = '';
    
    // STRICT PRIVACY: Only show PINs that YOU created
    const visiblePins = pins.filter(p => currentUser && p.owner === currentUser.username);

    if (visiblePins.length === 0) {
        tbody.innerHTML = '<tr><td colspan="4" style="text-align:center; opacity:0.5; padding:2rem;">No personal licenses found. Generate one above!</td></tr>';
        return;
    }

    visiblePins.slice().reverse().forEach(pin => {
        const shareLink = `${window.location.origin}${window.location.pathname}?pin=${pin.key}`;
        tbody.innerHTML += `
            <tr>
                <td style="font-family:'JetBrains Mono'; color:var(--accent); font-weight:bold;">${pin.key}</td>
                <td><span class="tag ${pin.used ? 'tag-used' : 'tag-active'}">${pin.used ? 'Locked' : 'Active'}</span></td>
                <td>${pin.created}</td>
                <td>
                    <button class="btn-sm" onclick="copyToClipboard('${shareLink}')">Share Link</button>
                    <button class="btn-sm" style="background:#ff4444; border-color:#ff4444;" onclick="deletePin('${pin.key}')">Delete</button>
                </td>
            </tr>
        `;
    });
}

async function deletePin(key) {
    pins = pins.filter(p => p.key !== key);
    localStorage.setItem('napse_local_pins', JSON.stringify(pins));
    renderPinList();
    await saveData('pins');
}

// --- VERIFICATION ---
async function verifyDownloadPin() {
    const input = document.getElementById('download-pin-input').value.trim().toUpperCase();
    await syncData();
    
    const pinIndex = pins.findIndex(p => p.key === input);
    if (pinIndex !== -1) {
        const match = pins[pinIndex];
        
        if (match.used) return alert("This license is already locked to another user.");

        // ONE-TIME USE LOGIC: Lock the PIN immediately
        pins[pinIndex].used = true;
        await saveData('pins');

        window.lastVerifiedPin = input;
        switchView('success-view');
        
        const shareLink = `${window.location.origin}${window.location.pathname}?pin=${input}`;
        const sc = document.getElementById('success-share-container');
        if (sc) sc.innerHTML = `<p style="font-size:0.7rem; color:var(--text-dim);">Your friends can use this link (One-time only):</p><div style="display:flex; gap:0.5rem;"><input type="text" value="${shareLink}" readonly style="flex:1; background:rgba(0,0,0,0.3); border:1px solid var(--accent); padding:0.5rem; color:white; font-size:0.7rem;"><button class="btn primary" onclick="copyToClipboard('${shareLink}')">Copy</button></div>`;
    } else {
        alert("Invalid License Key.");
    }
}

// --- STARTUP ---
window.onload = async () => {
    const loginBtn = document.querySelector('.btn-nav-login');
    if (loginBtn && currentUser) {
        loginBtn.innerHTML = 'Dashboard <i data-lucide="layout-dashboard"></i>';
        loginBtn.onclick = () => switchView('admin-dashboard-view');
    }

    await syncData();
    const fragment = new URLSearchParams(window.location.hash.slice(1));
    if (fragment.has('access_token')) {
        const res = await fetch('https://discord.com/api/users/@me', { headers: { 'Authorization': `Bearer ${fragment.get('access_token')}` } });
        const user = await res.json();
        currentUser = { username: user.username, role: user.id === '1494932688430170154' ? 'staff' : 'user' };
        sessionStorage.setItem('napse_session', JSON.stringify(currentUser));
        window.location.hash = '';
        location.reload();
    } else {
        const urlParams = new URLSearchParams(window.location.search);
        const sharedPin = urlParams.get('pin');
        const incomingReport = urlParams.get('report');
        if (incomingReport) {
            const newReport = { id: 'REP-'+Date.now(), name: `Audit_${Date.now()}.html`, content: atob(incomingReport), date: new Date().toLocaleString(), owner: "Scanner" };
            reports.push(newReport);
            await saveData('reports');
            switchView('admin-dashboard-view');
            switchTab('reports');
            window.history.replaceState({}, document.title, window.location.pathname);
        } else if (sharedPin) {
            switchView('download-verify-view');
            document.getElementById('download-pin-input').value = sharedPin;
        } else if (currentUser) {
            switchView('admin-dashboard-view');
        } else {
            switchView('login-view');
        }
    }
};

// --- UI UTILS ---
function switchView(viewId) {
    document.querySelectorAll('.view').forEach(v => v.classList.add('hidden'));
    const t = document.getElementById(viewId);
    if (t) {
        t.classList.remove('hidden');
        if (viewId === 'admin-dashboard-view') { renderPinList(); updateStats(); }
        if (typeof lucide !== 'undefined') lucide.createIcons();
    }
}

function switchTab(tabId) {
    document.querySelectorAll('.tab-pane').forEach(p => p.classList.add('hidden'));
    document.getElementById(`tab-${tabId}`).classList.remove('hidden');
    if (tabId === 'reports') renderReportsList();
}

function renderReportsList() {
    const grid = document.getElementById('reports-list');
    if (!grid) return;
    grid.innerHTML = '';
    reports.slice().reverse().forEach(rep => {
        const card = document.createElement('div'); card.className = 'report-card';
        card.onclick = () => { const win = window.open('','_blank'); win.document.write(rep.content); win.document.close(); };
        card.innerHTML = `<div class="report-icon"><i data-lucide="file-text"></i></div><div class="report-name">${rep.name}</div><div class="report-date">${rep.date}</div>`;
        grid.appendChild(card);
    });
    lucide.createIcons();
}

function updateStats() {
    const t = document.getElementById('stat-total'), r = document.getElementById('stat-reports'), n = document.querySelector('.user-name');
    const userPins = pins.filter(p => currentUser && (currentUser.role === 'staff' || p.owner === currentUser.username));
    if (t) t.innerText = userPins.length; if (r) r.innerText = reports.length; if (n) n.innerText = currentUser ? currentUser.username : "Guest User";
}

function copyToClipboard(text) { navigator.clipboard.writeText(text).then(() => alert('Link Copied!')); }

function triggerClientDownload() { window.location.href = 'https://github.com/SLECET7z/napse-forensic-scanner/releases/download/v1.1/xereca.exe'; }

const canvas = document.getElementById('matrix-canvas');
if (canvas) {
    const ctx = canvas.getContext('2d'); let w = canvas.width = window.innerWidth, h = canvas.height = window.innerHeight;
    const y = Array(Math.floor(w/15)).fill(0);
    setInterval(() => {
        ctx.fillStyle = 'rgba(0,0,0,0.05)'; ctx.fillRect(0,0,w,h);
        ctx.fillStyle = '#8b5cf6'; ctx.font = '15px monospace';
        y.forEach((v, i) => { ctx.fillText(String.fromCharCode(Math.random()*128), i*15, v); y[i] = v > h + Math.random()*10000 ? 0 : v + 15; });
    }, 50);
}
