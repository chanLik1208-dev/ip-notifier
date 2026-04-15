# IP Notifier

A cross-platform Go application that monitors your public IP address and notifies you via Discord Webhook when it changes. Features a system tray icon and a native GUI for configuration.

## Features

- **System Tray**: Displays current IP and provides quick access to settings. (Go)
- **Background Monitoring**: Automatically checks IP periodically. (Go)
- **Discord Notification**: Optional Discord message when IP changes. (Go)
- **Native GUI**: Native settings window using Tkinter. (Python)
- **Single Executable**: The Go binary embeds the Python GUI for easy distribution.

## Requirements

- **Go 1.21+** (for building)
- **Python 3.x** (with `tkinter`, usually pre-installed)

## How to Run

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/chanLik1208-dev/ip-notifier.git
    cd ip-notifier
    ```
2.  **Download dependencies**:
    ```bash
    go mod tidy
    ```
3.  **Run**:
    ```bash
    go run .
    ```

## Development & Build

To build the project into a single executable for Windows:
```bash
go build -ldflags "-H windowsgui" -o ip-notifier.exe .
```
*(The `-H windowsgui` flag prevents a command prompt window from appearing when you run the .exe)*

## GitHub Actions

This repository is configured with GitHub Actions to automatically build and package the application into a Release. Just push your code and check the "Actions" tab!
