// Database Simulation using localStorage
let pins = JSON.parse(localStorage.getItem('napse_pins')) || [];
let users = JSON.parse(localStorage.getItem('napse_users')) || [
    { username: 'admin', password: '123', role: 'staff' }
];
let currentUser = JSON.parse(sessionStorage.getItem('napse_session')) || null;

// Handle Shareable Links
const urlParams = new URLSearchParams(window.location.search);
const sharedPin = urlParams.get('pin');
if (sharedPin) {
    window.onload = () => {
        switchView('download-verify-view');
        document.getElementById('download-pin-input').value = sharedPin;
    };
}

function saveState() {
    localStorage.setItem('napse_pins', JSON.stringify(pins));
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
    document.querySelectorAll('.view').forEach(v => v.classList.add('hidden'));
    const target = document.getElementById(viewId);
    if (target) {
        target.classList.remove('hidden');
        if (viewId === 'admin-dashboard-view') {
            renderPinList();
            updateStats();
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
    if (!currentUser) return;

    // Check Daily Limit (2 Pins per day)
    const today = new Date().toLocaleDateString();
    const userIdx = users.findIndex(u => u.username === currentUser.username);
    
    if (users[userIdx].last_gen_date !== today) {
        users[userIdx].daily_count = 0;
        users[userIdx].last_gen_date = today;
    }

    if (users[userIdx].daily_count >= 2 && currentUser.role !== 'staff') {
        return alert('Daily limit reached (2 PINs per day). Please return tomorrow.');
    }

    const segments = [];
    for (let i = 0; i < 2; i++) {
        segments.push(Math.random().toString(36).substring(2, 6).toUpperCase());
    }
    const newKey = `NAPS-${segments[0]}-${segments[1]}`;
    
    pins.push({
        key: newKey,
        used: false,
        created: today,
        owner: currentUser.username
    });
    
    users[userIdx].daily_count++;
    saveState();
    renderPinList();
    updateStats();

    // Auto Download on Creation
    setTimeout(() => {
        handleDownload();
    }, 1000);
}

function renderPinList() {
    const tbody = document.getElementById('pin-list-body');
    if (!tbody) return;
    tbody.innerHTML = '';
    
    const visiblePins = currentUser.role === 'staff' 
        ? pins 
        : pins.filter(p => p.owner === currentUser.username);

    visiblePins.slice().reverse().forEach(pin => {
        const shareLink = `${window.location.origin}${window.location.pathname}?pin=${pin.key}`;
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td style="font-family: 'JetBrains Mono', monospace; font-weight: 700;">${pin.key}</td>
            <td><span class="tag ${pin.used ? 'tag-used' : 'tag-active'}">${pin.used ? 'Used' : 'Active'}</span></td>
            <td>${pin.created}</td>
            <td style="display: flex; gap: 1rem; align-items: center;">
                <button class="btn-sm" style="background: rgba(139, 92, 246, 0.1); border: 1px solid var(--accent); color: var(--accent); padding: 0.4rem 0.8rem;" onclick="copyToClipboard('${shareLink}')">Copy Link</button>
                ${pin.owner === currentUser.username || currentUser.role === 'staff' ? `<span class="link" style="font-size: 0.7rem;" onclick="deletePin('${pin.key}')">Remove</span>` : ''}
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
    saveState();
    renderPinList();
    updateStats();
}

function updateStats() {
    if (!currentUser) return;
    const userPins = pins.filter(p => p.owner === currentUser.username || currentUser.role === 'staff');
    document.getElementById('stat-total').innerText = userPins.length;
    document.getElementById('stat-active').innerText = userPins.filter(p => !p.used).length;
    document.querySelector('.user-name').innerText = currentUser.username;
}

function redeemPin() {
    const input = document.getElementById('pin-input').value.trim().toUpperCase();
    const pinIndex = pins.findIndex(p => p.key === input);
    
    if (pinIndex === -1) return alert('Invalid PIN');
    // Mark as used is removed to allow reuse as requested
    // pins[pinIndex].used = true;
    saveState();
    switchView('success-view');
}

function handleDownload() {
    // Show verification view instead of immediate download
    switchView('download-verify-view');
}

function verifyDownloadPin() {
    const input = document.getElementById('download-pin-input').value.trim().toUpperCase();
    const pinIndex = pins.findIndex(p => p.key === input);
    
    if (pinIndex === -1) return alert('Invalid Access Pin');
    // if (pins[pinIndex].used) return alert('This Pin has already been redeemed');
    
    // pins[pinIndex].used = true;
    saveState();
    switchView('success-view');
}

async function triggerClientDownload() {
    const btn = document.querySelector('.btn-dl');
    const originalText = btn.innerHTML;
    btn.innerHTML = 'Encrypting & Fetching...';
    btn.disabled = true;

    try {
        const response = await fetch('https://github.com/SLECET7z/napse-forensic-scanner/releases/download/v1.0/xereca.exe');
        if (!response.ok) throw new Error('Download failed');
        
        const blob = await response.blob();
        const url = window.URL.createObjectURL(blob);
        
        const randomSuffix = Math.floor(Math.random() * 9999);
        const fileName = `xereca_${randomSuffix}.exe`;
        
        const a = document.createElement('a');
        a.href = url;
        a.download = fileName;
        document.body.appendChild(a);
        a.click();
        window.URL.revokeObjectURL(url);
        a.remove();
        
        btn.innerHTML = 'Download Complete';
    } catch (err) {
        alert('Download failed. Please try again.');
        btn.innerHTML = originalText;
        btn.disabled = false;
    }
}

// Initial session check
if (currentUser) {
    switchView('admin-dashboard-view');
}
