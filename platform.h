#pragma once

#include <bgfx/c99/bgfx.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cocoa_app_desc_s {
  uint32_t width;
  uint32_t height;
  const char* title;
} cocoa_app_desc_t;

typedef struct cocoa_app_s {
  void* window;
  void* view;
  void* layer;
  void* device;
  void* delegate;
  uint32_t width;
  uint32_t height;
  int running;
} cocoa_app_t;

bool cocoa_app_init(cocoa_app_t* app, const cocoa_app_desc_t* desc);
void cocoa_app_shutdown(cocoa_app_t* app);

void cocoa_app_poll_events(cocoa_app_t* app);
bool cocoa_app_is_running(const cocoa_app_t* app);

bool cocoa_app_update_drawable(cocoa_app_t* app, uint32_t* width, uint32_t* height);
void cocoa_app_get_drawable_size(const cocoa_app_t* app, uint32_t* width, uint32_t* height);

bgfx_platform_data_t cocoa_app_platform_data(const cocoa_app_t* app);

#ifdef __cplusplus
}
#endif

