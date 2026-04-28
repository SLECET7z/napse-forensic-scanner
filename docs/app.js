// Database Simulation using localStorage
let pins = JSON.parse(localStorage.getItem('napse_pins')) || [
    { key: 'NAPS-1234-5678', used: false, created: new Date().toLocaleDateString(), owner: 'system' }
];

let users = JSON.parse(localStorage.getItem('napse_users')) || [
    { username: 'admin', password: '123', role: 'staff' }
];

let currentUser = null;

function saveState() {
    localStorage.setItem('napse_pins', JSON.stringify(pins));
    localStorage.setItem('napse_users', JSON.stringify(users));
}

// View Switching Logic
function switchView(viewId) {
    document.querySelectorAll('.view').forEach(v => {
        v.classList.add('hidden');
    });
    
    const target = document.getElementById(viewId);
    target.classList.remove('hidden');
    
    if (viewId === 'admin-dashboard-view' || viewId === 'user-dashboard-view') {
        renderPinList();
        updateStats();
    }
    
    if (typeof lucide !== 'undefined') {
        lucide.createIcons();
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

    users.push({ username: user, password: pass, role: 'user' });
    saveState();
    alert('Account created successfully! You can now login.');
    switchView('login-view');
}

function handleLogin() {
    const user = document.getElementById('login-user').value.trim();
    const pass = document.getElementById('login-pass').value;

    const found = users.find(u => u.username === user && u.password === pass);
    if (!found) return alert('Invalid username or password');

    currentUser = found;
    if (found.role === 'staff') {
        switchView('admin-dashboard-view');
    } else {
        // We'll reuse the dashboard for users but filter content
        switchView('admin-dashboard-view');
    }
}

function adminLogin() {
    const pass = document.getElementById('admin-pass').value;
    const found = users.find(u => u.role === 'staff' && u.password === pass);
    if (found) {
        currentUser = found;
        switchView('admin-dashboard-view');
    } else {
        alert('Invalid Access Secret');
    }
}

// PIN Management
function generatePin() {
    if (!currentUser) return;

    const segments = [];
    for (let i = 0; i < 2; i++) {
        segments.push(Math.random().toString(36).substring(2, 6).toUpperCase());
    }
    const newKey = `NAPS-${segments[0]}-${segments[1]}`;
    
    pins.push({
        key: newKey,
        used: false,
        created: new Date().toLocaleDateString(),
        owner: currentUser.username
    });
    
    saveState();
    renderPinList();
    updateStats();
}

function renderPinList() {
    const tbody = document.getElementById('pin-list-body');
    tbody.innerHTML = '';
    
    // Filter pins based on user role
    const visiblePins = currentUser && currentUser.role === 'staff' 
        ? pins 
        : pins.filter(p => p.owner === currentUser.username);

    visiblePins.slice().reverse().forEach(pin => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td style="font-family: 'JetBrains Mono', monospace; font-weight: 700;">${pin.key}</td>
            <td><span class="tag ${pin.used ? 'tag-used' : 'tag-active'}">${pin.used ? 'Used' : 'Active'}</span></td>
            <td>${pin.created}</td>
            <td>${pin.owner === currentUser.username || currentUser.role === 'staff' ? `<span class="link" onclick="deletePin('${pin.key}')">Remove</span>` : ''}</td>
        `;
        tbody.appendChild(tr);
    });
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
    
    // Update display name
    document.querySelector('.user-name').innerText = currentUser.username;
}

// Redemption Logic
function redeemPin() {
    const input = document.getElementById('pin-input').value.trim().toUpperCase();
    const pinIndex = pins.findIndex(p => p.key === input);
    
    if (pinIndex === -1) return alert('Invalid Access Pin');
    if (pins[pinIndex].used) return alert('This Pin has already been redeemed');
    
    pins[pinIndex].used = true;
    saveState();
    switchView('success-view');
}

function handleDownload() {
    alert('Preparing your forensic client build...\n\nYour one-time download session is active.');
}
