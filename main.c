#include <bgfx/c99/bgfx.h>
#include <stdio.h>

#include "platform.h"

int main(void) {
  cocoa_app_desc_t desc = {
      .width = 800,
      .height = 600,
      .title = "bgfx test",
  };

  cocoa_app_t app;
  if (!cocoa_app_init(&app, &desc)) {
    fprintf(stderr, "Failed to initialise Cocoa window.\n");
    return 1;
  }

  bgfx_init_t init;
  bgfx_init_ctor(&init);
  init.type = BGFX_RENDERER_TYPE_METAL;
  init.platformData = cocoa_app_platform_data(&app);
  init.resolution.reset = BGFX_RESET_VSYNC;
  init.resolution.formatColor = BGFX_TEXTURE_FORMAT_BGRA8;
  init.resolution.formatDepthStencil = BGFX_TEXTURE_FORMAT_D24S8;

  uint32_t fbWidth = 0;
  uint32_t fbHeight = 0;
  cocoa_app_get_drawable_size(&app, &fbWidth, &fbHeight);
  if (fbWidth == 0 || fbHeight == 0) {
    fbWidth = desc.width;
    fbHeight = desc.height;
  }
  init.resolution.width = fbWidth;
  init.resolution.height = fbHeight;

  if (!bgfx_init(&init)) {
    fprintf(stderr, "Failed to init bgfx\n");
    cocoa_app_shutdown(&app);
    return 1;
  }

  bgfx_set_debug(BGFX_DEBUG_TEXT);
  bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
  bgfx_set_view_rect(0, 0, 0, fbWidth, fbHeight);

  uint32_t frameCount = 0;
  while (cocoa_app_is_running(&app)) {
    cocoa_app_poll_events(&app);

    uint32_t newWidth = fbWidth;
    uint32_t newHeight = fbHeight;
    if (cocoa_app_update_drawable(&app, &newWidth, &newHeight)) {
      fbWidth = newWidth;
      fbHeight = newHeight;
      bgfx_reset(fbWidth, fbHeight, init.resolution.reset, init.resolution.formatColor);
      bgfx_set_view_rect(0, 0, 0, fbWidth, fbHeight);
    }

    bgfx_dbg_text_clear(0, false);
    bgfx_dbg_text_printf(0, 1, 0xf4, "bgfx frame %u", frameCount++);
    bgfx_touch(0);
    bgfx_frame(false);
  }

  bgfx_shutdown();
  cocoa_app_shutdown(&app);
  return 0;
}

