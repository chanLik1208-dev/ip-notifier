#if defined(__linux__)
#include "tray.h"
#include "checker.h"

#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <libnotify/notify.h>

#include <mutex>
#include <string>
#include <memory>

static TrayCallbacks  g_cbs;
static AppIndicator*  g_indicator   = nullptr;
static GtkWidget*     g_ip_item     = nullptr;

// ---------------------------------------------------------------------------
// GTK signal handlers – must run on GTK main thread
// ---------------------------------------------------------------------------
static void on_settings(GtkMenuItem*, gpointer) {
    if (g_cbs.on_open_settings) g_cbs.on_open_settings();
}
static void on_check_now(GtkMenuItem*, gpointer) {
    if (g_cbs.on_check_now) g_cbs.on_check_now();
}
static void on_quit(GtkMenuItem*, gpointer) {
    if (g_cbs.on_quit) g_cbs.on_quit();
    gtk_main_quit();
}

// ---------------------------------------------------------------------------
// TraySetIP helper – dispatch to GTK main thread
// ---------------------------------------------------------------------------
struct IPUpdateData { std::string ip; };

static gboolean update_ip_label_cb(gpointer data) {
    auto* d = static_cast<IPUpdateData*>(data);
    if (g_ip_item)
        gtk_menu_item_set_label(GTK_MENU_ITEM(g_ip_item),
                                ("IP: " + d->ip).c_str());
    delete d;
    return G_SOURCE_REMOVE;
}

// ---------------------------------------------------------------------------
// TrayShowNotification helper
// ---------------------------------------------------------------------------
struct NotifData { std::string title, message; };

static gboolean show_notif_cb(gpointer data) {
    auto* d = static_cast<NotifData*>(data);
    NotifyNotification* n =
        notify_notification_new(d->title.c_str(), d->message.c_str(),
                                "network-wireless");
    notify_notification_show(n, nullptr);
    g_object_unref(G_OBJECT(n));
    delete d;
    return G_SOURCE_REMOVE;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void TrayInit(const TrayCallbacks& callbacks) {
    g_cbs = callbacks;

    int argc = 0;
    gtk_init(&argc, nullptr);
    notify_init("IP Notifier");

    g_indicator = app_indicator_new(
        "ip-notifier", "network-wireless",
        APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status(g_indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_title(g_indicator, "IP Notifier");

    GtkWidget* menu = gtk_menu_new();

    // IP item (disabled)
    std::string ip;
    { std::lock_guard<std::mutex> lk(g_status_mutex); ip = g_status.current_ip; }
    g_ip_item = gtk_menu_item_new_with_label(("IP: " + ip).c_str());
    gtk_widget_set_sensitive(g_ip_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), g_ip_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* settings_item = gtk_menu_item_new_with_label("Settings");
    g_signal_connect(settings_item, "activate", G_CALLBACK(on_settings), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);

    GtkWidget* check_item = gtk_menu_item_new_with_label("Check Now");
    g_signal_connect(check_item, "activate", G_CALLBACK(on_check_now), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), check_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);

    gtk_widget_show_all(menu);
    app_indicator_set_menu(g_indicator, GTK_MENU(menu));
}

void TraySetIP(const std::string& ip) {
    g_idle_add(update_ip_label_cb, new IPUpdateData{ip});
}

void TrayShowNotification(const std::string& title, const std::string& message) {
    g_idle_add(show_notif_cb, new NotifData{title, message});
}

void TrayRun() {
    gtk_main();
}

void TrayQuit() {
    g_idle_add([](gpointer) -> gboolean { gtk_main_quit(); return G_SOURCE_REMOVE; },
               nullptr);
}

#endif // __linux__
