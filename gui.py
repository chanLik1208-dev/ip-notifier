import tkinter as tk
from tkinter import ttk, messagebox
import json
import os
import sys
import subprocess

CONFIG_FILE = "config.json"
LAST_IP_FILE = "last_ip.txt"

class App:
    def __init__(self, root):
        self.root = root
        self.root.title("IP Notifier Settings")
        self.root.geometry("450x450")
        self.root.resizable(False, False)
        
        self.style = ttk.Style()
        self.style.configure("TLabel", font=("Segoe UI", 10))
        self.style.configure("TButton", font=("Segoe UI", 10, "bold"))
        
        # Load Data
        self.config = self.load_config()
        self.current_ip = self.load_last_ip()
        
        self.setup_ui()

    def setup_ui(self):
        container = ttk.Frame(self.root, padding="20")
        container.pack(fill=tk.BOTH, expand=True)
        
        # Header
        ttk.Label(container, text="Network Monitor Status", font=("Segoe UI", 14, "bold")).pack(pady=(0, 20))
        
        # IP Display
        ip_frame = tk.Frame(container, bg="#f3f4f6", pady=20, padx=10, highlightthickness=1, highlightbackground="#e5e7eb")
        ip_frame.pack(fill=tk.X, pady=(0, 20))
        
        tk.Label(ip_frame, text="CURRENT PUBLIC IP", font=("Segoe UI", 8), bg="#f3f4f6", fg="#6b7280").pack()
        self.ip_label = tk.Label(ip_frame, text=self.current_ip, font=("Segoe UI", 24, "bold"), bg="#f3f4f6", fg="#2563eb")
        self.ip_label.pack()
        
        # Webhook URL
        ttk.Label(container, text="Discord Webhook URL").pack(anchor=tk.W)
        self.webhook_var = tk.StringVar(value=self.config.get("webhook_url", ""))
        self.webhook_entry = ttk.Entry(container, textvariable=self.webhook_var, width=50)
        self.webhook_entry.pack(fill=tk.X, pady=(0, 15))
        
        # Enable Checkbox
        self.enable_var = tk.BooleanVar(value=self.config.get("enable_webhook", False))
        ttk.Checkbutton(container, text="Enable Discord Notifications", variable=self.enable_var).pack(anchor=tk.W, pady=(0, 20))
        
        # Buttons
        btn_frame = ttk.Frame(container)
        btn_frame.pack(fill=tk.X, pady=(10, 0))
        
        ttk.Button(btn_frame, text="Save Settings", command=self.save_settings).pack(side=tk.RIGHT, padx=5)
        ttk.Button(btn_frame, text="Refresh Status", command=self.refresh_data).pack(side=tk.RIGHT, padx=5)

    def load_config(self):
        if os.path.exists(CONFIG_FILE):
            try:
                with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except: pass
        return {}

    def load_last_ip(self):
        if os.path.exists(LAST_IP_FILE):
            try:
                with open(LAST_IP_FILE, 'r', encoding='utf-8') as f:
                    return f.read().strip()
            except: pass
        return "Unknown"

    def refresh_data(self):
        self.current_ip = self.load_last_ip()
        self.ip_label.config(text=self.current_ip)

    def save_settings(self):
        self.config["webhook_url"] = self.webhook_var.get()
        self.config["enable_webhook"] = self.enable_var.get()
        
        # Preserve other fields like check_interval_minutes if they exist
        if "check_interval_minutes" not in self.config:
            self.config["check_interval_minutes"] = 10
        if "dashboard_port" not in self.config:
            self.config["dashboard_port"] = "8080"
            
        try:
            with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
                json.dump(self.config, f, indent=4)
            messagebox.showinfo("Success", "Settings saved successfully!")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save: {e}")

if __name__ == "__main__":
    root = tk.Tk()
    app = App(root)
    root.mainloop()
