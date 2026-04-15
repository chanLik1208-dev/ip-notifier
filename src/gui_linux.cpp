#if defined(__linux__)
#include "gui.h"
#include "config.h"
#include "checker.h"

#include <gtk/gtk.h>
#include <string>
#include <functional>
#include <mutex>

// ---------------------------------------------------------------------------
static Config*                g_cfg        = nullptr;
static std::function<void()>  g_on_changed;
static GtkWidget*             g_win        = nullptr;
static GtkWidget*             g_ip_label   = nullptr;
static GtkWidget*             g_wh_entry   = nullptr;
static GtkWidget*             g_in_entry   = nullptr;
static GtkWidget*             g_en_check   = nullptr;

// ---------------------------------------------------------------------------
static void on_save(GtkButton*, gpointer) {
    if (!g_cfg) return;
    g_cfg->webhook_url            = gtk_entry_get_text(GTK_ENTRY(g_wh_entry));
    const char* iv = gtk_entry_get_text(GTK_ENTRY(g_in_entry));
    g_cfg->check_interval_minutes = iv ? std::stoi(std::string(iv)) : 10;
    if (g_cfg->check_interval_minutes <= 0) g_cfg->check_interval_minutes = 1;
    g_cfg->enable_webhook = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_en_check));
    SaveConfig(*g_cfg);
    if (g_on_changed) g_on_changed();

    GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(g_win),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Settings saved!");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void on_refresh(GtkButton*, gpointer) {
    std::string ip;
    { std::lock_guard<std::mutex> lk(g_status_mutex); ip = g_status.current_ip; }
    gtk_label_set_text(GTK_LABEL(g_ip_label), ip.c_str());
}

static void on_win_destroy(GtkWidget*, gpointer) { g_win = nullptr; }

// ---------------------------------------------------------------------------
// Dispatch helper – run on GTK main thread
// ---------------------------------------------------------------------------
struct ShowData { Config* cfg; std::function<void()> cb; };

static gboolean do_show_settings(gpointer raw) {
    auto* d = static_cast<ShowData*>(raw);
    if (g_win) { gtk_window_present(GTK_WINDOW(g_win)); delete d; return G_SOURCE_REMOVE; }

    g_cfg        = d->cfg;
    g_on_changed = d->cb;
    delete d;

    g_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_win), "IP Notifier — Settings");
    gtk_window_set_default_size(GTK_WINDOW(g_win), 480, 420);
    gtk_window_set_resizable(GTK_WINDOW(g_win), FALSE);
    g_signal_connect(g_win, "destroy", G_CALLBACK(on_win_destroy), nullptr);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_container_add(GTK_CONTAINER(g_win), vbox);

    // Title
    GtkWidget* title = gtk_label_new("IP Address Notifier");
    PangoAttrList* al = pango_attr_list_new();
    pango_attr_list_insert(al, pango_attr_size_new_absolute(18 * PANGO_SCALE));
    pango_attr_list_insert(al, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(title), al);
    pango_attr_list_unref(al);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);

    // IP
    std::string cur;
    { std::lock_guard<std::mutex> lk(g_status_mutex); cur = g_status.current_ip; }
    g_ip_label = gtk_label_new(cur.c_str());
    PangoAttrList* al2 = pango_attr_list_new();
    pango_attr_list_insert(al2, pango_attr_size_new_absolute(22 * PANGO_SCALE));
    pango_attr_list_insert(al2, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(g_ip_label), al2);
    pango_attr_list_unref(al2);
    gtk_box_pack_start(GTK_BOX(vbox), g_ip_label, FALSE, FALSE, 0);

    // Webhook
    gtk_box_pack_start(GTK_BOX(vbox),
        gtk_label_new("Discord Webhook URL:"), FALSE, FALSE, 0);
    g_wh_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(g_wh_entry), g_cfg->webhook_url.c_str());
    gtk_box_pack_start(GTK_BOX(vbox), g_wh_entry, FALSE, FALSE, 0);

    // Interval
    gtk_box_pack_start(GTK_BOX(vbox),
        gtk_label_new("Check interval (minutes):"), FALSE, FALSE, 0);
    g_in_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(g_in_entry),
        std::to_string(g_cfg->check_interval_minutes).c_str());
    gtk_box_pack_start(GTK_BOX(vbox), g_in_entry, FALSE, FALSE, 0);

    // Enable checkbox
    g_en_check = gtk_check_button_new_with_label(
        "Enable Discord Webhook notifications");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_en_check),
        g_cfg->enable_webhook);
    gtk_box_pack_start(GTK_BOX(vbox), g_en_check, FALSE, FALSE, 0);

    // Buttons row
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* bRef = gtk_button_new_with_label("Refresh IP");
    g_signal_connect(bRef, "clicked", G_CALLBACK(on_refresh), nullptr);
    gtk_box_pack_start(GTK_BOX(hbox), bRef, FALSE, FALSE, 0);

    GtkWidget* bSave = gtk_button_new_with_label("Save Settings");
    g_signal_connect(bSave, "clicked", G_CALLBACK(on_save), nullptr);
    gtk_box_pack_end(GTK_BOX(hbox), bSave, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 8);

    gtk_widget_show_all(g_win);
    return G_SOURCE_REMOVE;
}

void ShowSettingsWindow(Config* config, std::function<void()> on_config_changed) {
    g_idle_add(do_show_settings, new ShowData{config, on_config_changed});
}

#endif // __linux__
