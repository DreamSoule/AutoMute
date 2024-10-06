#pragma once
#include <windows.h>
#include <audiopolicy.h>
#include <vector>
#include <functional>

namespace AudioDevice {
    // ��ʼ�� COM ��
    bool InitializeCOM();
    void UninitializeCOM();

    // ��ȡĬ����Ƶ����豸
    bool GetDefaultAudioEndpoint(IAudioSessionManager2** ppSessionManager);

    // ��ȡϵͳ��������Ƶ����豸
    bool GetAllAudioEndpoints(std::vector<IAudioSessionManager2*>& sessionManagers);

    // ����ȫ����Ƶ����豸
    void EnumAllAudioEndpoints(std::vector<IAudioSessionManager2*>& sessionManagers, std::function<void(ISimpleAudioVolume*, DWORD)> callback);

}