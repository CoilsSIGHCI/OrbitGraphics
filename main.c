#include <bgfx/c99/bgfx.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

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

  // Vertex structure
  struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;
  };

  // Simple triangle
  static struct PosColorVertex s_triangleVertices[] = {
      {0.0f, 0.5f, 0.0f, 0xff0000ff},
      {-0.5f, -0.5f, 0.0f, 0xff00ff00},
      {0.5f, -0.5f, 0.0f, 0xffff0000},
  };

  // Define vertex layout
  bgfx_vertex_layout_t layout;
  bgfx_vertex_layout_begin(&layout, BGFX_RENDERER_TYPE_NOOP);
  bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_COLOR0, 4, BGFX_ATTRIB_TYPE_UINT8,
                         true, false);
  bgfx_vertex_layout_end(&layout);

  // Create vertex buffer
  bgfx_vertex_buffer_handle_t vbh = bgfx_create_vertex_buffer(
      bgfx_make_ref(s_triangleVertices, sizeof(s_triangleVertices)), &layout,
      BGFX_BUFFER_NONE);

  // Use invalid program handle
  bgfx_program_handle_t program = BGFX_INVALID_HANDLE;

  // Central star (at origin)
  struct PosColorVertex starVertex = {0.0f, 0.0f, 0.0f, 0xffffff00};
  bgfx_vertex_buffer_handle_t vbhStar =
      bgfx_create_vertex_buffer(bgfx_make_ref(&starVertex, sizeof(starVertex)),
                                &layout, BGFX_BUFFER_NONE);

  // Orbiting planet
  struct PosColorVertex orbitVertex = {0.3f, 0.0f, 0.0f, 0xff00ffff};
  bgfx_dynamic_vertex_buffer_handle_t vbhOrbit =
      bgfx_create_dynamic_vertex_buffer_mem(
          bgfx_make_ref(&orbitVertex, sizeof(orbitVertex)), &layout,
          BGFX_BUFFER_NONE);

  bgfx_set_debug(BGFX_DEBUG_TEXT);
  bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xffffffff, 1.0f,
                      0);
  bgfx_set_view_rect(0, 0, 0, fbWidth, fbHeight);

  uint32_t frameCount = 0;
  while (cocoa_app_is_running(&app)) {
    cocoa_app_poll_events(&app);

    uint32_t newWidth = fbWidth;
    uint32_t newHeight = fbHeight;
    if (cocoa_app_update_drawable(&app, &newWidth, &newHeight)) {
      fbWidth = newWidth;
      fbHeight = newHeight;
      bgfx_reset(fbWidth, fbHeight, init.resolution.reset,
                 init.resolution.formatColor);
      bgfx_set_view_rect(0, 0, 0, fbWidth, fbHeight);
    }

    float t = (float)frameCount * 0.02f;
    float orbitRadius = 0.5f;
    struct PosColorVertex movingVertex = {
        orbitRadius * cosf(t), orbitRadius * sinf(t), 0.0f, 0xff00ffff};
    bgfx_update_dynamic_vertex_buffer(
        vbhOrbit, 0, bgfx_make_ref(&movingVertex, sizeof(movingVertex)));

    // Draw central star
    bgfx_set_vertex_buffer(0, vbhStar, 0, UINT32_MAX);
    bgfx_submit(0, program, 0, false);

    // Draw orbiting planet
    bgfx_set_dynamic_vertex_buffer(0, vbhOrbit, 0, UINT32_MAX);
    bgfx_submit(0, program, 0, false);

    bgfx_dbg_text_clear(0, false);
    bgfx_dbg_text_printf(0, 1, 0xf4, "bgfx frame %u", frameCount++);
    bgfx_frame(false);
  }

  bgfx_destroy_vertex_buffer(vbh);
  bgfx_destroy_vertex_buffer(vbhStar);
  bgfx_destroy_dynamic_vertex_buffer(vbhOrbit);
  bgfx_destroy_program(program);

  bgfx_shutdown();
  cocoa_app_shutdown(&app);
  return 0;
}
