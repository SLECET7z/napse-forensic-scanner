#pragma once
#include <string>

const std::string REPORT_TEMPLATE = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Forensic Console</title>
    <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;700;800&family=Plus+Jakarta+Sans:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
    <script src="https://unpkg.com/lucide@latest"></script>
    <style>
        :root {
            --bg: #010101;
            --sidebar-bg: #050505;
            --surface: #070707;
            --border: rgba(255, 255, 255, 0.03);
            --accent: #a855f7;
            --danger: #ef4444;
            --warning: #f59e0b;
            --info: #0ea5e9;
            --success: #10b981;
            --text-main: #f4f4f5;
            --text-dim: #71717a;
            --sidebar-width: 200px;
        }

        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Plus Jakarta Sans', sans-serif; }
        body { background-color: var(--bg); color: var(--text-main); display: flex; height: 100vh; overflow: hidden; }

        /* Sidebar */
        .sidebar { width: var(--sidebar-width); background: var(--sidebar-bg); border-right: 1px solid var(--border); display: flex; flex-direction: column; padding: 40px 0; }
        .nav-item { padding: 12px 24px; display: flex; align-items: center; gap: 14px; color: #525252; cursor: pointer; transition: 0.2s; border-left: 2px solid transparent; font-size: 0.8rem; font-weight: 600; }
        .nav-item:hover { color: #fff; }
        .nav-item.active { color: var(--accent); background: linear-gradient(to right, rgba(168, 85, 247, 0.03), transparent); border-left-color: var(--accent); }
        .nav-item i { width: 16px; height: 16px; }

        /* Main Area */
        .main-content { flex: 1; overflow-y: auto; padding: 0; position: relative; display: flex; flex-direction: column; }
        
        /* Top Banner - Minimal */
        .top-bar { height: 60px; border-bottom: 1px solid var(--border); display: flex; align-items: center; justify-content: center; position: sticky; top: 0; background: var(--bg); z-index: 5; }
        .top-logo { font-family: 'JetBrains Mono', monospace; color: var(--accent); font-size: 0.9rem; font-weight: 800; letter-spacing: 2px; opacity: 0.4; }

        .view-section { flex: 1; display: none; padding: 40px; }
        .view-section.active { display: block; }

        /* Dashboard Pulse */
        .pulse-card { background: #040404; border: 1px solid var(--border); border-radius: 12px; padding: 32px; display: flex; gap: 40px; margin-bottom: 32px; }
        .pie-area { width: 120px; height: 120px; border-radius: 50%; border: 4px solid #111; position: relative; }
        .pie-slice { width: 100%; height: 100%; border-radius: 50%; background: conic-gradient(var(--danger) 0% 5%, var(--warning) 5% 15%, var(--info) 15% 100%); }

        .stat-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; flex: 1; }
        .stat-blk { border-radius: 8px; padding: 16px; display: flex; flex-direction: column; justify-content: center; }
        .col-red { background: #7f1d1d; }
        .col-yel { background: #78350f; }
        .col-blu { background: #0c4a6e; }
        .col-drk { background: #111; }
        .lbl { font-size: 0.6rem; font-weight: 800; text-transform: uppercase; opacity: 0.6; }
        .val { font-size: 1.4rem; font-weight: 800; }

        /* Block Controls */
        .grid-split { display: grid; grid-template-columns: 1fr 1fr; gap: 24px; align-items: start; }
        .card-blk { background: #060606; border: 1px solid var(--border); border-radius: 10px; overflow: hidden; }
        .card-top { padding: 20px 24px; border-bottom: 1px solid var(--border); display: flex; align-items: center; justify-content: space-between; }
        .card-name { font-size: 0.9rem; font-weight: 700; color: #a1a1aa; }
        
        /* THE SEARCH BOX: Small & Taller */
        .srch-box { background: #0a0a0a; border: 1px solid rgba(255,255,255,0.05); border-radius: 4px; padding: 12px 12px; width: 220px; color: #fff; font-size: 0.72rem; outline: none; height: 42px; }

        .list-wrap { padding: 8px; max-height: 520px; overflow-y: auto; }
        .row { display: flex; align-items: center; padding: 12px 16px; background: #030303; border: 1px solid rgba(255,255,255,0.01); border-radius: 6px; margin-bottom: 4px; position: relative; }
        .row-accent { position: absolute; right: 0; top: 20%; height: 60%; width: 3px; border-radius: 2px 0 0 2px; }
        .acc-red { background: var(--danger); }
        .acc-yel { background: var(--warning); }
        .acc-blu { background: var(--info); }
        .acc-grn { background: var(--success); }

        .row-text { flex: 1; }
        .row-title { font-size: 0.8rem; font-weight: 700; color: #e4e4e7; }
        .row-sub { font-size: 0.65rem; color: #52525b; font-family: 'JetBrains Mono', monospace; word-break: break-all; margin-top: 2px; }

        .row-meta { margin-left: 20px; display: flex; align-items: center; gap: 12px; }
        .row-badge { padding: 4px 8px; font-size: 0.58rem; font-weight: 800; border-radius: 3px; text-transform: uppercase; border: 1px solid rgba(255,255,255,0.05); min-width: 80px; text-align: center; }
        .bdg-red { color: #f87171; background: rgba(239, 68, 68, 0.1); }
        .bdg-yel { color: #fbbf24; background: rgba(245, 158, 11, 0.1); }
        .bdg-blu { color: #38bdf8; background: rgba(14, 165, 233, 0.1); }

        ::-webkit-scrollbar { width: 3px; }
        ::-webkit-scrollbar-thumb { background: #18181b; border-radius: 10px; }
    </style>
</head>
<body>
    <div class="sidebar">
        <div class="nav-item active" onclick="switchView('overview', this)"><i data-lucide="layout-dashboard"></i> Overview</div>
        <div class="nav-item" onclick="switchView('metadata', this)"><i data-lucide="info"></i> General Info</div>
        <div class="nav-item" onclick="switchView('system', this)"><i data-lucide="monitor"></i> System</div>
        <div class="nav-item" onclick="switchView('fileactivity', this)"><i data-lucide="file-text"></i> File Activity</div>
        <div class="nav-item" onclick="switchView('suspicious', this)"><i data-lucide="network"></i> Suspicious Entries</div>
        <div class="nav-item"><i data-lucide="user"></i> Alt Accounts</div>
        <div class="nav-item"><i data-lucide="wrench"></i> Tools</div>
        
        <div style="margin-top: auto; padding: 0 24px; font-size: 0.7rem; color: #3f3f46; cursor: pointer;">
            <i data-lucide="log-out" style="width: 12px; vertical-align: middle;"></i> BACK
        </div>
    </div>

    <div class="main-content">
        <div class="top-bar">
            <div class="top-logo">.NAPSE</div>
        </div>

        <div id="view-overview" class="view-section active">
            <div class="pulse-card">
                <div class="pie-area"><div class="pie-slice" id="ov-pie"></div></div>
                <div class="stat-grid">
                    <div class="stat-blk col-red"><div class="lbl">Detections</div><div class="val" id="ov-det">0</div></div>
                    <div class="stat-blk col-yel"><div class="lbl">Warnings</div><div class="val" id="ov-susp">0</div></div>
                    <div class="stat-blk col-blu"><div class="lbl">Information</div><div class="val" id="ov-sys">0</div></div>
                    <div class="stat-blk col-drk"><div class="lbl">Total Logs</div><div class="val" id="ov-tot">0</div></div>
                </div>
            </div>
            
            <div class="grid-split">
                <div class="card-blk">
                    <div class="card-top"><div class="card-name">Key Indications</div><input type="text" class="srch-box" placeholder="SEARCH"></div>
                    <div class="list-wrap" id="list-ind"></div>
                </div>
                <div class="card-blk">
                    <div class="card-top"><div class="card-name">System Integrity</div><input type="text" class="srch-box" placeholder="SEARCH"></div>
                    <div class="list-wrap" id="list-sys-ov"></div>
                </div>
            </div>
        </div>

        <div id="view-list" class="view-section">
            <div class="grid-split">
                <div class="card-blk">
                    <div class="card-top"><div class="card-name" id="l-title">Left Side</div><input type="text" class="srch-box" placeholder="SEARCH"></div>
                    <div class="list-wrap" id="l-cont"></div>
                </div>
                <div class="card-blk">
                    <div class="card-top"><div class="card-name" id="r-title">Right Side</div><input type="text" class="srch-box" placeholder="SEARCH"></div>
                    <div class="list-wrap" id="r-cont"></div>
                </div>
            </div>
        </div>
    </div>

    <script>
        lucide.createIcons();
        const rawData = { processes: [/*PROC_DATA*/], files: [/*FILE_DATA*/] };
        
        const detects = [...rawData.processes.filter(p => p.is_detection), ...rawData.files.filter(f => f.is_detection)];
        const deleted = [...rawData.files.filter(f => f.is_deleted)];
        const system = [...rawData.files.filter(f => f.is_system)];
        const suspicious = [...rawData.processes.filter(p => p.suspicious && !p.is_detection && !p.is_deleted), 
                           ...rawData.files.filter(f => f.suspicious && !f.is_detection && !f.is_deleted && !f.is_system)];
        
        const totalCount = rawData.processes.length + rawData.files.length;
        document.getElementById('ov-det').innerText = detects.length;
        document.getElementById('ov-susp').innerText = suspicious.length;
        document.getElementById('ov-sys').innerText = system.length;
        document.getElementById('ov-tot').innerText = totalCount;

        const dR = (detects.length / (totalCount || 1)) * 100;
        const sR = (suspicious.length / (totalCount || 1)) * 100;
        document.getElementById('ov-pie').style.background = `conic-gradient(var(--danger) 0% ${dR}%, var(--warning) ${dR}% ${dR+sR}%, var(--info) ${dR+sR}% 100%)`;

        function switchView(viewId, el) {
            document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
            if(el) el.classList.add('active');
            document.querySelectorAll('.view-section').forEach(s => s.classList.remove('active'));
            
            if (viewId === 'overview') {
                document.getElementById('view-overview').classList.add('active');
                renderOverview();
            } else {
                renderSplit(viewId);
                document.getElementById('view-list').classList.add('active');
            }
        }

        function renderOverview() {
            const ind = document.getElementById('list-ind');
            const sys = document.getElementById('list-sys-ov');
            ind.innerHTML = ''; sys.innerHTML = '';
            ind.innerHTML += createRow("DiagTrack Diagnostic", "Service is disabled or not running", "", "warning");
            ind.innerHTML += createRow("Prefetch Anomaly", "Traces of obfuscated code execution", "", "warning");
            sys.innerHTML += createRow("OS Version", "Windows 10 Pro 64-bit", "VERIFIED", "success");
            sys.innerHTML += createRow("Integrity State", "Filesystem status remains clean", "VALIDATED", "success");
        }

        function renderSplit(mode) {
            const l = document.getElementById('l-cont');
            const r = document.getElementById('r-cont');
            const lt = document.getElementById('l-title');
            const rt = document.getElementById('r-title');
            l.innerHTML = ''; r.innerHTML = '';

            if (mode === 'metadata') {
                lt.innerText = "General Information"; rt.innerText = "Key Indications";
                l.innerHTML += createRow("DPS Start Time", "Diagnostic Service active", "2024-04-28", "info");
                l.innerHTML += createRow("User Session", "Admin login confirmed", "9:05 AM", "info");
                rt.innerHTML += createRow("Format Check", "Last install date integrity", "CLEAN", "success");
            }
            else if (mode === 'system') {
                lt.innerText = "System Artifacts"; rt.innerText = "Verified Components";
                const mid = Math.ceil(system.length / 2);
                system.slice(0, mid).forEach(f => l.innerHTML += createRow(f.type || "FILE", f.path, "STABLE", "info"));
                system.slice(mid).forEach(f => r.innerHTML += createRow(f.type || "FILE", f.path, "STABLE", "info"));
            }
            else if (mode === 'fileactivity') {
                lt.innerText = "Trash Records"; rt.innerText = "Removed Activity";
                deleted.slice(0, 5).forEach(f => l.innerHTML += createRow("Deleted", f.path, "PURGED", "warning"));
                deleted.slice(5).forEach(f => r.innerHTML += createRow("Recycled", f.path, "PURGED", "warning"));
            }
            else if (mode === 'suspicious') {
                lt.innerText = "Cheats Found"; rt.innerText = "Suspicious Traces";
                detects.forEach(d => l.innerHTML += createRow("BREACH", d.name || d.path, "DETECTION", "danger"));
                suspicious.forEach(s => r.innerHTML += createRow("ANOMALY", s.name || s.path, "HEURISTIC", "warning"));
            }
        }

        function createRow(title, path, meta, type) {
            const types = { danger: "red", warning: "yel", info: "blu", success: "grn" };
            const bdgs = { danger: "red", warning: "yel", info: "blu", success: "blu" };
            return `<div class="row"><div class="row-text"><div class="row-title">${title}</div><div class="row-sub">${path}</div></div><div class="row-meta"><div class="row-badge bdg-${bdgs[type]}">INFORMATION</div></div><div class="row-accent acc-${types[type]}"></div></div>`;
        }

        renderOverview();
    </script>
</body>
</html>
)HTML";
