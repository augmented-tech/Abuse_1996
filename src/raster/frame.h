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
    ROP_LIGHT,
    ROP_PUT_TRANS,
    ROP_CROSS,
    ROP_OBJECT,
    ROP_PARTICLE,
    ROP_LIFT_LAMP,
    ROP_SAVE_LAMP
};

class game_object;
class part_frame;

struct RasterCmd {
    RasterOp op;
    image *im;
    TransImage *tim;
    int32_t x, y, x2, y2;
    int color;
    int32_t xoff, yoff;
    int interpolate;
    image *old_screen;
    ivec2 old_aa, old_bb;
    void (*call)(view *);
    view *v;
    game_object *obj;
    part_frame *pf;
    int dir;
};

struct RasterFrame {
    std::vector<RasterCmd> cmds;
    void reset();
    void clip(int x, int y, int x2, int y2);
    void put_image(image *im, int x, int y);
    void put_trans(TransImage *im, int x, int y);
    void put_filled(TransImage *im, int x, int y, int color);
    void cross(int x, int y, int xinc, int yinc, int color);
    void call(void (*fn)(view *), view *v);
    void object(game_object *o);
    void particle(part_frame *pf, int x, int y, int dir);
    void lift_lamp(game_object *o);
    void save_lamp(game_object *o, view *v);
    void light(view *v, int32_t xoff, int32_t yoff, image *old_screen, ivec2 old_aa, ivec2 old_bb, int interpolate);
};

void raster_execute_legacy(RasterFrame const &f);
uint32_t raster_crc_rect(image *im, int x, int y, int x2, int y2);
int raster_selftest();

extern int g_raster_backend;
extern int g_raster_crc;

void raster_light_legacy(view *v, int32_t xoff, int32_t yoff, image *old_screen, ivec2 old_aa, ivec2 old_bb, int interpolate);
void raster_help_legacy(view *v);
void raster_lift_lamp(game_object *o);
void raster_save_lamp(game_object *o, view *v);

#endif
