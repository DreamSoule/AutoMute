#include "AudioDevice.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>

namespace AudioDevice {
    // 初始化 COM 库
    bool InitializeCOM() {
        return SUCCEEDED(CoInitialize(nullptr));
    }

    void UninitializeCOM() {
        return CoUninitialize();
    }

    // 获取默认音频输出设备
    bool GetDefaultAudioEndpoint(IAudioSessionManager2** ppSessionManager) {
        IMMDeviceEnumerator* pEnumerator = nullptr;
        IMMDevice* pDevice = nullptr;

        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator))) {
            return false;
        }
        if (FAILED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice))) {
            pEnumerator->Release();
            return false;
        }
        pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)ppSessionManager);

        pEnumerator->Release();
        pDevice->Release();
        return true;
    }

    // 获取系统中所有音频输出设备
    bool GetAllAudioEndpoints(std::vector<IAudioSessionManager2*>& sessionManagers) {
        IMMDeviceEnumerator* pEnumerator = nullptr;
        IMMDeviceCollection* pCollection = nullptr;

        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator))) {
            return false;
        }
        if (FAILED(pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection))) {
            pEnumerator->Release();
            return false;
        }

        UINT count;
        pCollection->GetCount(&count);

        for (UINT i = 0; i < count; ++i) {
            IMMDevice* pDevice = nullptr;
            if (SUCCEEDED(pCollection->Item(i, &pDevice))) {
                IAudioSessionManager2* pSessionManager = nullptr;
                if (SUCCEEDED(pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager))) {
                    sessionManagers.push_back(pSessionManager);
                }
                pDevice->Release();
            }
        }

        pCollection->Release();
        pEnumerator->Release();
        return true;
    }

    // 遍历全部音频输出设备
    void EnumAllAudioEndpoints(std::vector<IAudioSessionManager2*>& sessionManagers, std::function<void(ISimpleAudioVolume*, DWORD)> callback) {
        IAudioSessionEnumerator* pSessionEnumerator = nullptr;
        IAudioSessionControl* pSessionControl = nullptr;
        for (const auto& pSessionManager : sessionManagers) {
            pSessionManager->GetSessionEnumerator(&pSessionEnumerator);

            int sessionCount = 0;
            pSessionEnumerator->GetCount(&sessionCount);

            for (int i = 0; i < sessionCount; ++i) {
                pSessionEnumerator->GetSession(i, &pSessionControl);

                IAudioSessionControl2* pSessionControl2 = nullptr;
                if (FAILED(pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2))) {
                    pSessionControl->Release();
                    continue;
                }

                DWORD sessionProcessId = 0;
                if (FAILED(pSessionControl2->GetProcessId(&sessionProcessId)) || sessionProcessId <= 4) {
                    pSessionControl2->Release();
                    pSessionControl->Release();
                    continue;
                }

                ISimpleAudioVolume* pAudioVolume = nullptr;
                pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
                callback(pAudioVolume, sessionProcessId);
                pAudioVolume->Release();

                pSessionControl2->Release();
                pSessionControl->Release();
            }

            pSessionEnumerator->Release();
            //pSessionManager->Release();
        }
    }

}