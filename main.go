package main

import (
	_ "embed"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
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

	// Start System Tray (blocks until quit)
	systray.Run(onReady(&config), onExit)
}

func onReady(config *Config) func() {
	return func() {
		systray.SetTitle("IP Notifier")
		systray.SetTooltip("IP Notifier - Monitoring your public IP")

		mCurrentIP := systray.AddMenuItem("IP: "+currentStatus.CurrentIP, "Current IP Address")
		mCurrentIP.Disable()

		systray.AddSeparator()

		mOpenGUI := systray.AddMenuItem("⚙ 設定與狀態 (Open Settings)", "Open the application interface")
		mCheckNow := systray.AddMenuItem("🔄 立即檢查 (Check Now)", "Check IP Address manually")

		systray.AddSeparator()

		mQuit := systray.AddMenuItem("✖ Quit", "Exit the application")

		// Auto-open GUI on startup, after a small delay so tray is ready
		go func() {
			time.Sleep(500 * time.Millisecond)
			showTrayNotification("IP Notifier 已啟動", "目前 IP: "+currentStatus.CurrentIP+"\n程式正在背景監控中，點擊右下角圖示可開啟設定。")
			runPythonGUI()
		}()

		// Update IP display in tray periodically
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
	// Write the embedded script to a temp file
	tmpDir := os.TempDir()
	tmpFile := filepath.Join(tmpDir, "ip_notifier_gui.py")

	err := ioutil.WriteFile(tmpFile, guiPythonScript, 0644)
	if err != nil {
		fmt.Printf("Error writing temp GUI script: %v\n", err)
		return
	}

	// Find python executable
	pythonCmd := findPython()
	if pythonCmd == "" {
		fmt.Println("Python not found. Please install Python 3.x.")
		showTrayNotification("Error", "找不到 Python，請先安裝 Python 3.x")
		return
	}

	cmd := exec.Command(pythonCmd, tmpFile)
	if err := cmd.Run(); err != nil {
		// cmd.Run() blocks until window is closed - normal behavior
		fmt.Printf("GUI closed: %v\n", err)
	}

	// After window is closed, show tray notification
	showTrayNotification("IP Notifier", "設定已關閉，程式仍在背景監控中。")
}

func findPython() string {
	candidates := []string{"python", "python3"}
	for _, c := range candidates {
		if path, err := exec.LookPath(c); err == nil {
			return path
		}
	}
	return ""
}

// showTrayNotification sends a desktop notification cross-platform
func showTrayNotification(title, message string) {
	switch runtime.GOOS {
	case "windows":
		// Use PowerShell toast notification on Windows
		ps := fmt.Sprintf(`[System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms") | Out-Null; $n = New-Object System.Windows.Forms.NotifyIcon; $n.Icon = [System.Drawing.SystemIcons]::Information; $n.Visible = $true; $n.ShowBalloonTip(3000, '%s', '%s', [System.Windows.Forms.ToolTipIcon]::Info); Start-Sleep -Seconds 4; $n.Dispose()`, title, message)
		cmd := exec.Command("powershell", "-WindowStyle", "Hidden", "-Command", ps)
		cmd.Start()
	case "darwin":
		// macOS notification
		script := fmt.Sprintf(`display notification "%s" with title "%s"`, message, title)
		exec.Command("osascript", "-e", script).Start()
	case "linux":
		// Linux notification via notify-send
		exec.Command("notify-send", title, message).Start()
	}
}

func onExit() {
	os.Exit(0)
}

// Stub for file-based sync notification
func NotifyUIUpdate() {}
