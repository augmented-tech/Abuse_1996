#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include "frame.h"
#include "video.h"

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
    c.call = NULL;
    c.v = NULL;
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

void RasterFrame::call(void (*fn)(view *), view *v)
{
    RasterCmd c = blank_cmd();
    c.op = ROP_CALL;
    c.call = fn;
    c.v = v;
    cmds.push_back(c);
}

void RasterFrame::light()
{
    RasterCmd c = blank_cmd();
    c.op = ROP_LIGHT;
    cmds.push_back(c);
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
            c.im->PutImage(main_screen, ivec2(c.x, c.y));
            break;
        case ROP_PUT_FILLED:
            c.tim->PutFilled(main_screen, ivec2(c.x, c.y), (uint8_t)c.color);
            break;
        case ROP_CALL:
            c.call(c.v);
            break;
        case ROP_LIGHT:
            break;
        }
    }
}
