#include "notifier.h"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#else
#include <httplib.h>
#endif
#include <nlohmann/json.hpp>

#include <iostream>

using json = nlohmann::json;

// Extract host and path from a full Discord webhook URL
// e.g. https://discord.com/api/webhooks/123/TOKEN
static bool parseURL(const std::string& url, std::string& host, std::string& path) {
    const std::string https_prefix = "https://";
    if (url.rfind(https_prefix, 0) != 0) return false;
    std::string rest = url.substr(https_prefix.size());
    auto slash = rest.find('/');
    if (slash == std::string::npos) return false;
    host = rest.substr(0, slash);
    path = rest.substr(slash);
    return true;
}

void SendDiscordNotification(const std::string& webhook_url,
                             const std::string& old_ip,
                             const std::string& new_ip) {
    if (webhook_url.empty()) return;

    std::string host, path;
    if (!parseURL(webhook_url, host, path)) {
        std::cerr << "Invalid Discord webhook URL\n";
        return;
    }

    try {
        json embed;
        embed["title"]       = "IP Address Changed!";
        embed["description"] = "Your public IP address has changed.\n\n"
                               "**Old IP:** " + old_ip +
                               "\n**New IP:** " + new_ip;
        embed["color"]       = 0x00ff00;

        json payload;
        payload["embeds"] = json::array({embed});
        std::string body = payload.dump();

#ifdef _WIN32
        HINTERNET hSession = WinHttpOpen(L"IP Notifier / 1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (hSession) {
            auto s2w = [](const std::string& s) -> std::wstring {
                if (s.empty()) return {};
                int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
                std::wstring w(len, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
                return w;
            };

            HINTERNET hConnect = WinHttpConnect(hSession, s2w(host).c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (hConnect) {
                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", s2w(path).c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                if (hRequest) {
                    std::wstring headers = L"Content-Type: application/json\r\n";
                    if (!WinHttpSendRequest(hRequest, headers.c_str(), -1, (LPVOID)body.data(), body.size(), body.size(), 0)) {
                        std::cerr << "WinHttpSendRequest failed\n";
                    } else {
                        if (WinHttpReceiveResponse(hRequest, NULL)) {
                            DWORD statusCode = 0;
                            DWORD dwSize = sizeof(statusCode);
                            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
                            if (statusCode != 200 && statusCode != 204) {
                                std::cerr << "Discord returned HTTP " << statusCode << "\n";
                            }
                        }
                    }
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
            }
            WinHttpCloseHandle(hSession);
        }
#else
        httplib::SSLClient cli(host);
        cli.set_connection_timeout(10);
        cli.set_read_timeout(10);

        auto res = cli.Post(path, body, "application/json");
        if (!res) {
            std::cerr << "Discord request failed (no response)\n";
        } else if (res->status != 200 && res->status != 204) {
            std::cerr << "Discord returned HTTP " << res->status << "\n";
        }
#endif
    } catch (const std::exception& e) {
        std::cerr << "Discord notification error: " << e.what() << "\n";
    }
}
