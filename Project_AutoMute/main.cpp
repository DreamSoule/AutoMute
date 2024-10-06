#include "AutoMute.h"
#include <iostream>

AutoMute auto_mute("config.json");

void my_exit() {
    if (auto_mute.is_un_mute_all_on_exit()) {
        std::cout << "UnMute_ALL" << '\n';
        auto_mute.UnMute_ALL();
    }
    auto_mute.SaveSettings();
}

BOOL CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        my_exit();
        return TRUE;
    default:
        return FALSE;
    }
}

int main(int argc, char* argv[]) {
    system("chcp 65001"); /* 设置控制台的编码为 UTF-8 */

    auto_mute.LoadSettings();

    // std::cout << "local PID: " << GetCurrentProcessId() << '\n';
    DWORD sleep_time = max(1, auto_mute.sleep_time());
    std::cout << "sleep_time: " << sleep_time << '\n';
    std::cout << "audio_device_count: " << auto_mute.audio_device_count() << std::endl;

    // 注册退出时保存配置的回调函数
    std::atexit(my_exit);
    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        std::cerr << "Error installing handler." << std::endl;
        // return 1;
    }

    while (!GetAsyncKeyState(VK_END)) {
        auto_mute.Tick();
        Sleep(sleep_time);
    }
    return 0;
}
