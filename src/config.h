#pragma once
#include <string>

struct Config {
    std::string webhook_url;
    bool        enable_webhook         = false;
    int         check_interval_minutes = 10;
    std::string dashboard_port         = "8080";
};

Config LoadConfig();
bool   SaveConfig(const Config& config);
