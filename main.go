package main

import (
	_ "embed"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"time"

	"github.com/getlantern/systray"
)

//go:embed gui.py
var guiPythonScript []byte

func main() {
	config, err := LoadConfig()
	if err != nil {
		fmt.Printf("Error loading config: %v\n", err)
	}

	InitLastIP()

	// Start IP checker in background
	go CheckIPChange(&config)

	fmt.Println("🚀 IP Notifier started successfully!")
	fmt.Println("Check your system tray (bottom right corner) for the icon.")

	// Start System Tray
	systray.Run(onReady(&config), onExit)
}

func onReady(config *Config) func() {
	return func() {
		systray.SetTitle("IP Notifier")
		systray.SetTooltip("Monitoring Public IP")

		mCurrentIP := systray.AddMenuItem("IP: "+currentStatus.CurrentIP, "Current IP Address")
		mCurrentIP.Disable()

		systray.AddSeparator()

		mOpenGUI := systray.AddMenuItem("設定與狀態 (Open GUI)", "Open the application interface")
		mCheckNow := systray.AddMenuItem("Check Now (背景檢查)", "Check IP Address manually")

		systray.AddSeparator()

		mQuit := systray.AddMenuItem("Quit", "Exit the application")

		go func() {
			for {
				mCurrentIP.SetTitle("IP: " + currentStatus.CurrentIP)
				time.Sleep(5 * time.Second)
			}
		}()

		for {
			select {
			case <-mOpenGUI.ClickedCh:
				go runPythonGUI()
			case <-mCheckNow.ClickedCh:
				newIP, err := GetPublicIP()
				if err == nil {
					if newIP != currentStatus.CurrentIP {
						UpdateIP(newIP, *config)
					}
					mCurrentIP.SetTitle("IP: " + currentStatus.CurrentIP)
				}
			case <-mQuit.ClickedCh:
				systray.Quit()
				return
			}
		}
	}
}

func runPythonGUI() {
	// Create a temporary file for the Python script
	tmpDir := os.TempDir()
	tmpFile := filepath.Join(tmpDir, "ip_notifier_gui.py")

	// Write the embedded script to the temp file
	err := ioutil.WriteFile(tmpFile, guiPythonScript, 0644)
	if err != nil {
		fmt.Printf("Error writing temp GUI script: %v\n", err)
		return
	}

	// Try to run with 'python' or 'python3'
	cmd := exec.Command("python", tmpFile)
	if err := cmd.Start(); err != nil {
		cmd = exec.Command("python3", tmpFile)
		if err := cmd.Start(); err != nil {
			fmt.Printf("Error starting Python GUI: %v. Please ensure Python is installed.\n", err)
			return
		}
	}
	
	fmt.Println("GUI opened.")
}

func onExit() {
	os.Exit(0)
}

// Stub function to replace NotifyUIUpdate since we use file-based sync now
func NotifyUIUpdate() {}
