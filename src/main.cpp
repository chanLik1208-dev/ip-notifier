#include "config.h"
#include "checker.h"
#include "tray.h"
#include "gui.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

// ---------------------------------------------------------------------------
// Cross-platform entry point
// ---------------------------------------------------------------------------
#ifdef _WIN32
#  include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main() {
#endif

    // 1. Load config and restore last known IP
    Config config = LoadConfig();
    InitLastIP();

    // 2. Background IP checker
    std::atomic<bool> running{true};
    std::thread checker([&running]() {
        CheckIPLoop(&running);
    });
    checker.detach();

    // 3. Tray callbacks
    TrayCallbacks cb;

    cb.on_open_settings = [&config]() {
        ShowSettingsWindow(&config, [&config]() {
            // Reload config after user saves so checker picks up new interval
            config = LoadConfig();
        });
    };

    cb.on_check_now = [&config]() {
        // Run on a detached thread – tray callbacks must return quickly
        std::thread([&config]() {
            std::string ip = GetPublicIP();
            if (ip.empty()) {
                TrayShowNotification("IP Notifier", "Could not reach ipify.org");
                return;
            }
            std::string cur;
            { std::lock_guard<std::mutex> lk(g_status_mutex); cur = g_status.current_ip; }
            if (ip != cur) {
                UpdateIP(ip, config);
                TraySetIP(ip);
                TrayShowNotification("IP Notifier", "IP changed to: " + ip);
            } else {
                TrayShowNotification("IP Notifier", "IP unchanged: " + ip);
            }
        }).detach();
    };

    cb.on_quit = [&running]() {
        running = false;
        TrayQuit();
    };

    // 4. Initialise tray
    TrayInit(cb);

    // 5. Update the tray IP label every 5 s
    std::thread ip_refresher([&running]() {
        while (running.load()) {
            std::string ip;
            { std::lock_guard<std::mutex> lk(g_status_mutex); ip = g_status.current_ip; }
            TraySetIP(ip);
            for (int i = 0; i < 5 && running.load(); ++i)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    ip_refresher.detach();

    // 6. Startup notification
    {
        std::lock_guard<std::mutex> lk(g_status_mutex);
        TrayShowNotification("IP Notifier Started",
                             "Current IP: " + g_status.current_ip +
                             "\nMonitoring in the background.");
    }

    // 7. Run event loop (blocks until quit)
    TrayRun();

    running = false;
    return 0;
}
