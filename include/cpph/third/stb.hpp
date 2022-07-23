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
#pragma once
#include <cstdlib>
#include <cstdint>
#include <cstdio>

namespace cpph::stbi {
typedef unsigned char  uc;
typedef unsigned short us;

typedef struct
{
   int      (*read)  (void *user,char *data,int size);   // fill 'data' with 'size' bytes.  return number of bytes actually read
   void     (*skip)  (void *user,int n);                 // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
   int      (*eof)   (void *user);                       // returns nonzero if we are at end of file/data
} io_callbacks;


uc*       load_from_memory        (uc           const *buffer, int len   , int *x, int *y, int *channels_in_file, int desired_channels = 0);
uc*       load_from_callbacks     (io_callbacks const *clbk  , void *user, int *x, int *y, int *channels_in_file, int desired_channels = 0);

uc*       load                    (char const *filename, int *x, int *y, int *channels_in_file, int desired_channels = 0);
uc*       load_from_file          (FILE *f, int *x, int *y, int *channels_in_file, int desired_channels = 0);

float *   loadf_from_memory       (uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels = 0);
float *   loadf_from_callbacks    (io_callbacks const *clbk, void *user, int *x, int *y,  int *channels_in_file, int desired_channels = 0);

float *   loadf                   (char const *filename, int *x, int *y, int *channels_in_file, int desired_channels = 0);
float *   loadf_from_file         (FILE *f, int *x, int *y, int *channels_in_file, int desired_channels = 0);

int       info_from_memory        (uc const *buffer, int len, int *x, int *y, int *comp);
int       info_from_callbacks     (io_callbacks const *clbk, void *user, int *x, int *y, int *comp);

int       info                    (char const *filename,     int *x, int *y, int *comp);
int       info_from_file          (FILE *f,                  int *x, int *y, int *comp);

auto      failure_reason          () -> const char *;
void      image_free              (void *retval_from_load);


int       write_png               (char const *filename, int w, int h, int comp, const void  *data, int stride_in_bytes);
int       write_bmp               (char const *filename, int w, int h, int comp, const void  *data);
int       write_tga               (char const *filename, int w, int h, int comp, const void  *data);
int       write_hdr               (char const *filename, int w, int h, int comp, const float *data);
int       write_jpg               (char const *filename, int x, int y, int comp, const void  *data, int quality);

typedef void write_func           (void *context, void *data, int size);

int       write_png_to_func       (write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
int       write_bmp_to_func       (write_func *func, void *context, int w, int h, int comp, const void  *data);
int       write_tga_to_func       (write_func *func, void *context, int w, int h, int comp, const void  *data);
int       write_hdr_to_func       (write_func *func, void *context, int w, int h, int comp, const float *data);
int       write_jpg_to_func       (write_func *func, void *context, int x, int y, int comp, const void  *data, int quality);

void      flip_vertically_on_write(int flip_boolean);
}

namespace cpph::stbrp {
typedef struct context context;
typedef struct node    node;
typedef struct rect    rect;
typedef int            coord;

struct rect
{
          // reserved for your use:
          int                     id;

          // input:
          coord                   w, h;

          // output:
          coord                   x, y;
          int                     was_packed;  // non-zero if valid packing
}; // 16 bytes, nominally

void      init_target             (context *context, int width, int height, node *nodes, int num_nodes);
void      setup_allow_out_of_mem  (context *context, int allow_out_of_mem);
void      setup_heuristic         (context *context, int heuristic);

int       pack_rects              (context *context, rect *rects, int num_rects);

enum
{
          HEURISTIC_Skyline_default = 0,
          HEURISTIC_Skyline_BL_sortHeight = HEURISTIC_Skyline_default,
          HEURISTIC_Skyline_BF_sortHeight
};

struct node
{
          coord  x,y;
          node  *next;
};

struct context
{
          int                     width;
          int                     height;
          int                     align;
          int                     init_mode;
          int                     heuristic;
          int                     num_nodes;
          node                    *active_head;
          node                    *free_head;
          node                    extra[2]; // we allocate two extra nodes so optimal user-node-count is 'width' not 'width+2'
};
}

namespace cpph::stbir {

int       resize_uint8            (const unsigned char *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                         unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                         int            num_channels);

int       resize_float            (const float *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                         float *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                         int    num_channels);

enum {
          ALPHA_CHANNEL_NONE = -1,

          // Set this flag if your texture has premultiplied alpha. Otherwise, stbir will
          // use alpha-weighted resampling (effectively premultiplying, resampling,
          // then unpremultiplying).
          FLAG_ALPHA_PREMULTIPLIED = (1 << 0),
          // The specified alpha channel should be handled as gamma-corrected value even
          // when doing sRGB operations.
          FLAG_ALPHA_USES_COLORSPACE = (1 << 1)
};


int       resize_uint8_srgb       (const unsigned char *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                         unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                         int            num_channels , int alpha_channel, int flags);

typedef enum
{
          EDGE_CLAMP   = 1,
          EDGE_REFLECT = 2,
          EDGE_WRAP    = 3,
          EDGE_ZERO    = 4,
} edge;

// This function adds the ability to specify how requests to sample off the edge of the image are handled.
int       resize_uint8_srgb_edgemode(const unsigned char *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                           unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                           int            num_channels, int alpha_channel, int flags,
                                           edge           edge_wrap_mode);

typedef enum
{
          FILTER_DEFAULT      = 0,  // use same filter type that easy-to-use API chooses
          FILTER_BOX          = 1,  // A trapezoid w/1-pixel wide ramps, same result as box for integer scale ratios
          FILTER_TRIANGLE     = 2,  // On upsampling, produces same results as bilinear texture filtering
          FILTER_CUBICBSPLINE = 3,  // The cubic b-spline (aka Mitchell-Netrevalli with B=1,C=0), gaussian-esque
          FILTER_CATMULLROM   = 4,  // An interpolating cubic spline
          FILTER_MITCHELL     = 5,  // Mitchell-Netrevalli filter with B=1/3, C=1/3
} filter;

typedef enum
{
          COLORSPACE_LINEAR,
          COLORSPACE_SRGB,

          MAX_COLORSPACES,
} colorspace;

int       resize_uint8_generic    (const unsigned char *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                         unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                         int num_channels, int alpha_channel, int flags,
                                         edge edge_wrap_mode, filter filter, colorspace space,
                                         void *alloc_context);

int       resize_uint16_generic   (const uint16_t *input_pixels  , int input_w , int input_h , int input_stride_in_bytes,
                                         uint16_t *output_pixels , int output_w, int output_h, int output_stride_in_bytes,
                                         int num_channels, int alpha_channel, int flags,
                                         edge edge_wrap_mode, filter filter, colorspace space,
                                         void *alloc_context);

int       resize_float_generic    (const float *input_pixels         , int input_w , int input_h , int input_stride_in_bytes,
                                         float *output_pixels        , int output_w, int output_h, int output_stride_in_bytes,
                                         int num_channels, int alpha_channel, int flags,
                                         edge edge_wrap_mode, filter filter, colorspace space,
                                         void *alloc_context);

typedef enum
{
          TYPE_UINT8 ,
          TYPE_UINT16,
          TYPE_UINT32,
          TYPE_FLOAT ,

          MAX_TYPES
} datatype;

int       resize                  (const void *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                         void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                         datatype datatype,
                                         int num_channels, int alpha_channel, int flags,
                                         edge edge_mode_horizontal, edge edge_mode_vertical,
                                         filter filter_horizontal,  filter filter_vertical,
                                         colorspace space, void *alloc_context);

int       resize_subpixel         (const void *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                         void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                         datatype datatype,
                                         int num_channels, int alpha_channel, int flags,
                                         edge edge_mode_horizontal, edge edge_mode_vertical,
                                         filter filter_horizontal,  filter filter_vertical,
                                         colorspace space, void *alloc_context,
                                         float x_scale, float y_scale,
                                         float x_offset, float y_offset);

int       resize_region           (const void *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                         void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                         datatype datatype,
                                         int num_channels, int alpha_channel, int flags,
                                         edge edge_mode_horizontal, edge edge_mode_vertical,
                                         filter filter_horizontal,  filter filter_vertical,
                                         colorspace space, void *alloc_context,
                                         float s0, float t0, float s1, float t1);
}
