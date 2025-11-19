#include <bgfx/c99/bgfx.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform.h"

static void identity_matrix(float *mtx) {
  memset(mtx, 0, sizeof(float) * 16);
  mtx[0] = mtx[5] = mtx[10] = mtx[15] = 1.0f;
}

static FILE *open_shader_file(const char *relativePath, char *resolvedPath,
                              size_t resolvedSize) {
  static const char *prefixes[] = {
      "", "../", "../../", "../../../", "../../../../", "../../../../../"};
  for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
    snprintf(resolvedPath, resolvedSize, "%s%s", prefixes[i], relativePath);
    FILE *file = fopen(resolvedPath, "rb");
    if (file != NULL) {
      return file;
    }
  }
  resolvedPath[0] = '\0';
  return NULL;
}

static bgfx_shader_handle_t load_shader(const char *relativePath) {
  char resolvedPath[PATH_MAX];
  FILE *file =
      open_shader_file(relativePath, resolvedPath, sizeof(resolvedPath));
  if (!file) {
    fprintf(stderr, "Failed to open shader (relative path %s)\n", relativePath);
    bgfx_shader_handle_t invalid = BGFX_INVALID_HANDLE;
    return invalid;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size <= 0) {
    fprintf(stderr, "Shader file is empty: %s\n", resolvedPath);
    fclose(file);
    bgfx_shader_handle_t invalid = BGFX_INVALID_HANDLE;
    return invalid;
  }

  uint8_t *data = malloc((size_t)size);
  if (!data) {
    fprintf(stderr, "Failed to allocate memory for shader: %s\n", resolvedPath);
    fclose(file);
    bgfx_shader_handle_t invalid = BGFX_INVALID_HANDLE;
    return invalid;
  }

  if (fread(data, 1, (size_t)size, file) != (size_t)size) {
    fprintf(stderr, "Failed to read shader: %s\n", resolvedPath);
    free(data);
    fclose(file);
    bgfx_shader_handle_t invalid = BGFX_INVALID_HANDLE;
    return invalid;
  }

  fclose(file);

  const bgfx_memory_t *mem = bgfx_copy(data, (uint32_t)size);
  free(data);
  return bgfx_create_shader(mem);
}

static const char *shader_directory(bgfx_renderer_type_t renderer) {
  switch (renderer) {
  case BGFX_RENDERER_TYPE_METAL:
    return "Library/bgfx/examples/runtime/shaders/metal/";
  case BGFX_RENDERER_TYPE_OPENGL:
  case BGFX_RENDERER_TYPE_OPENGLES:
    return "Library/bgfx/examples/runtime/shaders/glsl/";
  case BGFX_RENDERER_TYPE_VULKAN:
    return "Library/bgfx/examples/runtime/shaders/spirv/";
  case BGFX_RENDERER_TYPE_DIRECT3D11:
  case BGFX_RENDERER_TYPE_DIRECT3D12:
    return "Library/bgfx/examples/runtime/shaders/dx11/";
  default:
    return "Library/bgfx/examples/runtime/shaders/metal/";
  }
}

static bgfx_program_handle_t
create_basic_program(bgfx_renderer_type_t renderer) {
  const char *dir = shader_directory(renderer);
  char vsPath[512];
  char fsPath[512];
  snprintf(vsPath, sizeof(vsPath), "%svs_cubes.bin", dir);
  snprintf(fsPath, sizeof(fsPath), "%sfs_cubes.bin", dir);

  bgfx_shader_handle_t vsh = load_shader(vsPath);
  bgfx_shader_handle_t fsh = load_shader(fsPath);
  if (!BGFX_HANDLE_IS_VALID(vsh) || !BGFX_HANDLE_IS_VALID(fsh)) {
    fprintf(stderr, "Failed to load shaders for renderer %d\n", renderer);
    if (BGFX_HANDLE_IS_VALID(vsh)) {
      bgfx_destroy_shader(vsh);
    }
    if (BGFX_HANDLE_IS_VALID(fsh)) {
      bgfx_destroy_shader(fsh);
    }
    bgfx_program_handle_t invalid = BGFX_INVALID_HANDLE;
    return invalid;
  }

  return bgfx_create_program(vsh, fsh, true);
}

static void ortho_matrix(float *mtx, float left, float right, float bottom,
                         float top, float znear, float zfar) {
  float width = right - left;
  float height = top - bottom;
  float depth = zfar - znear;

  memset(mtx, 0, sizeof(float) * 16);
  mtx[0] = 2.0f / width;
  mtx[5] = 2.0f / height;
  mtx[10] = 1.0f / depth;
  mtx[12] = -(right + left) / width;
  mtx[13] = -(top + bottom) / height;
  mtx[14] = -znear / depth;
  mtx[15] = 1.0f;
}

static void update_view_projection(uint32_t fbWidth, uint32_t fbHeight,
                                   float *view, float *proj) {
  identity_matrix(view);

  float aspect = (float)fbWidth / (float)fbHeight;
  float left = -aspect;
  float right = aspect;
  float bottom = -1.0f;
  float top = 1.0f;
  float znear = -1.0f;
  float zfar = 1.0f;
  ortho_matrix(proj, left, right, bottom, top, znear, zfar);

  bgfx_set_view_transform(0, view, proj);
}

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
  bgfx_vertex_layout_begin(&layout, bgfx_get_renderer_type());
  bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_COLOR0, 4, BGFX_ATTRIB_TYPE_UINT8,
                         true, false);
  bgfx_vertex_layout_end(&layout);

  // Create vertex buffer
  bgfx_vertex_buffer_handle_t vbh = bgfx_create_vertex_buffer(
      bgfx_make_ref(s_triangleVertices, sizeof(s_triangleVertices)), &layout,
      BGFX_BUFFER_NONE);

  bgfx_program_handle_t program = create_basic_program(init.type);
  if (!BGFX_HANDLE_IS_VALID(program)) {
    fprintf(stderr, "Failed to create shader program.\n");
    bgfx_destroy_vertex_buffer(vbh);
    bgfx_shutdown();
    cocoa_app_shutdown(&app);
    return 1;
  }

  // Central star (at origin)
  struct PosColorVertex starVertex = {0.0f, 0.0f, 0.0f, 0xffffff00};
  bgfx_vertex_buffer_handle_t vbhStar = bgfx_create_vertex_buffer(
      bgfx_copy(&starVertex, sizeof(starVertex)), &layout, BGFX_BUFFER_NONE);

  // Orbiting planet
  struct PosColorVertex orbitVertex = {0.3f, 0.0f, 0.0f, 0xff00ffff};
  bgfx_dynamic_vertex_buffer_handle_t vbhOrbit =
      bgfx_create_dynamic_vertex_buffer_mem(
          bgfx_copy(&orbitVertex, sizeof(orbitVertex)), &layout,
          BGFX_BUFFER_NONE);

  bgfx_set_debug(BGFX_DEBUG_TEXT);
  bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xff101020, 1.0f,
                      0);
  bgfx_set_view_rect(0, 0, 0, fbWidth, fbHeight);
  float view[16];
  float proj[16];
  update_view_projection(fbWidth, fbHeight, view, proj);
  float model[16];
  identity_matrix(model);

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
      update_view_projection(fbWidth, fbHeight, view, proj);
    }

    bgfx_touch(0);

    float t = (float)frameCount * 0.02f;
    float orbitRadius = 0.5f;
    struct PosColorVertex movingVertex = {
        orbitRadius * cosf(t), orbitRadius * sinf(t), 0.0f, 0xff00ffff};
    bgfx_update_dynamic_vertex_buffer(
        vbhOrbit, 0, bgfx_copy(&movingVertex, sizeof(movingVertex)));

    // Draw reference triangle so pipeline issues are obvious.
    bgfx_set_state(BGFX_STATE_DEFAULT, 0);
    bgfx_set_transform(model, 1);
    bgfx_set_vertex_buffer(0, vbh, 0, UINT32_MAX);
    bgfx_submit(0, program, 0, false);

    // Draw central star
    bgfx_set_state(BGFX_STATE_DEFAULT | BGFX_STATE_PT_POINTS, 0);
    bgfx_set_transform(model, 1);
    bgfx_set_vertex_buffer(0, vbhStar, 0, UINT32_MAX);
    bgfx_submit(0, program, 0, false);

    // Draw orbiting planet
    bgfx_set_state(BGFX_STATE_DEFAULT | BGFX_STATE_PT_POINTS, 0);
    bgfx_set_transform(model, 1);
    bgfx_set_dynamic_vertex_buffer(0, vbhOrbit, 0, UINT32_MAX);
    bgfx_submit(0, program, 0, false);

    bgfx_dbg_text_clear(0, false);
    bgfx_dbg_text_printf(0, 1, 0xf4, "bgfx frame %u", frameCount);
    const bgfx_stats_t *stats = bgfx_get_stats();
    if (stats) {
      bgfx_dbg_text_printf(0, 2, 0xf0, "draw calls %u", stats->numDraw);
    }
    frameCount++;
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
