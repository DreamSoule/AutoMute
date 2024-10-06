#pragma once
#include <windows.h>
#include <audiopolicy.h>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include "json.hpp"

struct AutoMuteCfg_t {
    bool bEnable;
    bool bBlackList;
    bool Muted;
    AutoMuteCfg_t& add_black_list() { this->bBlackList = true; return *this; };
};

class AutoMute
{
public:
    AutoMute(const std::string& _cfg_file_name);
    AutoMute() = delete;
    ~AutoMute();

    void LoadSettings();

    void SaveSettings();

    void UnMute_ALL();
    void Tick();
    bool is_un_mute_all_on_exit() const { return this->un_mute_all_on_exit; }
    DWORD sleep_time() const { return this->dw_sleep_time; }
    DWORD audio_device_count() const { return this->dw_audio_device_count; }
private:
    std::map<std::wstring, AutoMuteCfg_t> settings;
    std::vector<IAudioSessionManager2*> all_sessionManagers;
    std::string cfg_file_name;
    nlohmann::json loaded_json;
    HWND last_fg_Window;
    DWORD last_fg_ProcessID;
    DWORD dw_sleep_time;
    DWORD dw_audio_device_count;
    BOOL is_initialized;
    bool un_mute_all_on_exit;
};
