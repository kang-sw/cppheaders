/**
Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "cpph/third/stb.hpp"

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "import/stb_image_write.h"

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

namespace cpph::stbi {
uc* load_from_memory(const uc* buffer, int len, int* x, int* y, int* channels_in_file, int desired_channels) { return stbi_load_from_memory(buffer, len, x, y, channels_in_file, desired_channels); }
uc* load_from_callbacks(const io_callbacks* clbk, void* user, int* x, int* y, int* channels_in_file, int desired_channels) { return stbi_load_from_callbacks((stbi_io_callbacks const*)clbk, user, x, y, channels_in_file, desired_channels); }

uc* load(char const* filename, int* x, int* y, int* channels_in_file, int desired_channels) { return stbi_load(filename, x, y, channels_in_file, desired_channels); }
uc* load_from_file(FILE* f, int* x, int* y, int* channels_in_file, int desired_channels) { return stbi_load_from_file(f, x, y, channels_in_file, desired_channels); }

float* loadf_from_memory(uc const* buffer, int len, int* x, int* y, int* channels_in_file, int desired_channels) { return stbi_loadf_from_memory(buffer, len, x, y, channels_in_file, desired_channels); }
float* loadf_from_callbacks(io_callbacks const* clbk, void* user, int* x, int* y, int* channels_in_file, int desired_channels) { return stbi_loadf_from_callbacks((stbi_io_callbacks const*)clbk, user, x, y, channels_in_file, desired_channels); }

float* loadf(char const* filename, int* x, int* y, int* channels_in_file, int desired_channels) { return stbi_loadf(filename, x, y, channels_in_file, desired_channels); }
float* loadf_from_file(FILE* f, int* x, int* y, int* channels_in_file, int desired_channels) { return stbi_loadf_from_file(f, x, y, channels_in_file, desired_channels); }

int info_from_memory(uc const* buffer, int len, int* x, int* y, int* comp) { return stbi_info_from_memory(buffer, len, x, y, comp); }
int info_from_callbacks(io_callbacks const* clbk, void* user, int* x, int* y, int* comp) { return stbi_info_from_callbacks((stbi_io_callbacks const*)clbk, user, x, y, comp); }

int info(char const* filename, int* x, int* y, int* comp) { return stbi_info(filename, x, y, comp); }
int info_from_file(FILE* f, int* x, int* y, int* comp) { return stbi_info_from_file(f, x, y, comp); }

auto failure_reason() -> const char* { return stbi_failure_reason(); }
void image_free(void* retval_from_load) { stbi_image_free(retval_from_load); }

int write_png(char const* filename, int w, int h, int comp, const void* data, int stride_in_bytes) { return stbi_write_png(filename, w, h, comp, data, stride_in_bytes); }
int write_bmp(char const* filename, int w, int h, int comp, const void* data) { return stbi_write_bmp(filename, w, h, comp, data); }
int write_tga(char const* filename, int w, int h, int comp, const void* data) { return stbi_write_tga(filename, w, h, comp, data); }
int write_hdr(char const* filename, int w, int h, int comp, const float* data) { return stbi_write_hdr(filename, w, h, comp, data); }
int write_jpg(char const* filename, int x, int y, int comp, const void* data, int quality) { return stbi_write_jpg(filename, x, y, comp, data, quality); }

int write_png_to_func(write_func* func, void* context, int w, int h, int comp, const void* data, int stride_in_bytes) { return stbi_write_png_to_func(func, context, w, h, comp, data, stride_in_bytes); }
int write_bmp_to_func(write_func* func, void* context, int w, int h, int comp, const void* data) { return stbi_write_bmp_to_func(func, context, w, h, comp, data); }
int write_tga_to_func(write_func* func, void* context, int w, int h, int comp, const void* data) { return stbi_write_tga_to_func(func, context, w, h, comp, data); }
int write_hdr_to_func(write_func* func, void* context, int w, int h, int comp, const float* data) { return stbi_write_hdr_to_func(func, context, w, h, comp, data); }
int write_jpg_to_func(write_func* func, void* context, int x, int y, int comp, const void* data, int quality) { return stbi_write_jpg_to_func(func, context, x, y, comp, data, quality); }

void flip_vertically_on_write(int flip_boolean) { stbi_flip_vertically_on_write(flip_boolean); }
}  // namespace cpph::stbi

namespace cpph::stbrp {
void init_target(context* context, int width, int height, node* nodes, int num_nodes) { stbrp_init_target((stbrp_context*)context, width, height, (stbrp_node*)nodes, num_nodes); }
void setup_allow_out_of_mem(context* context, int allow_out_of_mem) { stbrp_setup_allow_out_of_mem((stbrp_context*)context, allow_out_of_mem); }
void setup_heuristic(context* context, int heuristic) { stbrp_setup_heuristic((stbrp_context*)context, heuristic); }
int pack_rects(context* context, rect* rects, int num_rects) { return stbrp_pack_rects((stbrp_context*)context, (stbrp_rect*)rects, num_rects); }
}  // namespace cpph::stbrp

namespace cpph::stbir {
int resize_uint8(const unsigned char* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                 unsigned char* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                 int num_channels)
{
    return stbir_resize_uint8(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, num_channels);
}

int resize_float(const float* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                 float* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                 int num_channels)
{
    // [\s\*\w]+\s+(\w+)([,\)])
    return stbir_resize_float(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, num_channels);
}

int resize_uint8_srgb(const unsigned char* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                      unsigned char* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                      int num_channels, int alpha_channel, int flags)
{
    return stbir_resize_uint8_srgb(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, num_channels, alpha_channel, flags);
}

// This function adds the ability to specify how requests to sample off the edge of the image are handled.
int resize_uint8_srgb_edgemode(const unsigned char* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                               unsigned char* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                               int num_channels, int alpha_channel, int flags,
                               edge edge_wrap_mode)
{
    return stbir_resize_uint8_srgb_edgemode(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, num_channels, alpha_channel, flags, (stbir_edge)edge_wrap_mode);
}

int resize_uint8_generic(const unsigned char* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                         unsigned char* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                         int num_channels, int alpha_channel, int flags,
                         edge edge_wrap_mode, filter filter, colorspace space,
                         void* alloc_context)
{
    return stbir_resize_uint8_generic(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, num_channels, alpha_channel, flags, (stbir_edge)edge_wrap_mode, (stbir_filter)filter, (stbir_colorspace)space, alloc_context);
}

int resize_uint16_generic(const uint16_t* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                          uint16_t* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                          int num_channels, int alpha_channel, int flags,
                          edge edge_wrap_mode, filter filter, colorspace space,
                          void* alloc_context)
{
    return stbir_resize_uint16_generic(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, num_channels, alpha_channel, flags, (stbir_edge)edge_wrap_mode, (stbir_filter)filter, (stbir_colorspace)space, alloc_context);
}

int resize_float_generic(const float* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                         float* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                         int num_channels, int alpha_channel, int flags,
                         edge edge_wrap_mode, filter filter, colorspace space,
                         void* alloc_context)
{
    return stbir_resize_float_generic(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, num_channels, alpha_channel, flags, (stbir_edge)edge_wrap_mode, (stbir_filter)filter, (stbir_colorspace)space, alloc_context);
}

int resize(const void* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
           void* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
           datatype datatype,
           int num_channels, int alpha_channel, int flags,
           edge edge_mode_horizontal, edge edge_mode_vertical,
           filter filter_horizontal, filter filter_vertical,
           colorspace space, void* alloc_context)
{
    return stbir_resize(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, (stbir_datatype)datatype, num_channels, alpha_channel, flags, (stbir_edge)edge_mode_horizontal, (stbir_edge)edge_mode_vertical, (stbir_filter)filter_horizontal, (stbir_filter)filter_vertical, (stbir_colorspace)space, alloc_context);
}

int resize_subpixel(const void* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                    void* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                    datatype datatype,
                    int num_channels, int alpha_channel, int flags,
                    edge edge_mode_horizontal, edge edge_mode_vertical,
                    filter filter_horizontal, filter filter_vertical,
                    colorspace space, void* alloc_context,
                    float x_scale, float y_scale,
                    float x_offset, float y_offset)
{
    return stbir_resize_subpixel(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, (stbir_datatype)datatype, num_channels, alpha_channel, flags, (stbir_edge)edge_mode_horizontal, (stbir_edge)edge_mode_vertical, (stbir_filter)filter_horizontal, (stbir_filter)filter_vertical, (stbir_colorspace)space, alloc_context, x_scale, y_scale, x_offset, y_offset);
}

int resize_region(const void* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
                  void* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                  datatype datatype,
                  int num_channels, int alpha_channel, int flags,
                  edge edge_mode_horizontal, edge edge_mode_vertical,
                  filter filter_horizontal, filter filter_vertical,
                  colorspace space, void* alloc_context,
                  float s0, float t0, float s1, float t1)
{
    return stbir_resize_region(input_pixels, input_w, input_h, input_stride_in_bytes, output_pixels, output_w, output_h, output_stride_in_bytes, (stbir_datatype)datatype, num_channels, alpha_channel, flags, (stbir_edge)edge_mode_horizontal, (stbir_edge)edge_mode_vertical, (stbir_filter)filter_horizontal, (stbir_filter)filter_vertical, (stbir_colorspace)space, alloc_context, s0, t0, s1, t1);
}
}  // namespace cpph::stbir
