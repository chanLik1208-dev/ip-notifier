#include "notifier.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
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

        httplib::SSLClient cli(host);
        cli.set_connection_timeout(10);
        cli.set_read_timeout(10);

        auto res = cli.Post(path, body, "application/json");
        if (!res) {
            std::cerr << "Discord request failed (no response)\n";
        } else if (res->status != 200 && res->status != 204) {
            std::cerr << "Discord returned HTTP " << res->status << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Discord notification error: " << e.what() << "\n";
    }
}
