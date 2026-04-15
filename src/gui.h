#pragma once
#include "config.h"
#include <functional>

void ShowSettingsWindow(Config* config,
                        std::function<void()> on_config_changed = nullptr);
