#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#include "gui.h"
#include "config.h"
#include "checker.h"
#include <mutex>
#include <functional>

// ---------------------------------------------------------------------------
static Config*                g_cfg        = nullptr;
static std::function<void()>  g_on_changed;
static NSWindow*              g_win        = nil;
static NSTextField*           g_ip_field   = nil;
static NSTextField*           g_wh_field   = nil;
static NSTextField*           g_in_field   = nil;
static NSButton*              g_en_check   = nil;

// ---------------------------------------------------------------------------
@interface SettingsController : NSObject
- (void)save:(id)sender;
- (void)refresh:(id)sender;
@end

@implementation SettingsController
- (void)save:(id)sender {
    if (!g_cfg) return;
    g_cfg->webhook_url            = g_wh_field.stringValue.UTF8String;
    g_cfg->check_interval_minutes = g_in_field.intValue;
    if (g_cfg->check_interval_minutes <= 0) g_cfg->check_interval_minutes = 1;
    g_cfg->enable_webhook = (g_en_check.state == NSControlStateValueOn);
    SaveConfig(*g_cfg);
    if (g_on_changed) g_on_changed();
    NSAlert* a = [[NSAlert alloc] init];
    a.messageText = @"Settings saved!";
    [a runModal];
}
- (void)refresh:(id)sender {
    std::string ip;
    { std::lock_guard<std::mutex> lk(g_status_mutex); ip = g_status.current_ip; }
    g_ip_field.stringValue = [NSString stringWithUTF8String:ip.c_str()];
}
@end

// ---------------------------------------------------------------------------
static NSTextField* makeLabel(NSString* text, NSRect frame, CGFloat size, BOOL bold) {
    NSTextField* f = [[NSTextField alloc] initWithFrame:frame];
    f.stringValue    = text;
    f.editable       = NO;
    f.bezeled        = NO;
    f.drawsBackground = NO;
    f.font = bold ? [NSFont boldSystemFontOfSize:size] : [NSFont systemFontOfSize:size];
    return f;
}
static NSTextField* makeInput(NSString* text, NSRect frame) {
    NSTextField* f = [[NSTextField alloc] initWithFrame:frame];
    f.stringValue = text;
    f.font = [NSFont systemFontOfSize:13];
    return f;
}

// ---------------------------------------------------------------------------
void ShowSettingsWindow(Config* config, std::function<void()> on_config_changed) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_win && [g_win isVisible]) { [g_win makeKeyAndOrderFront:nil]; return; }

        g_cfg        = config;
        g_on_changed = on_config_changed;

        SettingsController* ctrl = [[SettingsController alloc] init];

        NSRect frame = NSMakeRect(0, 0, 480, 440);
        g_win = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
            backing:NSBackingStoreBuffered
            defer:NO];
        g_win.title = @"IP Notifier — Settings";
        [g_win center];

        NSView* cv = g_win.contentView;
        const CGFloat W = 440, LX = 20;
        CGFloat y = 360;

        // Title
        [cv addSubview:makeLabel(@"IP Address Notifier",
            NSMakeRect(LX, y, W, 30), 18, YES)];
        y -= 44;

        // IP
        std::string cur;
        { std::lock_guard<std::mutex> lk(g_status_mutex); cur = g_status.current_ip; }
        g_ip_field = makeLabel(
            [NSString stringWithUTF8String:cur.c_str()],
            NSMakeRect(LX, y, W, 36), 24, YES);
        [cv addSubview:g_ip_field];
        y -= 48;

        // Webhook
        [cv addSubview:makeLabel(@"Discord Webhook URL:", NSMakeRect(LX, y, W, 20), 13, NO)];
        y -= 26;
        g_wh_field = makeInput([NSString stringWithUTF8String:config->webhook_url.c_str()],
                               NSMakeRect(LX, y, W, 24));
        [cv addSubview:g_wh_field];
        y -= 40;

        // Interval
        [cv addSubview:makeLabel(@"Check interval (minutes):", NSMakeRect(LX, y, W, 20), 13, NO)];
        y -= 26;
        g_in_field = makeInput([NSString stringWithFormat:@"%d", config->check_interval_minutes],
                               NSMakeRect(LX, y, 100, 24));
        [cv addSubview:g_in_field];
        y -= 40;

        // Enable checkbox
        g_en_check = [[NSButton alloc] initWithFrame:NSMakeRect(LX, y, W, 24)];
        [g_en_check setButtonType:NSButtonTypeSwitch];
        g_en_check.title = @"Enable Discord Webhook notifications";
        g_en_check.state = config->enable_webhook ? NSControlStateValueOn : NSControlStateValueOff;
        [cv addSubview:g_en_check];
        y -= 48;

        // Buttons
        NSButton* bRefresh = [[NSButton alloc] initWithFrame:NSMakeRect(LX, y, 110, 28)];
        bRefresh.title  = @"Refresh IP";
        bRefresh.target = ctrl;
        bRefresh.action = @selector(refresh:);
        [cv addSubview:bRefresh];

        NSButton* bSave = [[NSButton alloc] initWithFrame:NSMakeRect(340, y, 120, 28)];
        bSave.title  = @"Save Settings";
        bSave.target = ctrl;
        bSave.action = @selector(save:);
        [bSave setKeyEquivalent:@"\r"];
        [cv addSubview:bSave];

        [g_win makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    });
}

#endif // __APPLE__
