package main

import (
	"io/ioutil"
	"net/http"
	"strings"
	"time"
	"fmt"
)

type IPStatus struct {
	CurrentIP   string
	LastUpdated time.Time
	History     []string
}

var currentStatus = IPStatus{
	CurrentIP:   "Unknown",
	LastUpdated: time.Now(),
}

func GetPublicIP() (string, error) {
	resp, err := http.Get("https://api.ipify.org")
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	ip, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}

	return strings.TrimSpace(string(ip)), nil
}

func CheckIPChange(config *Config) {
	for {
		newIP, err := GetPublicIP()
		if err != nil {
			fmt.Printf("Error checking IP: %v\n", err)
		} else {
			if newIP != currentStatus.CurrentIP {
				UpdateIP(newIP, *config)
			}
		}
		time.Sleep(time.Duration(config.CheckIntervalMinutes) * time.Minute)
	}
}

func UpdateIP(newIP string, config Config) {
	fmt.Printf("IP changed from %s to %s\n", currentStatus.CurrentIP, newIP)
	
	oldIP := currentStatus.CurrentIP
	currentStatus.CurrentIP = newIP
	currentStatus.LastUpdated = time.Now()
	
	// Persistent last_ip.txt
	_ = ioutil.WriteFile("last_ip.txt", []byte(newIP), 0644)

	// Notify GUI if it's open
	NotifyUIUpdate()

	if oldIP != "Unknown" && config.EnableWebhook {
		SendDiscordNotification(config.WebhookURL, oldIP, newIP)
	}
}

func InitLastIP() {
	data, err := ioutil.ReadFile("last_ip.txt")
	if err == nil {
		currentStatus.CurrentIP = strings.TrimSpace(string(data))
	}
}
