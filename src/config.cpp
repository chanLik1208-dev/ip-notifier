#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

static const std::string CONFIG_PATH = "config.json";

static Config defaultConfig() {
    Config c;
    c.webhook_url             = "";
    c.enable_webhook          = false;
    c.check_interval_minutes  = 10;
    c.dashboard_port          = "8080";
    return c;
}

Config LoadConfig() {
    std::ifstream f(CONFIG_PATH);
    if (!f.is_open()) {
        Config c = defaultConfig();
        SaveConfig(c);
        return c;
    }
    try {
        json j;
        f >> j;
        Config c;
        c.webhook_url            = j.value("webhook_url",            "");
        c.enable_webhook         = j.value("enable_webhook",         false);
        c.check_interval_minutes = j.value("check_interval_minutes", 10);
        c.dashboard_port         = j.value("dashboard_port",         "8080");
        return c;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << "\n";
        return defaultConfig();
    }
}

bool SaveConfig(const Config& config) {
    try {
        json j;
        j["webhook_url"]            = config.webhook_url;
        j["enable_webhook"]         = config.enable_webhook;
        j["check_interval_minutes"] = config.check_interval_minutes;
        j["dashboard_port"]         = config.dashboard_port;
        std::ofstream f(CONFIG_PATH);
        f << j.dump(2);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << "\n";
        return false;
    }
}
