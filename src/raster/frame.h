#ifndef __RASTER_FRAME_H__
#define __RASTER_FRAME_H__

#include "common.h"
#include "image.h"
#include "transimage.h"
#include "view.h"
#include <cstdint>
#include <vector>

enum RasterOp : uint8_t {
    ROP_CLIP,
    ROP_PUT_IMAGE,
    ROP_PUT_FILLED,
    ROP_CALL,
    ROP_LIGHT
};

struct RasterCmd {
    RasterOp op;
    image *im;
    TransImage *tim;
    int32_t x, y, x2, y2;
    int color;
    void (*call)(view *);
    view *v;
};

struct RasterFrame {
    std::vector<RasterCmd> cmds;
    void reset();
    void clip(int x, int y, int x2, int y2);
    void put_image(image *im, int x, int y);
    void put_filled(TransImage *im, int x, int y, int color);
    void call(void (*fn)(view *), view *v);
    void light();
};

void raster_execute_legacy(RasterFrame const &f);
uint32_t raster_crc_rect(image *im, int x, int y, int x2, int y2);
int raster_selftest();

void raster_light_legacy(view *v, int32_t xoff, int32_t yoff, image *old_screen, ivec2 old_aa, ivec2 old_bb, int interpolate);
void raster_help_legacy(view *v);

#endif
