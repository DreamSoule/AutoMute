#include "AutoMute.h"
#include "AudioDevice.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <locale>
#include <tlhelp32.h>
using namespace nlohmann;

struct ProcessInfo_t {
    std::wstring ImageName;
    //DWORD ProcessID;
    ProcessInfo_t() : ImageName({}) {}
    ProcessInfo_t(const WCHAR* name) { this->ImageName = name; }
    ProcessInfo_t(const std::wstring& name) : ImageName(name) {}
};

// audiodg.exe | Windows 音频设备图形隔离

#define AM_CFG_BLACKLIST AutoMuteCfg_t().add_black_list()

std::unordered_map<DWORD, ProcessInfo_t> all_ProcessInfo{};
std::map<std::wstring, AutoMuteCfg_t> AutoMute_DefaultCfg = {
    { L"audiodg.exe", AM_CFG_BLACKLIST },
    //{ L"cmd.exe", AM_CFG_BLACKLIST },
    //{ L"conhost.exe", AM_CFG_BLACKLIST },
    //{ L"telegram.exe", AM_CFG_BLACKLIST },
    //{ L"wallpaper32.exe", AM_CFG_BLACKLIST },
    //{ L"kxmixer.exe", AM_CFG_BLACKLIST },
    //{ L"steelseriessonar.exe", AM_CFG_BLACKLIST },
    //{ L"steelseriesggclient.exe", AM_CFG_BLACKLIST },
    //{ L"kugou.exe", AM_CFG_BLACKLIST },
    //{ L"qq.exe", AM_CFG_BLACKLIST },
    //{ L"wechat.exe", AM_CFG_BLACKLIST },
    //{ L"wmplayer.exe", AM_CFG_BLACKLIST },
    //{ L"vlc.exe", AM_CFG_BLACKLIST },
    //{ L"spotify.exe", AM_CFG_BLACKLIST },
    //{ L"cloudmusic.exe", AM_CFG_BLACKLIST },
    //{ L"qqmusic.exe", AM_CFG_BLACKLIST },
    //{ L"video.ui.exe", AM_CFG_BLACKLIST },
    //{ L"中文测试", AM_CFG_BLACKLIST },
    //{ L"chrome.exe", AM_CFG_BLACKLIST },
    //{ L"firefox.exe", AM_CFG_BLACKLIST },
    //{ L"msedge.exe", AM_CFG_BLACKLIST },
    //{ L"opera.exe", AM_CFG_BLACKLIST },
    //{ L"safari.exe", AM_CFG_BLACKLIST },
    //{ L"qqbrowser.exe", AM_CFG_BLACKLIST },
    //{ L"liebao.exe", AM_CFG_BLACKLIST },
    //{ L"ucweb.exe", AM_CFG_BLACKLIST },
    //{ L"qqnt.exe", AM_CFG_BLACKLIST },
    //{ L"amdrsserv.exe", AM_CFG_BLACKLIST },
};


// 将 std::wstring 转换为 std::string
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

// 将 std::string 转换为 std::wstring
std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::wstring ToLower(const std::wstring& input) {
    std::wstring output = input;
    std::transform(output.begin(), output.end(), output.begin(), ::towlower);
    return output;
}

std::string FormatTime(time_t rawTime) {
    struct tm timeInfo;
    localtime_s(&timeInfo, &rawTime);

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);

    return std::string(buffer);
}

int update_ProcessInfo() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return -1;
    }
    all_ProcessInfo.clear();
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            all_ProcessInfo.emplace(pe.th32ProcessID, ProcessInfo_t((WCHAR*)pe.szExeFile));
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return all_ProcessInfo.size();
}
std::wstring GetProcessName(DWORD processId) {
    if (all_ProcessInfo.count(processId) < 1) {
        update_ProcessInfo();
        if (all_ProcessInfo.count(processId) < 1) {
            return L"";
        }
    }
    return all_ProcessInfo[processId].ImageName;
}

#if 0
std::wstring GetProcessName(DWORD processId) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return L"";
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == processId) {
                CloseHandle(hSnapshot);
                return pe.szExeFile;
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return L"";
}
#endif


void AutoMute::LoadSettings() {
    try {
        std::ifstream file(this->cfg_file_name);
        if (!file.is_open()) {
            std::cerr << "Could not open file for reading: " << this->cfg_file_name << std::endl;
            return;
        }
        file >> this->loaded_json;
        this->dw_sleep_time = this->loaded_json["AutoMute"].value("sleep_time", 100);
        this->un_mute_all_on_exit = this->loaded_json["AutoMute"].value("un_mute_all_on_exit", true);

        std::cout << u8"黑名单进程列表: " << std::endl;
        for (auto& [key, value] : this->loaded_json["AutoMute"]["black_lists"].items()) {
            // std::wstring image_name = value;
            std::string image_name = value;
            std::wstring image_name_ws = ToLower(StringToWString(image_name));

            this->settings.emplace(image_name_ws, AM_CFG_BLACKLIST);
            std::cout << image_name << ' ';
            //std::wcout << L"已加载黑名单进程名: " << image_name_ws << std::endl;
        }
        std::cout << "\n\n";
        std::cout << "settings.size(): " << this->settings.size() << std::endl;
        // this->auto_mute_mode = j["AutoMute"].value("auto_mute_mode", 1);
        //this->settings.clear();
        //this->settings.emplace(L"audiodg.exe", AM_CFG_BLACKLIST);
        //for (const auto& [key, value] : AutoMuteCfg) {
        //    this->settings.emplace(ToLower(key), value);
        //}
    }
    catch (std::exception& e) {
        std::cerr << "LoadSettings ERROR: " << e.what() << std::endl;
    }
}

void AutoMute::SaveSettings() {
    try {
        std::ofstream file(this->cfg_file_name);
        if (!file.is_open()) {
            std::cerr << "Could not open file for writing: " << this->cfg_file_name << std::endl;
            return;
        }
        this->loaded_json["AutoMute"]["sleep_time"] = this->dw_sleep_time;
        this->loaded_json["AutoMute"]["un_mute_all_on_exit"] = this->un_mute_all_on_exit;
        int AutoMute_black_lists_i = 0;
        for (const auto& [key, value] : this->settings) {
            if (!value.bBlackList) {
                continue;
            }
            // this->loaded_json["AutoMute"]["black_lists"][AutoMute_black_lists_i] = key;
            this->loaded_json["AutoMute"]["black_lists"][AutoMute_black_lists_i] = WStringToString(ToLower(key));
            AutoMute_black_lists_i++;
        }
        file << this->loaded_json.dump(2, ' ', false); // 格式化输出，缩进为2个空格, 如果ensure_ascii为真，则输出中的所有非ASCII字符都用\uXXXX序列，结果仅由ASCII字符组成
    }
    catch (std::exception& e) {
        std::cerr << "SaveSettings ERROR: " << e.what() << std::endl;
    }
}

void AutoMute::UnMute_ALL() {
    if (!this->is_initialized) {
        return;
    }
    AudioDevice::EnumAllAudioEndpoints(this->all_sessionManagers, [](ISimpleAudioVolume* pAudioVolume, DWORD ProcessID) {
        if (!pAudioVolume) { return; }
        pAudioVolume->SetMute(FALSE, nullptr);
        //std::wcout << L"[UnMute_ALL] pid: " << ProcessID << L" image_name: " << GetProcessName(ProcessID) << std::endl;
    });
}

void AutoMute::Tick() {
    if (!this->is_initialized) {
        return;
    }
    HWND cur_fg_Window = GetForegroundWindow();
    if (cur_fg_Window == this->last_fg_Window) {
        return;
    }
    this->last_fg_Window = cur_fg_Window;
    DWORD cur_fg_ProcessID = 0;
    DWORD cur_fg_WindowThreadID = GetWindowThreadProcessId(cur_fg_Window, &cur_fg_ProcessID);
    if (cur_fg_ProcessID == this->last_fg_ProcessID) {
        return;
    }
    this->last_fg_ProcessID = cur_fg_ProcessID;
    std::wstring current_time_ws(StringToWString(FormatTime(time(NULL))));
    AudioDevice::EnumAllAudioEndpoints(this->all_sessionManagers, [cur_fg_ProcessID, this, current_time_ws](ISimpleAudioVolume* pAudioVolume, DWORD ProcessID) {
        if (!pAudioVolume) { return; }
        std::wstring wProcessName_origin(GetProcessName(ProcessID));
        std::wstring wProcessName(ToLower(wProcessName_origin));
        // if (wProcessName == L"audiodg.exe") { return; }
        if (this->settings.count(wProcessName) > 0) {
            if (!this->settings[wProcessName].bEnable || this->settings[wProcessName].bBlackList) {
                return;
            }
        }
        BOOL cur_muted = FALSE;
        pAudioVolume->GetMute(&cur_muted);
        BOOL set_Mute = ProcessID != cur_fg_ProcessID;
        if (cur_muted == set_Mute) {
            return;
        }
        pAudioVolume->SetMute(set_Mute, nullptr);
        std::wcout << '[' << current_time_ws << ']' << L" pid: " << ProcessID << L" set_mute: " << set_Mute << L" image_name: " << wProcessName_origin << std::endl;
    });
}

AutoMute::AutoMute(const std::string& _cfg_file_name) {
    this->last_fg_ProcessID = NULL;
    this->last_fg_Window = NULL;
    this->is_initialized = false;
    if (!AudioDevice::InitializeCOM()) {
        MessageBox(NULL, L"Failed: InitializeCOM", L"AutoMute", MB_OK | MB_SYSTEMMODAL);
        return;
    }
    if (!AudioDevice::GetAllAudioEndpoints(this->all_sessionManagers)) {
        MessageBox(NULL, L"Failed: GetAllAudioEndpoints", L"AutoMute", MB_OK | MB_SYSTEMMODAL);
        return;
    }
    this->cfg_file_name = _cfg_file_name;
    this->settings = AutoMute_DefaultCfg;
    // this->LoadSettings();
    this->dw_sleep_time = 100;
    this->un_mute_all_on_exit = true;
    this->dw_audio_device_count = this->all_sessionManagers.size();
    this->is_initialized = true;
}

//AutoMute::AutoMute() {
//    AutoMute("config.json");
//}

AutoMute::~AutoMute() {
    if (is_initialized) {
        // UnMute_ALL();
        for (const auto& sessionManager : this->all_sessionManagers) {
            sessionManager->Release();
        }
        AudioDevice::UninitializeCOM();
    }
    is_initialized = false;
    SaveSettings();
}
