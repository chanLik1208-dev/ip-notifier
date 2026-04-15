#pragma once
#include <string>
#include <functional>

struct TrayCallbacks {
    std::function<void()> on_open_settings;
    std::function<void()> on_check_now;
    std::function<void()> on_quit;
};

void TrayInit(const TrayCallbacks& callbacks);
void TraySetIP(const std::string& ip);
void TrayShowNotification(const std::string& title, const std::string& message);
void TrayRun();   // blocks until quit
void TrayQuit();
