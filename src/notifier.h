#pragma once
#include <string>

void SendDiscordNotification(const std::string& webhook_url,
                             const std::string& old_ip,
                             const std::string& new_ip);
