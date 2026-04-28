// Global Database (Simple PINs)
const AUTHORIZED_PINS = Array.from({ length: 100 }, (_, i) => `NAPSE-${(i + 1).toString().padStart(2, '0')}`);

let pins = [];
let users = JSON.parse(localStorage.getItem('napse_users')) || [
    { username: 'admin', password: '123', role: 'staff' }
];
let currentUser = JSON.parse(sessionStorage.getItem('napse_session')) || null;

// Discord OAuth2 Configuration
const DISCORD_CLIENT_ID = '1494932688430170154'; 
const REDIRECT_URI = window.location.origin + window.location.pathname;

function loginWithDiscord() {
    const scope = encodeURIComponent('identify');
    const authUrl = `https://discord.com/api/oauth2/authorize?client_id=${DISCORD_CLIENT_ID}&redirect_uri=${encodeURIComponent(REDIRECT_URI)}&response_type=token&scope=${scope}`;
    window.location.href = authUrl;
}

// Handle Discord Callback
const fragment = new URLSearchParams(window.location.hash.slice(1));
if (fragment.has('access_token')) {
    const accessToken = fragment.get('access_token');
    fetch('https://discord.com/api/users/@me', {
        headers: { 'Authorization': `Bearer ${accessToken}` }
    })
    .then(res => res.json())
    .then(user => {
        currentUser = { username: user.username, role: 'staff', avatar: `https://cdn.discordapp.com/avatars/${user.id}/${user.avatar}.png` };
        sessionStorage.setItem('napse_session', JSON.stringify(currentUser));
        window.location.hash = '';
        switchView('admin-dashboard-view');
    });
}

// Sync PINs from GitHub
async function syncPinsFromGitHub() {
    try {
        const res = await fetch(`https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/contents/pins.json`, {
            headers: { 'Authorization': `token ${GITHUB_TOKEN}` }
        });
        const data = await res.json();
        pins = JSON.parse(atob(data.content));
        window.pinsSha = data.sha;
        renderPinList();
    } catch (err) {
        console.error('Sync Error:', err);
    }
}

// Save PINs to GitHub
async function savePinsToGitHub() {
    const content = btoa(JSON.stringify(pins, null, 2));
    try {
        await fetch(`https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/contents/pins.json`, {
            method: 'PUT',
            headers: { 
                'Authorization': `token ${GITHUB_TOKEN}`,
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                message: 'Syncing PINs',
                content: content,
                sha: window.pinsSha
            })
        });
        await syncPinsFromGitHub(); // Refresh SHA
    } catch (err) {
        console.error('Save Error:', err);
    }
}

// Load initially
syncPinsFromGitHub();

// Handle Shareable Links
const urlParams = new URLSearchParams(window.location.search);
const sharedPin = urlParams.get('pin');
const reportData = urlParams.get('report');
const transferPort = urlParams.get('transfer');

if (transferPort) {
    window.onload = async () => {
        switchView('report-viewer-view');
        const viewer = document.getElementById('report-viewer-content');
        viewer.innerHTML = '<div class="empty-state">Securely fetching forensic data from local server...</div>';
        
        let attempts = 0;
        let success = false;
        let html = '';

        while (attempts < 5 && !success) {
            try {
                const response = await fetch(`http://localhost:${transferPort}/report`);
                if (response.ok) {
                    html = await response.text();
                    success = true;
                }
            } catch (err) {
                attempts++;
                await new Promise(r => setTimeout(r, 1000)); // Wait 1s
            }
        }

        if (success) {
            viewer.innerHTML = html;
            
            // Auto-save to reports list
            const reportId = 'REP-' + Math.floor(Math.random() * 100000);
            const newReport = {
                id: reportId,
                name: 'Cloud_Report_' + new Date().getTime() + '.html',
                content: html,
                date: new Date().toLocaleString(),
                owner: currentUser ? currentUser.username : 'Anonymous'
            };
            
            let reports = JSON.parse(localStorage.getItem('napse_reports')) || [];
            reports.push(newReport);
            localStorage.setItem('napse_reports', JSON.stringify(reports));
        } else {
            viewer.innerHTML = '<div class="empty-state">Data transfer timed out. Please run the scanner again.</div>';
        }
    };
} else if (reportData) {
    window.onload = () => {
        const decoded = atob(reportData);
        
        // Auto-save to reports list if not already there
        const reportId = 'REP-' + Math.floor(Math.random() * 100000);
        const newReport = {
            id: reportId,
            name: 'Cloud_Report_' + new Date().getTime() + '.html',
            content: decoded,
            date: new Date().toLocaleString(),
            owner: currentUser ? currentUser.username : 'Anonymous'
        };
        
        let reports = JSON.parse(localStorage.getItem('napse_reports')) || [];
        reports.push(newReport);
        localStorage.setItem('napse_reports', JSON.stringify(reports));

        const viewer = document.getElementById('report-viewer-content');
        if (viewer) {
            viewer.innerHTML = decoded;
            switchView('report-viewer-view');
        }
    };
} else if (sharedPin) {
    window.onload = () => {
        const pin = pins.find(p => p.key === sharedPin);
        if (pin && pin.used) {
            alert('This forensic link has expired.');
            switchView('login-view');
        } else {
            switchView('download-verify-view');
            document.getElementById('download-pin-input').value = sharedPin;
        }
    };
}

function saveState() {
    savePinsToGitHub();
    localStorage.setItem('napse_users', JSON.stringify(users));
}

// Matrix Animation Logic
const canvas = document.getElementById('matrix-canvas');
const ctx = canvas.getContext('2d');

let width = canvas.width = window.innerWidth;
let height = canvas.height = window.innerHeight;

const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$#@&%*';
const fontSize = 14;
const columns = width / fontSize;
const drops = [];

for (let i = 0; i < columns; i++) {
    drops[i] = 1;
}

function drawMatrix() {
    ctx.fillStyle = 'rgba(3, 3, 3, 0.05)';
    ctx.fillRect(0, 0, width, height);

    ctx.fillStyle = '#8b5cf6'; // Purple rain
    ctx.font = fontSize + 'px JetBrains Mono';

    for (let i = 0; i < drops.length; i++) {
        const text = characters.charAt(Math.floor(Math.random() * characters.length));
        ctx.fillText(text, i * fontSize, drops[i] * fontSize);

        if (drops[i] * fontSize > height && Math.random() > 0.975) {
            drops[i] = 0;
        }
        drops[i]++;
    }
}

setInterval(drawMatrix, 50);

window.addEventListener('resize', () => {
    width = canvas.width = window.innerWidth;
    height = canvas.height = window.innerHeight;
});

// View Switching Logic
function switchView(viewId) {
    console.log('Switching to view:', viewId);
    
    // Toggle Nav Visibility
    const nav = document.getElementById('main-nav');
    if (nav) {
        nav.style.display = viewId === 'admin-dashboard-view' ? 'none' : 'flex';
    }

    document.querySelectorAll('.view').forEach(v => v.classList.add('hidden'));
    const target = document.getElementById(viewId);
    if (target) {
        target.classList.remove('hidden');
        if (viewId === 'admin-dashboard-view') {
            renderPinList();
            updateStats();
            
            // Only show reports tab to staff
            const reportsTab = document.getElementById('reports-tab-btn');
            if (reportsTab) {
                reportsTab.style.display = currentUser.role === 'staff' ? 'block' : 'none';
            }
        }
        if (typeof lucide !== 'undefined') lucide.createIcons();
    } else {
        console.error('Target view not found:', viewId);
    }
}

// Authentication Logic
function handleRegister() {
    const user = document.getElementById('reg-user').value.trim();
    const pass = document.getElementById('reg-pass').value;
    const confirm = document.getElementById('reg-pass-confirm').value;

    if (!user || !pass) return alert('Please fill in all fields');
    if (pass !== confirm) return alert('Passwords do not match');
    if (users.find(u => u.username === user)) return alert('Username already exists');

    users.push({ 
        username: user, 
        password: pass, 
        role: 'user',
        daily_count: 0,
        last_gen_date: null
    });
    saveState();
    alert('Account created! Please login.');
    switchView('login-view');
}

function handleLogin() {
    const user = document.getElementById('login-user').value.trim();
    const pass = document.getElementById('login-pass').value;

    const found = users.find(u => u.username === user && u.password === pass);
    if (!found) return alert('Invalid credentials');

    currentUser = found;
    sessionStorage.setItem('napse_session', JSON.stringify(currentUser));
    switchView('admin-dashboard-view');
}

// PIN Management with Daily Limit
function generatePin() {
    alert('Global System Active: Please use the 100 pre-authorized PINs listed below.');
}

function renderPinList() {
    const tbody = document.getElementById('pin-list-body');
    if (!tbody) return;
    tbody.innerHTML = '';
    
    // Show all 100 hardcoded pins
    AUTHORIZED_PINS.forEach((pin, index) => {
        const shareLink = `${window.location.origin}${window.location.pathname}?pin=${pin}`;
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td style="font-family: 'JetBrains Mono', monospace; font-weight: 700;">${pin}</td>
            <td><span class="tag tag-active">Authorized</span></td>
            <td>Napse Global</td>
            <td style="display: flex; gap: 1rem; align-items: center;">
                <button class="btn-sm" style="background: rgba(139, 92, 246, 0.1); border: 1px solid var(--accent); color: var(--accent); padding: 0.4rem 0.8rem;" onclick="copyToClipboard('${shareLink}')">Copy Link</button>
            </div>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function copyToClipboard(text) {
    const el = document.createElement('textarea');
    el.value = text;
    document.body.appendChild(el);
    el.select();
    document.execCommand('copy');
    document.body.removeChild(el);
    alert('Shareable link copied to clipboard!');
}

function deletePin(key) {
    pins = pins.filter(p => p.key !== key);
    savePinsToGitHub();
    renderPinList();
    updateStats();
}

function updateStats() {
    if (!currentUser) return;
    const userPins = pins.filter(p => p.owner === currentUser.username || currentUser.role === 'staff');
    document.getElementById('stat-total').innerText = userPins.length;
    
    // Statistics for reports
    const visibleReports = currentUser.role === 'staff' ? reports : [];
    document.getElementById('stat-reports').innerText = visibleReports.length;
    
    document.querySelector('.user-name').innerText = currentUser.username;
}

function redeemPin() {
    const input = document.getElementById('pin-input').value.trim().toUpperCase();
    const pinIndex = pins.findIndex(p => p.key === input);
    
    if (pinIndex === -1) return alert('Invalid PIN');
    if (pins[pinIndex].used) return alert('This license key has already been used.');

    saveState();
    switchView('success-view');
}

function handleDownload() {
    // Show verification view instead of immediate download
    switchView('download-verify-view');
}

function verifyDownloadPin() {
    const input = document.getElementById('download-pin-input').value.trim().toUpperCase();
    
    // Check if it's in the authorized list
    if (AUTHORIZED_PINS.includes(input)) {
        // Store for download logic
        window.lastVerifiedPin = input;
        
        // Switch view
        switchView('success-view');
        window.history.replaceState({}, document.title, window.location.pathname);
    } else {
        alert('Invalid Access Pin. This license key is not recognized.');
        location.href = 'index.html';
    }
}

async function triggerClientDownload() {
    const btn = document.querySelector('.btn-dl');
    const originalText = btn.innerHTML;
    btn.innerHTML = 'Connecting to Secure Server...';
    btn.disabled = true;

    try {
        // Direct link is more reliable for large binaries and avoids CORS issues
        const pin = window.lastVerifiedPin || 'GUEST';
        const fileName = `xereca_${pin}.exe`;
        const downloadUrl = 'https://github.com/SLECET7z/napse-forensic-scanner/releases/download/v1.1/xereca.exe';

        // Use a temporary link to trigger the download
        const a = document.createElement('a');
        a.href = downloadUrl;
        a.download = fileName;
        a.style.display = 'none';
        document.body.appendChild(a);
        a.click();
        
        setTimeout(() => {
            document.body.removeChild(a);
            btn.innerHTML = 'Download Started';
            setTimeout(() => {
                btn.innerHTML = 'Download Again';
                btn.disabled = false;
            }, 3000);
        }, 100);
        
    } catch (err) {
        console.error('Download Error:', err);
        alert('Mirror 1 failed. Please try again or contact support.');
        btn.innerHTML = originalText;
        btn.disabled = false;
    }
}

// Sidebar & Tab Management
function switchTab(tabId) {
    // Update Sidebar
    document.querySelectorAll('.nav-item').forEach(b => b.classList.remove('active'));
    event.currentTarget.classList.add('active');

    // Update Workspace View
    document.querySelectorAll('.tab-pane').forEach(p => p.classList.add('hidden'));
    document.getElementById(`tab-${tabId}`).classList.remove('hidden');
    
    // Update Header Text
    const titles = {
        'keys': { t: 'License Management', d: 'Manage and distribute system access keys.' },
        'reports': { t: 'Forensic Intelligence', d: 'View and analyze scanned system data.' },
        'settings': { t: 'System Settings', d: 'Configure your forensic workstation.' }
    };
    
    document.getElementById('page-title').innerText = titles[tabId].t;
    document.getElementById('page-desc').innerText = titles[tabId].d;

    if (tabId === 'reports') renderReportsList();
    if (typeof lucide !== 'undefined') lucide.createIcons();
}

function copyDirectLink() {
    const directLink = 'https://github.com/SLECET7z/napse-forensic-scanner/releases/download/v1.1/xereca.exe';
    navigator.clipboard.writeText(directLink).then(() => {
        alert('Direct EXE link copied to clipboard!');
    });
}

// Report Management
let reports = JSON.parse(localStorage.getItem('napse_reports')) || [];

function handleReportUpload(event) {
    const file = event.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = function(e) {
        const report = {
            id: 'REP-' + Math.floor(Math.random() * 100000),
            name: file.name,
            content: e.target.result,
            date: new Date().toLocaleString(),
            owner: currentUser.username
        };
        
        reports.push(report);
        localStorage.setItem('napse_reports', JSON.stringify(reports));
        renderReportsList();
        alert('Report uploaded successfully to dashboard!');
    };
    reader.readAsText(file);
}

function renderReportsList() {
    const grid = document.getElementById('reports-list');
    if (!grid) return;
    grid.innerHTML = '';

    // STRICT PRIVACY: Only staff can see reports
    if (currentUser.role !== 'staff') {
        grid.innerHTML = '<div class="empty-state">Access Denied. Only Administrators can view forensic reports.</div>';
        return;
    }

    if (reports.length === 0) {
        grid.innerHTML = '<div class="empty-state">No reports uploaded yet. Upload your scanner .html reports here.</div>';
        return;
    }

    reports.slice().reverse().forEach(rep => {
        const card = document.createElement('div');
        card.className = 'report-card animate-in';
        card.onclick = () => viewReport(rep.id);
        card.innerHTML = `
            <div class="report-icon"><i data-lucide="file-text"></i></div>
            <div class="report-name">${rep.name}</div>
            <div class="report-date">${rep.date}</div>
            <div style="margin-top: 1rem; font-size: 0.7rem; color: var(--accent);">Click to View Full Report</div>
        `;
        grid.appendChild(card);
    });
    lucide.createIcons();
}

function viewReport(reportId) {
    const report = reports.find(r => r.id === reportId);
    if (!report) return;
    
    // Open the report content in a new window/tab
    const win = window.open('', '_blank');
    win.document.write(report.content);
    win.document.close();
}

function logout() {
    sessionStorage.removeItem('napse_session');
    location.href = 'index.html';
}

// Initial session check
if (currentUser) {
    switchView('admin-dashboard-view');
} else {
    // Show nothing by default (just matrix background)
    document.querySelectorAll('.view').forEach(v => v.classList.add('hidden'));
    switchView('login-view');
}
