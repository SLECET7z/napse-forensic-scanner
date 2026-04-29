#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

struct ProcessInfo {
    std::wstring name;
    DWORD pid;
    bool suspicious;
    bool is_detection; // Known cheat match
    std::wstring flagReason;
};

struct FileResult {
    std::wstring path;
    std::wstring type; 
    std::wstring lastModified;
    bool suspicious;
    bool is_detection; // Known cheat file match
    bool is_deleted;   // Found in USN/Recycler but file is gone
    bool is_system;    // Recognized system component (Microsoft, Epic, etc.)
    std::wstring reason;
};

class Scanner {
public:
    Scanner() : m_Progress(0.0f), m_Finished(false) {}

    void StartScan();
    float GetProgress() const { return m_Progress; }
    bool IsFinished() const { return m_Finished; }

    const std::vector<ProcessInfo>& GetProcesses() const { return m_Processes; }
    const std::vector<FileResult>& GetFiles() const { return m_Files; }

    void GenerateReport(const std::string& outputPath);
    bool SendToDiscord(const std::string& webhookUrl, const std::string& reportPath);
    bool UploadToGitHub(const std::string& token, const std::string& reportPath, const std::string& fileName);

    void SetPin(const std::string& pin) { m_Pin = pin; }
    std::string GetPin() const { return m_Pin; }
    bool ValidatePin(); // Remote check against GitHub pins.json

    struct SystemInfo {
        std::wstring os;
        std::wstring cpu;
        std::wstring gpu;
        std::wstring time;
        std::wstring installDate;
        bool formatDetected;
        bool vpn;
    };

    const SystemInfo& GetSystemInfo() const { return m_SysInfo; }

private:
    void GatherSystemInfo();
    void ScanProcesses();
    void ScanFileSystem();
    void CheckPrefetch();
    void CheckRecentFiles();
    void CheckDownloads();
    void CheckTemp();
    void CheckRegistry();
    void ScanRegistry() { CheckRegistry(); } // Alias for compatibility
    void ScanMemory();
    void ScanProcessStrings(DWORD pid, const std::wstring& procName);
    void CheckJournal();
    void CheckFiveM();
    void CheckUSN();
    void ScanAllDrives();
    void ScanDirectoryRecursive(const std::wstring& path, int depth, bool inSuspiciousParent = false);
    bool CheckVirusTotal(const std::wstring& filePath);
    bool IsSystemPath(const std::wstring& path);
    std::wstring ResolveRecycleName(const std::wstring& iPath);
    std::wstring GetOriginalName(const std::wstring& path);
    bool IsRandomName(const std::wstring& name);
    std::string CalculateHash(const std::wstring& filePath);
    
    std::string m_VTKey;
    std::string m_Pin;
    SystemInfo m_SysInfo;
    std::vector<ProcessInfo> m_Processes;
    std::vector<FileResult> m_Files;
    float m_Progress;
    bool m_Finished;

    bool IsSuspiciousProcess(const std::wstring& name);
    bool IsSuspiciousFile(const std::wstring& name);
};
