#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>

#include "tray.h"
#include "checker.h"
#include <mutex>
#include <string>

static TrayCallbacks g_cbs;
static NSStatusItem* g_statusItem = nil;
static NSMenuItem*   g_ipItem     = nil;

// ---------------------------------------------------------------------------
@interface TrayDelegate : NSObject
@end

@implementation TrayDelegate
- (void)openSettings:(id)sender {
    if (g_cbs.on_open_settings) g_cbs.on_open_settings();
}
- (void)checkNow:(id)sender {
    if (g_cbs.on_check_now) g_cbs.on_check_now();
}
- (void)quit:(id)sender {
    if (g_cbs.on_quit) g_cbs.on_quit();
    [NSApp terminate:nil];
}
@end

// ---------------------------------------------------------------------------
void TrayInit(const TrayCallbacks& callbacks) {
    g_cbs = callbacks;

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

    TrayDelegate* delegate = [[TrayDelegate alloc] init];

    g_statusItem = [[NSStatusBar systemStatusBar]
                    statusItemWithLength:NSVariableStatusItemLength];
    g_statusItem.button.title   = @"🌐";
    g_statusItem.button.toolTip = @"IP Notifier";

    NSMenu* menu = [[NSMenu alloc] initWithTitle:@""];

    // IP display (disabled)
    std::string ip;
    { std::lock_guard<std::mutex> lk(g_status_mutex); ip = g_status.current_ip; }
    g_ipItem = [[NSMenuItem alloc]
                initWithTitle:[NSString stringWithFormat:@"IP: %s", ip.c_str()]
                action:nil keyEquivalent:@""];
    g_ipItem.enabled = NO;
    [menu addItem:g_ipItem];
    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* settings = [[NSMenuItem alloc]
        initWithTitle:@"Settings" action:@selector(openSettings:) keyEquivalent:@","];
    settings.target = delegate;
    [menu addItem:settings];

    NSMenuItem* check = [[NSMenuItem alloc]
        initWithTitle:@"Check Now" action:@selector(checkNow:) keyEquivalent:@"r"];
    check.target = delegate;
    [menu addItem:check];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* quit = [[NSMenuItem alloc]
        initWithTitle:@"Quit" action:@selector(quit:) keyEquivalent:@"q"];
    quit.target = delegate;
    [menu addItem:quit];

    g_statusItem.menu = menu;
}

void TraySetIP(const std::string& ip) {
    if (!g_ipItem) return;
    NSString* title = [NSString stringWithFormat:@"IP: %s", ip.c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{ g_ipItem.title = title; });
}

void TrayShowNotification(const std::string& title, const std::string& message) {
    NSString* t = [NSString stringWithUTF8String:title.c_str()];
    NSString* m = [NSString stringWithUTF8String:message.c_str()];

    dispatch_async(dispatch_get_main_queue(), ^{
        if (@available(macOS 10.14, *)) {
            UNUserNotificationCenter* center =
                [UNUserNotificationCenter currentNotificationCenter];
            [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
                                 completionHandler:^(BOOL granted, NSError* _Nullable error) {}];
            UNMutableNotificationContent* content =
                [[UNMutableNotificationContent alloc] init];
            content.title = t;
            content.body  = m;
            UNNotificationRequest* req =
                [UNNotificationRequest requestWithIdentifier:@"ip-change"
                                                     content:content
                                                     trigger:nil];
            [center addNotificationRequest:req withCompletionHandler:nil];
        } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            NSUserNotification* n = [[NSUserNotification alloc] init];
            n.title           = t;
            n.informativeText = m;
            [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:n];
#pragma clang diagnostic pop
        }
    });
}

void TrayRun() {
    [NSApp run];
}

void TrayQuit() {
    [NSApp terminate:nil];
}

#endif // __APPLE__
