package main

import (
	"encoding/json"
	"io/ioutil"
	"os"
)

type Config struct {
	WebhookURL            string `json:"webhook_url"`
	EnableWebhook         bool   `json:"enable_webhook"`
	CheckIntervalMinutes  int    `json:"check_interval_minutes"`
	DashboardPort         string `json:"dashboard_port"`
}

var defaultConfig = Config{
	WebhookURL:           "",
	EnableWebhook:        false,
	CheckIntervalMinutes: 10,
	DashboardPort:        "8080",
}

func LoadConfig() (Config, error) {
	configPath := "config.json"
	if _, err := os.Stat(configPath); os.IsNotExist(err) {
		SaveConfig(defaultConfig)
		return defaultConfig, nil
	}

	data, err := ioutil.ReadFile(configPath)
	if err != nil {
		return defaultConfig, err
	}

	var config Config
	if err := json.Unmarshal(data, &config); err != nil {
		return defaultConfig, err
	}

	return config, nil
}

func SaveConfig(config Config) error {
	data, err := json.MarshalIndent(config, "", "  ")
	if err != nil {
		return err
	}
	return ioutil.WriteFile("config.json", data, 0644)
}
