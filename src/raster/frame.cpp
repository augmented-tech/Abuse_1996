#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include "frame.h"
#include "light.h"
#include "objects.h"
#include "particle.h"
#include "video.h"
#include <cstring>

static RasterCmd blank_cmd()
{
    RasterCmd c;
    c.op = ROP_CLIP;
    c.im = NULL;
    c.tim = NULL;
    c.x = 0;
    c.y = 0;
    c.x2 = 0;
    c.y2 = 0;
    c.color = 0;
    c.xoff = 0;
    c.yoff = 0;
    c.interpolate = 0;
    c.old_screen = NULL;
    c.old_aa = ivec2(0);
    c.old_bb = ivec2(0);
    c.call = NULL;
    c.v = NULL;
    c.obj = NULL;
    c.pf = NULL;
    c.dir = 0;
    return c;
}

void RasterFrame::reset()
{
    cmds.clear();
    if (cmds.capacity() < 2048)
        cmds.reserve(2048);
}

void RasterFrame::clip(int x, int y, int x2, int y2)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_CLIP;
    c.x = x;
    c.y = y;
    c.x2 = x2;
    c.y2 = y2;
    cmds.push_back(c);
}

void RasterFrame::put_image(image *im, int x, int y)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_PUT_IMAGE;
    c.im = im;
    c.x = x;
    c.y = y;
    cmds.push_back(c);
}

void RasterFrame::put_trans(TransImage *im, int x, int y)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_PUT_TRANS;
    c.tim = im;
    c.x = x;
    c.y = y;
    cmds.push_back(c);
}

void RasterFrame::put_filled(TransImage *im, int x, int y, int color)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_PUT_FILLED;
    c.tim = im;
    c.x = x;
    c.y = y;
    c.color = color;
    cmds.push_back(c);
}

void RasterFrame::cross(int x, int y, int xinc, int yinc, int color)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_CROSS;
    c.x = x;
    c.y = y;
    c.x2 = xinc;
    c.y2 = yinc;
    c.color = color;
    cmds.push_back(c);
}

void RasterFrame::call(void (*fn)(view *), view *v)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_CALL;
    c.call = fn;
    c.v = v;
    cmds.push_back(c);
}

void RasterFrame::object(game_object *o)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_OBJECT;
    c.obj = o;
    cmds.push_back(c);
}

void RasterFrame::particle(part_frame *pf, int x, int y, int dir)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_PARTICLE;
    c.pf = pf;
    c.x = x;
    c.y = y;
    c.dir = dir;
    cmds.push_back(c);
}

void RasterFrame::lift_lamp(game_object *o)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_LIFT_LAMP;
    c.obj = o;
    cmds.push_back(c);
}

void RasterFrame::save_lamp(game_object *o, view *v)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_SAVE_LAMP;
    c.obj = o;
    c.v = v;
    cmds.push_back(c);
}

void RasterFrame::light(view *v, int32_t xoff, int32_t yoff, image *old_screen, ivec2 old_aa, ivec2 old_bb, int interpolate)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_LIGHT;
    c.v = v;
    c.xoff = xoff;
    c.yoff = yoff;
    c.old_screen = old_screen;
    c.old_aa = old_aa;
    c.old_bb = old_bb;
    c.interpolate = interpolate;
    cmds.push_back(c);
}

int g_raster_backend = 1;
int g_raster_crc = 0;

uint32_t raster_crc_rect(image *im, int x, int y, int x2, int y2)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (int row = y; row < y2; row++)
    {
        uint8_t *p = im->scan_line(row) + x;
        for (int col = x; col < x2; col++, p++)
        {
            crc ^= *p;
            for (int b = 0; b < 8; b++)
            {
                uint32_t mask = (uint32_t)-(int)(crc & 1u);
                crc = (crc >> 1) ^ (0xEDB88320u & mask);
            }
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

static int rows_differ(image *a, image *b)
{
    int w = a->Size().x;
    int h = a->Size().y;
    for (int y = 0; y < h; y++)
    {
        if (memcmp(a->scan_line(y), b->scan_line(y), (size_t)w) != 0)
            return 1;
    }
    return 0;
}

int raster_selftest()
{
    if (light_cache_selftest())
        return 1;

    image *dst_a = new image(ivec2(32, 32));
    image *dst_b = new image(ivec2(32, 32));
    image *tile = new image(ivec2(8, 8));
    dst_a->clear(1);
    dst_b->clear(1);
    for (int y = 0; y < 8; y++)
        memset(tile->scan_line(y), (uint8_t)(10 + y), 8);

    image *saved = main_screen;
    main_screen = dst_a;
    main_screen->PutImage(tile, ivec2(4, 6));

    main_screen = dst_b;
    RasterFrame f;
    f.reset();
    f.clip(0, 0, 32, 32);
    f.put_image(tile, 4, 6);
    raster_execute_legacy(f);

    int bad = 0;
    if (raster_crc_rect(dst_a, 0, 0, 32, 32) != raster_crc_rect(dst_b, 0, 0, 32, 32))
        bad = 1;
    if (rows_differ(dst_a, dst_b))
        bad = 1;

    image *src = new image(ivec2(8, 8));
    src->clear(0);
    src->PutPixel(ivec2(1, 0), 9);
    TransImage *filled = new TransImage(src, "selftest");
    dst_a->clear(3);
    dst_b->clear(3);
    main_screen = dst_a;
    filled->PutFilled(main_screen, ivec2(4, 6), 0);
    main_screen = dst_b;
    RasterFrame g;
    g.reset();
    g.clip(0, 0, 32, 32);
    g.put_filled(filled, 4, 6, 0);
    raster_execute_legacy(g);
    if (raster_crc_rect(dst_a, 0, 0, 32, 32) != raster_crc_rect(dst_b, 0, 0, 32, 32))
        bad = 1;
    if (rows_differ(dst_a, dst_b))
        bad = 1;

    main_screen = saved;
    delete filled;
    delete src;
    delete tile;
    delete dst_b;
    delete dst_a;
    if (!bad)
        fprintf(stderr, "raster_selftest ok\n");
    return bad;
}

void raster_execute_legacy(RasterFrame const &f)
{
    for (size_t i = 0; i < f.cmds.size(); i++)
    {
        RasterCmd const &c = f.cmds[i];
        switch (c.op)
        {
        case ROP_CLIP:
            main_screen->SetClip(ivec2(c.x, c.y), ivec2(c.x2, c.y2));
            break;
        case ROP_PUT_IMAGE:
            main_screen->PutImage(c.im, ivec2(c.x, c.y));
            break;
        case ROP_PUT_FILLED:
            c.tim->PutFilled(main_screen, ivec2(c.x, c.y), (uint8_t)c.color);
            break;
        case ROP_PUT_TRANS:
            c.tim->PutImage(main_screen, ivec2(c.x, c.y));
            break;
        case ROP_CROSS:
            main_screen->Line(ivec2(c.x, c.y), ivec2(c.x + c.x2, c.y + c.y2), (uint8_t)c.color);
            main_screen->Line(ivec2(c.x + c.x2, c.y), ivec2(c.x, c.y + c.y2), (uint8_t)c.color);
            break;
        case ROP_CALL:
            c.call(c.v);
            break;
        case ROP_OBJECT:
            c.obj->draw();
            break;
        case ROP_PARTICLE:
            c.pf->draw(main_screen, c.x, c.y, c.dir);
            break;
        case ROP_LIFT_LAMP:
            raster_lift_lamp(c.obj);
            break;
        case ROP_SAVE_LAMP:
            raster_save_lamp(c.obj, c.v);
            break;
        case ROP_LIGHT:
            raster_light_legacy(c.v, c.xoff, c.yoff, c.old_screen, c.old_aa, c.old_bb, c.interpolate);
            break;
        }
    }
}
