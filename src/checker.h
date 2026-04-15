#pragma once
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include "config.h"

struct IPStatus {
    std::string current_ip   = "Unknown";
    std::chrono::system_clock::time_point last_updated;
};

extern IPStatus    g_status;
extern std::mutex  g_status_mutex;

std::string GetPublicIP();
void        InitLastIP();
void        UpdateIP(const std::string& new_ip, const Config& config);
void        CheckIPLoop(std::atomic<bool>* running);
