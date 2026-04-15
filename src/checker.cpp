#include "checker.h"
#include "notifier.h"
#include "config.h"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#else
#include <httplib.h>
#endif

#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

IPStatus   g_status;
std::mutex g_status_mutex;

static const std::string LAST_IP_PATH = "last_ip.txt";

// -----------------------------------------------------------------------
std::string GetPublicIP() {
#ifdef _WIN32
    std::string ip;
    HINTERNET hSession = WinHttpOpen(L"IP Notifier / 1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        HINTERNET hConnect = WinHttpConnect(hSession, L"api.ipify.org", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (hRequest) {
                if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                    if (WinHttpReceiveResponse(hRequest, NULL)) {
                        DWORD size = 0;
                        DWORD downloaded = 0;
                        do {
                            if (WinHttpQueryDataAvailable(hRequest, &size) && size > 0) {
                                std::string buf(size, 0);
                                if (WinHttpReadData(hRequest, (LPVOID)buf.data(), size, &downloaded)) {
                                    ip += buf.substr(0, downloaded);
                                }
                            }
                        } while (size > 0);
                    }
                }
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }
    if (!ip.empty()) {
        ip.erase(ip.find_last_not_of(" \r\n\t") + 1);
        ip.erase(0, ip.find_first_not_of(" \r\n\t"));
    }
    return ip;
#else
    try {
        httplib::SSLClient cli("api.ipify.org");
        cli.set_connection_timeout(10);
        cli.set_read_timeout(10);
        auto res = cli.Get("/");
        if (res && res->status == 200) {
            // trim whitespace
            std::string ip = res->body;
            ip.erase(ip.find_last_not_of(" \r\n\t") + 1);
            ip.erase(0, ip.find_first_not_of(" \r\n\t"));
            return ip;
        }
    } catch (const std::exception& e) {
        std::cerr << "GetPublicIP error: " << e.what() << "\n";
    }
    return "";
#endif
}

// -----------------------------------------------------------------------
void InitLastIP() {
    std::ifstream f(LAST_IP_PATH);
    if (!f.is_open()) return;
    std::string ip;
    std::getline(f, ip);
    if (!ip.empty()) {
        std::lock_guard<std::mutex> lk(g_status_mutex);
        g_status.current_ip = ip;
    }
}

// -----------------------------------------------------------------------
void UpdateIP(const std::string& new_ip, const Config& config) {
    std::string old_ip;
    {
        std::lock_guard<std::mutex> lk(g_status_mutex);
        old_ip              = g_status.current_ip;
        g_status.current_ip = new_ip;
        g_status.last_updated = std::chrono::system_clock::now();
    }

    std::cout << "IP changed: " << old_ip << " -> " << new_ip << "\n";

    // Persist to last_ip.txt
    std::ofstream f(LAST_IP_PATH);
    if (f.is_open()) f << new_ip;

    // Discord notification
    if (old_ip != "Unknown" && config.enable_webhook && !config.webhook_url.empty()) {
        SendDiscordNotification(config.webhook_url, old_ip, new_ip);
    }
}

// -----------------------------------------------------------------------
void CheckIPLoop(std::atomic<bool>* running) {
    while (running->load()) {
        // Reload config each cycle so interval / webhook changes take effect
        Config config = LoadConfig();

        std::string new_ip = GetPublicIP();
        if (!new_ip.empty()) {
            std::string current;
            {
                std::lock_guard<std::mutex> lk(g_status_mutex);
                current = g_status.current_ip;
            }
            if (new_ip != current) {
                UpdateIP(new_ip, config);
            }
        }

        // Sleep in 1-second increments so we can react to quit quickly
        int secs = config.check_interval_minutes * 60;
        for (int i = 0; i < secs && running->load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
