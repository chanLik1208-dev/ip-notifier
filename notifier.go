package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net/http"
)

type DiscordPayload struct {
	Content string `json:"content"`
	Embeds  []Embed `json:"embeds"`
}

type Embed struct {
	Title       string `json:"title"`
	Description string `json:"description"`
	Color       int    `json:"color"`
}

func SendDiscordNotification(webhookURL, oldIP, newIP string) {
	if webhookURL == "" {
		return
	}

	payload := DiscordPayload{
		Embeds: []Embed{
			{
				Title:       "IP Address Changed!",
				Description: fmt.Sprintf("Your IP address has changed.\n\n**Old IP:** %s\n**New IP:** %s", oldIP, newIP),
				Color:       0x00ff00, // Green
			},
		},
	}

	data, err := json.Marshal(payload)
	if err != nil {
		fmt.Printf("Error marshaling Discord payload: %v\n", err)
		return
	}

	resp, err := http.Post(webhookURL, "application/json", bytes.NewBuffer(data))
	if err != nil {
		fmt.Printf("Error sending Discord notification: %v\n", err)
		return
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusNoContent && resp.StatusCode != http.StatusOK {
		fmt.Printf("Discord returned non-OK status: %d\n", resp.StatusCode)
	}
}
