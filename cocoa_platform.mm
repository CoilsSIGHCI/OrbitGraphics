#include "platform.h"

#include <CoreFoundation/CoreFoundation.h>
#include <string.h>

#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface WindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) cocoa_app_t* owner;
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(id)sender {
  if (self.owner != NULL) {
    self.owner->running = 0;
  }
  return YES;
}
@end

static CGSize currentDrawableSize(NSView* view) {
  NSRect bounds = view.bounds;
  return [view convertSizeToBacking:bounds.size];
}

bool cocoa_app_init(cocoa_app_t* app, const cocoa_app_desc_t* desc) {
  if (app == NULL || desc == NULL) {
    return false;
  }

  memset(app, 0, sizeof(*app));

  @autoreleasepool {
    NSApplication* application = [NSApplication sharedApplication];
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect frame = NSMakeRect(0, 0, desc->width, desc->height);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                              NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    if (window == nil) {
      return false;
    }

    NSString* title = desc->title ? [NSString stringWithUTF8String:desc->title] : @"bgfx";
    [window setTitle:title];
    [window center];

    WindowDelegate* delegate = [[WindowDelegate alloc] init];
    delegate.owner = app;
    [window setDelegate:delegate];

    NSView* contentView = [window contentView];
    [contentView setWantsLayer:YES];

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
      return false;
    }

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.device = device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
    metalLayer.frame = contentView.bounds;
    metalLayer.contentsScale = window.backingScaleFactor;
    metalLayer.drawableSize = currentDrawableSize(contentView);
    [contentView setLayer:metalLayer];

    [window makeKeyAndOrderFront:nil];
    [application activateIgnoringOtherApps:YES];

    app->window = (__bridge_retained void*)window;
    app->view = (__bridge void*)contentView;
    app->layer = (__bridge_retained void*)metalLayer;
    app->device = (__bridge_retained void*)device;
    app->delegate = (__bridge_retained void*)delegate;

    app->width = (uint32_t)metalLayer.drawableSize.width;
    app->height = (uint32_t)metalLayer.drawableSize.height;
    if (app->width == 0 || app->height == 0) {
      app->width = desc->width;
      app->height = desc->height;
      metalLayer.drawableSize = CGSizeMake(app->width, app->height);
    }

    app->running = 1;
  }

  return true;
}

void cocoa_app_shutdown(cocoa_app_t* app) {
  if (app == NULL) {
    return;
  }

  @autoreleasepool {
    if (app->window != NULL) {
      NSWindow* window = (__bridge_transfer NSWindow*)app->window;
      [window setDelegate:nil];
      [window orderOut:nil];
      app->window = NULL;
    }
    if (app->delegate != NULL) {
      (void)(__bridge_transfer WindowDelegate*)app->delegate;
      app->delegate = NULL;
    }
    if (app->layer != NULL) {
      (void)(__bridge_transfer CAMetalLayer*)app->layer;
      app->layer = NULL;
    }
    if (app->device != NULL) {
      (void)(__bridge_transfer id<MTLDevice>)app->device;
      app->device = NULL;
    }
  }

  app->view = NULL;
  app->running = 0;
}

void cocoa_app_poll_events(cocoa_app_t* app) {
  if (app == NULL || !app->running) {
    return;
  }

  @autoreleasepool {
    NSApplication* application = [NSApplication sharedApplication];
    for (;;) {
      NSEvent* event = [application nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
      if (event == nil) {
        break;
      }
      [application sendEvent:event];
    }
    [application updateWindows];
  }
}

bool cocoa_app_is_running(const cocoa_app_t* app) {
  return app != NULL && app->running != 0;
}

bool cocoa_app_update_drawable(cocoa_app_t* app, uint32_t* width, uint32_t* height) {
  if (app == NULL || !app->running) {
    return false;
  }

  NSView* view = (__bridge NSView*)app->view;
  if (view == nil) {
    return false;
  }

  CGSize drawableSize = currentDrawableSize(view);
  uint32_t newWidth = (uint32_t)drawableSize.width;
  uint32_t newHeight = (uint32_t)drawableSize.height;
  if (newWidth == 0 || newHeight == 0) {
    newWidth = app->width;
    newHeight = app->height;
  }

  bool changed = newWidth != app->width || newHeight != app->height;
  if (changed) {
    app->width = newWidth;
    app->height = newHeight;

    CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)app->layer;
    NSWindow* window = (__bridge NSWindow*)app->window;

    if (metalLayer != nil) {
      metalLayer.contentsScale = window.backingScaleFactor;
      metalLayer.frame = view.bounds;
      metalLayer.drawableSize = CGSizeMake(newWidth, newHeight);
    }
  }

  if (width != NULL) {
    *width = app->width;
  }
  if (height != NULL) {
    *height = app->height;
  }

  return changed;
}

void cocoa_app_get_drawable_size(const cocoa_app_t* app, uint32_t* width, uint32_t* height) {
  if (width != NULL) {
    *width = app != NULL ? app->width : 0;
  }
  if (height != NULL) {
    *height = app != NULL ? app->height : 0;
  }
}

bgfx_platform_data_t cocoa_app_platform_data(const cocoa_app_t* app) {
  bgfx_platform_data_t data;
  memset(&data, 0, sizeof(data));
  if (app != NULL) {
    data.ndt = NULL;
    data.nwh = app->layer;
    data.context = app->device;
    data.backBuffer = NULL;
    data.backBufferDS = NULL;
  }
  return data;
}

#else

bool cocoa_app_init(cocoa_app_t* app, const cocoa_app_desc_t* desc) {
  (void)app;
  (void)desc;
  return false;
}

void cocoa_app_shutdown(cocoa_app_t* app) {
  (void)app;
}

void cocoa_app_poll_events(cocoa_app_t* app) {
  (void)app;
}

bool cocoa_app_is_running(const cocoa_app_t* app) {
  (void)app;
  return false;
}

bool cocoa_app_update_drawable(cocoa_app_t* app, uint32_t* width, uint32_t* height) {
  (void)app;
  if (width != NULL) {
    *width = 0;
  }
  if (height != NULL) {
    *height = 0;
  }
  return false;
}

void cocoa_app_get_drawable_size(const cocoa_app_t* app, uint32_t* width, uint32_t* height) {
  (void)app;
  if (width != NULL) {
    *width = 0;
  }
  if (height != NULL) {
    *height = 0;
  }
}

bgfx_platform_data_t cocoa_app_platform_data(const cocoa_app_t* app) {
  (void)app;
  bgfx_platform_data_t data;
  bgfx_platform_data_init(&data);
  return data;
}

#endif
