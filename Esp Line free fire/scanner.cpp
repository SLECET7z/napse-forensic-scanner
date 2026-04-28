#include "pch.h"
#include "scanner.h"
#include <fstream>
#include <shellapi.h>
#include <intrin.h>
#include <dxgi.h>
#include <winhttp.h>
#include <wincrypt.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "version.lib")
#include <filesystem>

// Forward declaration
static std::string Base64Encode(const std::string& in);

void Scanner::ScanFileSystem() {
    // Standard checks
    CheckPrefetch();
    CheckRecentFiles();
    CheckDownloads();
    CheckRegistry();
    CheckFiveM();
    CheckUSN();
    
    // Ghost Search: If a file was in Prefetch or Recent but is now gone, it was definitely deleted.
    TCHAR prefetchPath[MAX_PATH];
    GetWindowsDirectory(prefetchPath, MAX_PATH);
    std::wstring pfDir = std::wstring(prefetchPath) + L"\\Prefetch\\";
    
    if (fs::exists(pfDir)) {
        try {
            for (auto const& entry : fs::directory_iterator(pfDir)) {
                std::wstring pfName = entry.path().filename().wstring();
                // Prefetch filenames look like PROGRAM-HASH.pf
                size_t dash = pfName.find_last_of(L"-");
                if (dash != std::wstring::npos) {
                    std::wstring originalName = pfName.substr(0, dash);
                    // This is an approximation, but useful for forensics
                    
                    // If the name matches suspicious keywords but the file is gone from common paths
                    std::vector<std::wstring> suspect = { L"CHEAT", L"HACK", L"EULEN", L"CLEANER" };
                    for (const auto& s : suspect) {
                        if (originalName.find(s) != std::wstring::npos) {
                            FileResult res;
                            res.path = originalName + L" (Trace Found in Prefetch)";
                            res.type = L"Deleted Execution Trace";
                            res.is_deleted = true;
                            res.suspicious = true;
                            res.reason = L"Execution record found in Prefetch, but file has been removed/hidden.";
                            m_Files.push_back(res);
                            break;
                        }
                    }
                }
            }
        } catch (...) {}
    }
}

void Scanner::StartScan() {
    m_Progress = 0.0f;
    m_Finished = false;
    m_Processes.clear();
    m_Files.clear();

    // Stage 1: System Details
    m_Progress = 0.05f;
    GatherSystemInfo();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stage 2: Processes
    m_Progress = 0.2f;
    ScanProcesses();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stage 3: Prefetch
    m_Progress = 0.4f;
    CheckPrefetch();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stage 4: Recent Files
    m_Progress = 0.5f;
    CheckRecentFiles();
    m_Progress = 0.55f;
    CheckTemp();
    m_Progress = 0.65f;
    CheckDownloads();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stage 6: Registry
    m_Progress = 0.92f;
    CheckRegistry();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stage 7: FiveM
    m_Progress = 0.95f;
    CheckFiveM();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stage 9: Universal Disk Scan
    m_Progress = 0.98f;
    ScanAllDrives();

    // Stage 10: USN Journal / Recycle Bin
    m_Progress = 0.99f;
    CheckUSN();

    m_Progress = 1.0f;
    m_Finished = true;
}

void Scanner::CheckRecentFiles() {
    wchar_t* appData;
    size_t len;
    _wdupenv_s(&appData, &len, L"APPDATA");
    if (!appData) return;

    std::wstring path = std::wstring(appData) + L"\\Microsoft\\Windows\\Recent";
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            std::wstring fileName = entry.path().filename().wstring();
            
            // For every recent file shortcut, check if the original file still exists.
            // If it doesn't, it was recently deleted/moved.
            // This is a common way to find evidence of 'cleaning'.
            
            FileResult res;
            res.path = entry.path().wstring();
            res.type = L"Recent Link";
            res.is_deleted = false; 
            res.is_detection = false;
            res.suspicious = false;
            res.is_system = IsSystemPath(res.path);
            res.lastModified = L"N/A";

            if (IsSuspiciousFile(fileName) && !res.is_system) {
                res.suspicious = true;
                res.reason = L"Recently accessed potential cheat artifact.";
                m_Files.push_back(res);
            } else if (res.is_system) {
                res.reason = L"Verified System Link";
                m_Files.push_back(res);
            }
        }
    } catch (...) {}
    free(appData);
}

void Scanner::ScanProcesses() {
    std::vector<std::wstring> detectPatterns = { L"eulen", L"skript", L"redengine", L"hx-cheats", L"nexus", L"nexusmenu", L"f0rest", L"desudo", L"lumia", L"absolute", L"dopamine", L"fallout" };
    std::vector<std::wstring> suspiciousPatterns = { L"injector", L"cheat", L"hack", L"freefire", L"bypass", L"spoofer", L"cleaner", L"unban", L"aimbot", L"wallhack", L"esp", L"triggerbot", L"recoil" };

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnap, &pe)) {
        do {
            std::wstring name = pe.szExeFile;
            bool isDet = false;
            bool isSusp = false;
            std::wstring reason;

            for (auto& p : detectPatterns) {
                if (name.find(p) != std::string::npos) { isDet = true; reason = L"Confirmed Forensic Signature: " + p; break; }
            }
            if (!isDet) {
                for (auto& p : suspiciousPatterns) {
                    if (name.find(p) != std::string::npos) { isSusp = true; reason = L"Heuristic Anomaly: " + p; break; }
                }
            }

            if (isDet || isSusp) {
                ProcessInfo info;
                info.name = name;
                info.pid = pe.th32ProcessID;
                info.suspicious = true;
                info.is_detection = isDet;
                info.flagReason = reason;
                m_Processes.push_back(info);
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
}

bool Scanner::IsSuspiciousProcess(const std::wstring& name) {
    std::vector<std::wstring> blacklist = { 
        L"eulen", L"redengine", L"skript", L"nexus", L"hx-cheats", L"f0rest", L"desudo", 
        L"lumia", L"absolute", L"dopamine", L"fallout", L"injector", L"aimbot", L"unban",
        L"spoofer", L"bypass", L"esp", L"wallhack", L"triggerbot", L"recoil", L"norecoil",
        L"silent", L"magic", L"bullet", L"rage", L"legit", L"cheat", L"hack"
    };

    for (const auto& item : blacklist) {
        if (name.find(item) != std::wstring::npos) return true;
    }
    return false;
}

void Scanner::CheckPrefetch() {
    std::wstring path = L"C:\\Windows\\Prefetch";
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            std::wstring fileName = entry.path().filename().wstring();
            
            // Prefetch usually identifies the executable name before the hash
            size_t dash = fileName.find_last_of(L"-");
            if (dash != std::wstring::npos) {
                std::wstring baseExe = fileName.substr(0, dash);
                
                bool isMatch = IsSuspiciousFile(baseExe);
                std::wstring originalName = L"";
                
                // If we have a potential hit, or even if not, try to resolve the internal name
                // Note: We'd need the actual path, but for prefetch we often only have the name.
                // If possible, we can try to find the exe to get more info.
                
                if (isMatch) {
                    FileResult res;
                    res.path = baseExe + L" (Trace in Prefetch)";
                    res.type = L"Prefetch Record";
                    
                    std::wstring lowerExe = baseExe;
                    std::transform(lowerExe.begin(), lowerExe.end(), lowerExe.begin(), ::towlower);
                    
                    std::vector<std::wstring> badCheats = { L"eulen", L"skript", L"redengine", L"hx-cheats", L"nexus", L"f0rest", L"desudo" };
                    bool isHardDetection = false;
                    for(const auto& bad : badCheats) { if(lowerExe.find(bad) != std::wstring::npos) { isHardDetection = true; break; } }

                    res.suspicious = !res.is_system;
                    res.is_detection = isHardDetection;
                    res.reason = isHardDetection ? L"Confirmed execution of blacklisted cheat software." : L"Suspicious execution trace found in system prefetch.";
                    
                    m_Files.push_back(res);
                }
            }
        }
    } catch (...) {}
}


void Scanner::CheckDownloads() {
    wchar_t* userProfile;
    size_t len;
    _wdupenv_s(&userProfile, &len, L"USERPROFILE");
    if (!userProfile) return;

    std::wstring path = std::wstring(userProfile) + L"\\Downloads";
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (IsSuspiciousFile(entry.path().filename().wstring())) {
                FileResult res;
                res.path = entry.path().wstring();
                res.type = L"Download";
                res.is_deleted = false;
                res.is_system = (res.path.find(L"Microsoft") != std::wstring::npos || 
                                 res.path.find(L"Epic Games") != std::wstring::npos);
                res.suspicious = !res.is_system;
                res.is_detection = res.path.find(L"Cheat") != std::wstring::npos || res.path.find(L"Loader") != std::wstring::npos;
                res.reason = res.is_system ? L"Verified Installer/Package" : L"Suspicious keywords in Download folder";
                res.lastModified = L"N/A";
                m_Files.push_back(res);
            }
        }
    } catch (...) {}
    free(userProfile);
}

bool Scanner::IsSuspiciousFile(const std::wstring& name) {
    std::vector<std::wstring> keywords = {
        L"cheat", L"hack", L"eulen", L"skript", L"executor", L"injector",
        L"aimbot", L"wallhack", L"triggerbot", L"viperx", L"ocean", 
        L"redengine", L"lumia", L"fallout", L"freefire", L"internal", 
        L"external", L"manual map", L"0xdead", L"xereca", L"skript", L"nexus", 
        L"hx-cheats", L"f0rest", L"desudo", L"absolute", L"dopamine", 
        L"spoofer", L"unban", L"bypass", L"norecoil", L"silent", L"magic", 
        L"bullet", L"rage", L"legit", L"menu"
    };

    std::wstring lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

    for (const auto& kw : keywords) {
        if (lowerName.find(kw) != std::wstring::npos) return true;
    }
    return false;
}

void Scanner::GatherSystemInfo() {
    HKEY hKey;
    // Get OS
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buf[256];
        DWORD sz = sizeof(buf);
        if (RegQueryValueExW(hKey, L"ProductName", NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS) {
            m_SysInfo.os = buf;
        }
        RegCloseKey(hKey);
    }
    if (m_SysInfo.os.empty()) m_SysInfo.os = L"Windows 10/11";

    // Get CPU
    int cpuInfo[4] = { -1 };
    char cpuId[0x40];
    memset(cpuId, 0, sizeof(cpuId));
    __cpuid(cpuInfo, 0x80000002);
    memcpy(cpuId, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000003);
    memcpy(cpuId + 16, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000004);
    memcpy(cpuId + 32, cpuInfo, sizeof(cpuInfo));
    std::string cpuStr = cpuId;
    m_SysInfo.cpu = std::wstring(cpuStr.begin(), cpuStr.end());

    // Get GPU
    IDXGIFactory* pFactory;
    if (CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory) == S_OK) {
        IDXGIAdapter* pAdapter;
        if (pFactory->EnumAdapters(0, &pAdapter) == S_OK) {
            DXGI_ADAPTER_DESC desc;
            pAdapter->GetDesc(&desc);
            m_SysInfo.gpu = desc.Description;
            pAdapter->Release();
        }
        pFactory->Release();
    }

    // Get Current Time
    auto now_now = std::chrono::system_clock::now();
    std::time_t t_now = std::chrono::system_clock::to_time_t(now_now);
    char timeBuf_now[64];
    struct tm timeInfo_now;
    localtime_s(&timeInfo_now, &t_now);
    strftime(timeBuf_now, sizeof(timeBuf_now), "%Y-%m-%d %H:%M:%S", &timeInfo_now);
    std::string timeStr_final = timeBuf_now;
    m_SysInfo.time = std::wstring(timeStr_final.begin(), timeStr_final.end());

    // Get Install Date
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD installData = 0;
        DWORD dataSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"InstallDate", NULL, NULL, (LPBYTE)&installData, &dataSize) == ERROR_SUCCESS) {
            std::time_t installTime = (std::time_t)installData;
            struct tm timeinfo;
            localtime_s(&timeinfo, &installTime);
            wchar_t dateBuf[64];
            wcsftime(dateBuf, 64, L"%Y-%m-%d", &timeinfo);
            m_SysInfo.installDate = dateBuf;

            double diff = std::difftime(t_now, installTime);
            m_SysInfo.formatDetected = (diff < 604800); // 7 days
        }
        RegCloseKey(hKey);
    }
    m_SysInfo.vpn = false;
}
void Scanner::CheckRegistry() {
    HKEY hKey;
    // Check UserAssist (Execution history)
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t subKey[256];
        DWORD subKeyLen = 256;
        for (DWORD i = 0; RegEnumKeyExW(hKey, i, subKey, &subKeyLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; ++i) {
            subKeyLen = 256;
            HKEY hSubKey;
            if (RegOpenKeyExW(hKey, subKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                HKEY hCount;
                if (RegOpenKeyExW(hSubKey, L"Count", 0, KEY_READ, &hCount) == ERROR_SUCCESS) {
                    wchar_t valueName[512];
                    DWORD valueNameLen = 512;
                    for (DWORD j = 0; RegEnumValueW(hCount, j, valueName, &valueNameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; ++j) {
                        valueNameLen = 512;
                        // UserAssist uses ROT13. Simple forensic check for keyword patterns
                        if (IsSuspiciousFile(valueName)) {
                            FileResult res;
                            res.path = L"Registry: UserAssist Trace";
                            res.type = L"Registry";
                            res.suspicious = true;
                            res.is_deleted = false;
                            res.is_detection = false;
                            res.is_system = false;
                            m_Files.push_back(res);
                        }
                    }
                    RegCloseKey(hCount);
                }
                RegCloseKey(hSubKey);
            }
        }
        RegCloseKey(hKey);
    }
}

void Scanner::CheckFiveM() {
    wchar_t* appData;
    size_t len;
    _wdupenv_s(&appData, &len, L"LOCALAPPDATA");
    if (!appData) return;

    std::wstring fivemPath = std::wstring(appData) + L"\\FiveM\\FiveM.app";
    std::wstring logPath = fivemPath + L"\\CitizenFX.log.1"; // Previous session log

    std::ifstream logFile(logPath);
    if (logFile.is_open()) {
        std::string line;
        while (std::getline(logFile, line)) {
            if (line.find("skript") != std::string::npos || line.find("eulen") != std::string::npos) {
                FileResult res;
                res.path = logPath;
                res.type = L"FiveM Log";
                res.is_deleted = false;
                res.is_detection = true;
                res.is_system = false;
                res.suspicious = true;
                res.reason = L"Cheat Execution History in CitizenFX.log";
                m_Files.push_back(res);
                break;
            }
        }
    }
    free(appData);
}

void Scanner::CheckUSN() {
    // Scan the Recycle Bin on all available logical drives.
    // This will now catch EVERYTHING in the trash, not just specific names.
    std::vector<std::wstring> drives = { L"C:\\", L"D:\\", L"H:\\" };
    
    for (const auto& drive : drives) {
        std::wstring recycler = drive + L"$Recycle.Bin";
        if (fs::exists(recycler)) {
            try {
                for (auto const& dir_entry : fs::recursive_directory_iterator(recycler, fs::directory_options::skip_permission_denied)) {
                    if (dir_entry.is_regular_file()) {
                        std::wstring path = dir_entry.path().wstring();
                        std::wstring fileName = dir_entry.path().filename().wstring();

                        std::wstring resolvedPath = ResolveRecycleName(path);
                        std::wstring resolvedName = fs::path(resolvedPath).filename().wstring();

                        std::wstring ext = fs::path(resolvedPath).extension().wstring();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

                        bool isScriptOrExe = (ext == L".exe" || ext == L".dll" || ext == L".sys" || 
                                              ext == L".bat" || ext == L".cmd" || ext == L".sh" || 
                                              ext == L".ps1" || ext == L".vbs");
                        
                        bool isSuspicious = IsSuspiciousFile(resolvedName);

                        if (isScriptOrExe || isSuspicious) {
                            FileResult res;
                            res.path = resolvedPath;
                            res.type = L"Recycled Artifact";
                            res.is_deleted = true;
                            res.is_detection = false;
                            res.is_system = false;
                            res.suspicious = true;
                            res.reason = isSuspicious ? L"Known cheat keyword in recycled file." : L"Suspicious script/binary in recycle bin.";
                            m_Files.push_back(res);
                        }
                    }
                }
            } catch (...) {}
        }
    }
}

#include "report_template.h"

void Scanner::GenerateReport(const std::string& outputPath) {
    std::string html = REPORT_TEMPLATE;

    // Build Process Data
    std::string procData = "";
    for (size_t i = 0; i < m_Processes.size(); ++i) {
        const auto& p = m_Processes[i];
        std::string name(p.name.begin(), p.name.end());
        std::string reason(p.flagReason.begin(), p.flagReason.end());
        procData += "{\"pid\":" + std::to_string(p.pid) + ",\"name\":\"" + name + "\",\"suspicious\":" + (p.suspicious ? "true" : "false") + ",\"is_detection\":" + (p.is_detection ? "true" : "false") + ",\"reason\":\"" + reason + "\"}";
        if (i < m_Processes.size() - 1) procData += ",";
    }

    // Build File Data
    std::string fileData = "";
    for (size_t i = 0; i < m_Files.size(); ++i) {
        const auto& f = m_Files[i];
        std::string path(f.path.begin(), f.path.end());
        // Escape backslashes for JS
        size_t pos = 0;
        while ((pos = path.find("\\", pos)) != std::string::npos) {
            path.replace(pos, 1, "\\\\");
            pos += 2;
        }
        std::string type(f.type.begin(), f.type.end());
        std::string reason(f.reason.begin(), f.reason.end());
        if (reason.empty()) reason = f.suspicious ? "Heuristic Flag" : "Validated Component";
        fileData += "{\"path\":\"" + path + "\",\"type\":\"" + type + "\",\"suspicious\":" + (f.suspicious ? "true" : "false") + 
                    ",\"is_detection\":" + (f.is_detection ? "true" : "false") + 
                    ",\"is_deleted\":" + (f.is_deleted ? "true" : "false") + 
                    ",\"is_system\":" + (f.is_system ? "true" : "false") + 
                    ",\"reason\":\"" + reason + "\"}";
        if (i < m_Files.size() - 1) fileData += ",";
    }

    // Inject
    size_t procPos = html.find("/*PROC_DATA*/");
    if (procPos != std::string::npos) html.replace(procPos, 13, procData);

    size_t filePos = html.find("/*FILE_DATA*/");
    if (filePos != std::string::npos) html.replace(filePos, 13, fileData);

    // Inject System Info
    auto inject = [&](const std::string& placeholder, const std::wstring& value) {
        std::string v(value.begin(), value.end());
        size_t p = html.find(placeholder);
        if (p != std::string::npos) html.replace(p, placeholder.length(), v);
    };

    inject("Windows 10/11 x64", m_SysInfo.os);
    inject("Just now", m_SysInfo.time);
    inject("no", m_SysInfo.vpn ? L"yes" : L"no");
    inject("2024-01-01", m_SysInfo.installDate);
    inject("FORMAT_STATE", m_SysInfo.formatDetected ? L"RECENT FORMAT" : L"CLEAN HISTORY");

    // Save
    std::ofstream out(outputPath);
    out << html;
    out.close();

    // Prepare Cloud View URL
    std::string base64 = Base64Encode(html);
    std::string cloudUrl = "https://slecet7z.github.io/napse-forensic-scanner/?report=" + base64;
    
    // Open Cloud View
    ShellExecuteA(NULL, "open", cloudUrl.c_str(), NULL, NULL, SW_SHOW);

    // Self-Delete logic
    TCHAR szFile[MAX_PATH];
    GetModuleFileName(NULL, szFile, MAX_PATH);
    
    std::string currentPath = std::filesystem::path(szFile).string();
    std::string cmd = "cmd.exe /c timeout /t 3 & del \"" + currentPath + "\" & del report.html";
    ShellExecuteA(NULL, "open", "cmd.exe", cmd.c_str(), NULL, SW_HIDE);
    
    exit(0);
}

static std::string Base64Encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

void Scanner::ScanAllDrives() {
    wchar_t drives[256];
    if (GetLogicalDriveStringsW(256, drives)) {
        wchar_t* drive = drives;
        while (*drive) {
            std::wstring drivePath = drive;
            ScanDirectoryRecursive(drivePath, 0);
            drive += lstrlenW(drive) + 1;
        }
    }
}

void Scanner::ScanDirectoryRecursive(const std::wstring& path, int depth, bool inSuspiciousParent) {
    if (depth > 6) return;

    try {
        for (const auto& entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
            if (entry.is_directory()) {
                std::wstring dirName = entry.path().filename().wstring();
                
                if (dirName == L"Windows" || dirName == L"System32" || dirName == L"ProgramData" || 
                    dirName == L"$Recycle.Bin" || dirName == L"AppData" || dirName == L"node_modules" ||
                    dirName == L".git" || dirName == L".vs")
                    continue;

                bool currentSuspicious = inSuspiciousParent || IsSuspiciousFile(dirName);
                if (currentSuspicious && !inSuspiciousParent) {
                    FileResult res;
                    res.path = entry.path().wstring();
                    res.type = L"Suspicious Folder";
                    res.suspicious = true;
                    res.is_system = IsSystemPath(res.path);
                    if (!res.is_system) m_Files.push_back(res);
                }

                ScanDirectoryRecursive(entry.path().wstring(), depth + 1, currentSuspicious);
            }
            else {
                std::wstring fileName = entry.path().filename().wstring();
                std::wstring ext = entry.path().extension().wstring();

                bool isMatch = IsSuspiciousFile(fileName);
                bool shouldFlag = isMatch || (inSuspiciousParent && (ext == L".exe" || ext == L".dll" || ext == L".sys"));

                if (shouldFlag) {
                    FileResult res;
                    res.path = entry.path().wstring();
                    res.type = isMatch ? L"Cheat Artifact" : L"Suspicious Project Binary";
                    res.suspicious = true;
                    res.is_deleted = false;
                    res.is_detection = false;
                    res.is_system = IsSystemPath(res.path);
                    
                    if (!res.is_system) {
                        m_Files.push_back(res);
                        if (ext == L".exe" || ext == L".dll") {
                            CheckVirusTotal(entry.path().wstring());
                        }
                    }
                }
            }
        }
    } catch (...) {}
}

bool Scanner::IsSystemPath(const std::wstring& path) {
    static const std::vector<std::wstring> systemKws = {
        L"Microsoft", L"Epic Games", L"dotnet", L"Windows Kits", L"ucrt", 
        L"Program Files", L"Windows\\System32", L"Windows\\SysWOW64",
        L"Common Files", L"SteamLibrary\\steamapps\\common", L"pinnacle",
        L"NVIDIA Corporation", L"AMD", L"Intel", L"DriverStore", L"vcpkg"
    };
    for (const auto& kw : systemKws) {
        if (path.find(kw) != std::wstring::npos) return true;
    }
    return false;
}

std::wstring Scanner::ResolveRecycleName(const std::wstring& iPath) {
    std::wstring fileName = fs::path(iPath).filename().wstring();
    if (fileName.find(L"$I") != 0) return iPath; // Not an index file

    std::ifstream file(iPath, std::ios::binary);
    if (!file.is_open()) return iPath;

    // Skip the header (Offset 24 is where the path starts in Win10+)
    file.seekg(24, std::ios::beg);
    
    wchar_t originalPath[MAX_PATH] = { 0 };
    file.read((char*)originalPath, sizeof(originalPath));
    file.close();

    std::wstring resolved(originalPath);
    if (resolved.empty()) return iPath;
    
    return resolved;
}

std::wstring Scanner::GetOriginalName(const std::wstring& path) {
    DWORD dwHandle = 0;
    DWORD dwSize = GetFileVersionInfoSizeW(path.c_str(), &dwHandle);
    if (dwSize == 0) return L"";

    std::vector<BYTE> buffer(dwSize);
    if (!GetFileVersionInfoW(path.c_str(), dwHandle, dwSize, buffer.data())) return L"";

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;
    UINT cbTranslate;

    if (VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) {
        for (unsigned int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) {
            wchar_t subBlock[256];
            
            // Try OriginalFilename then FileDescription
            swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\OriginalFilename", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
            wchar_t* lpBuffer = nullptr;
            UINT dwBytes = 0;
            if (VerQueryValueW(buffer.data(), subBlock, (LPVOID*)&lpBuffer, &dwBytes) && dwBytes > 0) return lpBuffer;

            swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\FileDescription", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
            if (VerQueryValueW(buffer.data(), subBlock, (LPVOID*)&lpBuffer, &dwBytes) && dwBytes > 0) return lpBuffer;
        }
    }
    return L"";
}

bool Scanner::IsRandomName(const std::wstring& name) {
    std::wstring stem = fs::path(name).stem().wstring();
    if (stem.length() < 4) return false;
    
    bool allHex = true;
    for (wchar_t c : stem) {
        if (!iswxdigit(c)) { allHex = false; break; }
    }
    return allHex;
}

std::string Scanner::CalculateHash(const std::wstring& filePath) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return "";

    std::string hashStr = "";
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            BYTE buffer[1024];
            DWORD bytesRead = 0;
            while (ReadFile(hFile, buffer, 1024, &bytesRead, NULL) && bytesRead > 0) {
                CryptHashData(hHash, buffer, bytesRead, 0);
            }
            BYTE hash[32];
            DWORD hashLen = 32;
            if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                char hex[65];
                for (int i = 0; i < 32; i++) sprintf_s(hex + i * 2, 3, "%02X", hash[i]);
                hashStr = hex;
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    CloseHandle(hFile);
    return hashStr;
}

bool Scanner::CheckVirusTotal(const std::wstring& filePath) {
    if (m_VTKey.empty()) m_VTKey = "76e48bf90c91012b0419978b1c1cc5d64c1682c794e378f2813ccedff35702d8";
    std::string hash = CalculateHash(filePath);
    if (hash.empty()) return false;

    HINTERNET hSession = WinHttpOpen(L"OceanScanner / 1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"www.virustotal.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    std::wstring url = L"/api/v3/files/" + std::wstring(hash.begin(), hash.end());
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", url.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    if (hRequest) {
        std::wstring header = L"x-apikey: " + std::wstring(m_VTKey.begin(), m_VTKey.end());
        WinHttpAddRequestHeaders(hRequest, header.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD);

        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            if (WinHttpReceiveResponse(hRequest, NULL)) {
                DWORD size = 0;
                WinHttpQueryDataAvailable(hRequest, &size);
                if (size > 0) {
                    char* buffer = new char[size + 1];
                    DWORD read = 0;
                    WinHttpReadData(hRequest, buffer, size, &read);
                    buffer[read] = 0;
                    std::string response = buffer;
                    delete[] buffer;

                    if (response.find("\"malicious\"") != std::string::npos && response.find("\"malicious\":0") == std::string::npos) {
                        FileResult res;
                        res.path = filePath + L" (Flagged by VT)";
                        res.type = L"VirusTotal";
                        res.suspicious = true;
                        m_Files.push_back(res);
                    }
                }
            }
        }
        WinHttpCloseHandle(hRequest);
    }
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return true;
}

void Scanner::CheckTemp() {
    std::vector<std::wstring> paths;
    
    // User Temp
    wchar_t* tempPath = nullptr;
    size_t len = 0;
    _wdupenv_s(&tempPath, &len, L"TEMP");
    if (tempPath) {
        paths.push_back(tempPath);
        free(tempPath);
    }
    
    // System Temp
    paths.push_back(L"C:\\Windows\\Temp");

    for (const auto& path : paths) {
        if (!fs::exists(path)) continue;
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                std::wstring ext = entry.path().extension().wstring();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

                bool isBinary = (ext == L".exe" || ext == L".dll" || ext == L".sys");
                bool isMatch = IsSuspiciousFile(entry.path().filename().wstring());
                bool isRandom = IsRandomName(entry.path().filename().wstring());

                if (isBinary || isMatch || isRandom) {
                    FileResult res;
                    res.path = entry.path().wstring();
                    
                    std::wstring originalName = GetOriginalName(res.path);
                    if (!originalName.empty()) {
                        res.path += L" [Unmasked: " + originalName + L"]";
                    }

                    res.type = isRandom ? L"Randomized Breach" : L"Temp Violation";
                    res.is_deleted = false;
                    res.is_detection = true; 
                    res.is_system = false;
                    res.suspicious = true;
                    res.reason = isRandom ? L"High-entropy randomized filename detected (Cheat Heuristic)." 
                                          : (isBinary ? L"Restricted executable residency in temporary folder." : L"Known cheat signature found in temp.");
                    m_Files.push_back(res);
                }
            }
        } catch (...) {}
    }
}

