import tkinter as tk
from tkinter import ttk, messagebox
import json
import os
import sys
import platform

CONFIG_FILE = "config.json"
LAST_IP_FILE = "last_ip.txt"

# --- Color Palette ---
BG = "#0f172a"
CARD_BG = "#1e293b"
ACCENT = "#6366f1"
ACCENT_HOVER = "#4f46e5"
TEXT = "#f8fafc"
TEXT_MUTED = "#94a3b8"
SUCCESS = "#22c55e"
DANGER = "#ef4444"
BORDER = "#334155"

class App:
    def __init__(self, root):
        self.root = root
        self.root.title("IP Notifier")
        self.root.geometry("480x500")
        self.root.resizable(False, False)
        self.root.configure(bg=BG)

        # Hide to taskbar instead of closing
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

        # Load data
        self.config = self.load_config()
        self.current_ip = self.load_last_ip()

        self.setup_styles()
        self.setup_ui()
        
        # Smooth fade-in effect
        self.root.attributes("-alpha", 0)
        self.fade_in()

    def fade_in(self, alpha=0):
        alpha += 0.05
        self.root.attributes("-alpha", alpha)
        if alpha < 1.0:
            self.root.after(15, lambda: self.fade_in(alpha))

    def on_close(self):
        """Hide to tray instead of destroying."""
        self.root.withdraw()

    def setup_styles(self):
        style = ttk.Style()
        style.theme_use("default")
        style.configure("TFrame", background=BG)
        style.configure("Card.TFrame", background=CARD_BG, relief="flat")
        style.configure("TLabel", background=BG, foreground=TEXT, font=("Segoe UI", 10))
        style.configure("Title.TLabel", background=BG, foreground=TEXT, font=("Segoe UI", 18, "bold"))
        style.configure("Muted.TLabel", background=CARD_BG, foreground=TEXT_MUTED, font=("Segoe UI", 8))
        style.configure("IP.TLabel", background=CARD_BG, foreground=ACCENT, font=("Segoe UI", 28, "bold"))
        style.configure("Primary.TButton", background=ACCENT, foreground=TEXT, font=("Segoe UI", 10, "bold"))
        style.configure("Secondary.TButton", background=CARD_BG, foreground=TEXT, font=("Segoe UI", 10))
        style.map("Primary.TButton", background=[("active", ACCENT_HOVER)])
        style.map("Secondary.TButton", background=[("active", BORDER)])

    def setup_ui(self):
        main = ttk.Frame(self.root, padding=24, style="TFrame")
        main.pack(fill=tk.BOTH, expand=True)

        # --- Header ---
        header = ttk.Frame(main, style="TFrame")
        header.pack(fill=tk.X, pady=(0, 16))

        ttk.Label(header, text="🌐 IP Notifier", style="Title.TLabel").pack(side=tk.LEFT)

        # --- IP Status Card ---
        card = ttk.Frame(main, style="Card.TFrame", padding=20)
        card.pack(fill=tk.X, pady=(0, 20))
        card.configure(relief="solid")  # border

        ttk.Label(card, text="CURRENT PUBLIC IP", style="Muted.TLabel").pack()
        self.ip_label = ttk.Label(card, text=self.current_ip, style="IP.TLabel")
        self.ip_label.pack(pady=(4, 2))

        self.status_dot = tk.Label(card, text="● LIVE MONITORING", bg=CARD_BG, fg=SUCCESS,
                                   font=("Segoe UI", 8, "bold"))
        self.status_dot.pack()

        # --- Settings Form ---
        form = ttk.Frame(main, style="TFrame")
        form.pack(fill=tk.X)

        self._make_label(form, "Discord Webhook URL")
        self.webhook_var = tk.StringVar(value=self.config.get("webhook_url", ""))
        self._make_entry(form).config(textvariable=self.webhook_var)

        self._make_label(form, "Check Interval (minutes)")
        self.interval_var = tk.StringVar(value=str(self.config.get("check_interval_minutes", 10)))
        self._make_entry(form).config(textvariable=self.interval_var)

        # Toggle switch (Checkbox styled)
        toggle_frame = ttk.Frame(form, style="TFrame")
        toggle_frame.pack(fill=tk.X, pady=(10, 0))

        self.enable_var = tk.BooleanVar(value=self.config.get("enable_webhook", False))
        self.toggle_btn = tk.Checkbutton(
            toggle_frame,
            text="  Enable Discord Notifications",
            variable=self.enable_var,
            bg=BG, fg=TEXT,
            selectcolor=ACCENT,
            activebackground=BG,
            activeforeground=TEXT,
            font=("Segoe UI", 10),
            cursor="hand2",
            highlightthickness=0,
            relief="flat"
        )
        self.toggle_btn.pack(anchor=tk.W)

        # --- Action Buttons ---
        btn_frame = ttk.Frame(main, style="TFrame", padding=(0, 20, 0, 0))
        btn_frame.pack(fill=tk.X)

        self._make_button(btn_frame, "🔄 Refresh IP", self.refresh_data, style="Secondary.TButton").pack(side=tk.LEFT, padx=(0, 8))
        self._make_button(btn_frame, "💾 Save Settings", self.save_settings, style="Primary.TButton").pack(side=tk.RIGHT)

    def _make_label(self, parent, text):
        lbl = ttk.Label(parent, text=text, style="TLabel")
        lbl.pack(anchor=tk.W, pady=(10, 2))
        return lbl

    def _make_entry(self, parent):
        entry = tk.Entry(
            parent,
            bg=CARD_BG, fg=TEXT,
            insertbackground=TEXT,
            relief="flat",
            font=("Segoe UI", 10),
            highlightthickness=1,
            highlightcolor=ACCENT,
            highlightbackground=BORDER
        )
        entry.pack(fill=tk.X, ipady=8)
        return entry

    def _make_button(self, parent, text, command, style="Primary.TButton"):
        btn = tk.Button(
            parent,
            text=text,
            command=command,
            bg=ACCENT if "Primary" in style else CARD_BG,
            fg=TEXT,
            activebackground=ACCENT_HOVER if "Primary" in style else BORDER,
            activeforeground=TEXT,
            font=("Segoe UI", 10, "bold" if "Primary" in style else "normal"),
            relief="flat",
            cursor="hand2",
            padx=16, pady=8
        )
        return btn

    def load_config(self):
        if os.path.exists(CONFIG_FILE):
            try:
                with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except Exception:
                pass
        return {}

    def load_last_ip(self):
        if os.path.exists(LAST_IP_FILE):
            try:
                with open(LAST_IP_FILE, 'r', encoding='utf-8') as f:
                    return f.read().strip() or "Unknown"
            except Exception:
                pass
        return "Unknown"

    def refresh_data(self):
        self.current_ip = self.load_last_ip()
        self.ip_label.config(text=self.current_ip)
        self._flash_label()

    def _flash_label(self):
        """Briefly change color to indicate a refresh."""
        self.ip_label.config(foreground=SUCCESS)
        self.root.after(600, lambda: self.ip_label.config(foreground=ACCENT))

    def save_settings(self):
        self.config["webhook_url"] = self.webhook_var.get()
        self.config["enable_webhook"] = self.enable_var.get()
        try:
            self.config["check_interval_minutes"] = int(self.interval_var.get())
        except ValueError:
            messagebox.showerror("Error", "Check interval must be a number!")
            return

        if "dashboard_port" not in self.config:
            self.config["dashboard_port"] = "8080"

        try:
            with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
                json.dump(self.config, f, indent=4)
            self._flash_save_success()
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save: {e}")

    def _flash_save_success(self):
        """Show a non-blocking success toast inside the window."""
        toast = tk.Label(self.root, text="✓  Saved!", bg=SUCCESS, fg="#fff",
                         font=("Segoe UI", 10, "bold"), padx=12, pady=6)
        toast.place(relx=0.5, rely=0.95, anchor="center")
        self.root.after(1800, toast.destroy)


if __name__ == "__main__":
    # Resolve working directory to where the exe lives so config files are found
    if getattr(sys, 'frozen', False):
        os.chdir(os.path.dirname(sys.executable))
    elif __file__:
        # When launched from a temp dir by Go, find config next to the exe
        exe_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
        # If the config file exists relative to exe, cd there
        if os.path.exists(os.path.join(exe_dir, "config.json")):
            os.chdir(exe_dir)

    root = tk.Tk()
    app = App(root)
    root.mainloop()
