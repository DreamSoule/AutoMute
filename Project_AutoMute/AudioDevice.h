#pragma once
#include <windows.h>
#include <audiopolicy.h>
#include <vector>
#include <functional>

namespace AudioDevice {
    // 初始化 COM 库
    bool InitializeCOM();
    void UninitializeCOM();

    // 获取默认音频输出设备
    bool GetDefaultAudioEndpoint(IAudioSessionManager2** ppSessionManager);

    // 获取系统中所有音频输出设备
    bool GetAllAudioEndpoints(std::vector<IAudioSessionManager2*>& sessionManagers);

    // 遍历全部音频输出设备
    void EnumAllAudioEndpoints(std::vector<IAudioSessionManager2*>& sessionManagers, std::function<void(ISimpleAudioVolume*, DWORD)> callback);

}