/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef _VIDEO_HPP_
#define _VIDEO_HPP_

#define TRI_1024x768x256 0x62
#define TRI_800x600x256  0x5e
#define TRI_640x480x256  0x5c
#define VGA_320x200x256  0x13
#define CGA_640x200x2    6
#define XWINDOWS_256     256
#define XWINDOWS_2       2

#include "image.h"

extern int xres,yres;
extern unsigned int scale;//AR
extern int xoff,yoff;
extern image *main_screen;

extern int g_allow_present;
extern float g_present_shift_x, g_present_shift_y;
extern int g_hud_split;
extern int g_fov_present; // 1 = crop present to 320x200 classic FOV

void set_mode(int argc=0, char **argv=NULL);
void close_graphics();
void update_window_done();
void window_to_game(int wx, int wy, int &gx, int &gy);
void game_to_window(int gx, int gy, int &wx, int &wy);
void video_set_linear_filter(bool on);
void video_set_grab(bool on);
void present_src_size(int &pw, int &ph);

void update_dirty(image *im, int xoff=0, int yoff=0);
void put_part_image(image *im, int x, int y, int x1, int y1, int x2, int y2);
void put_image(image * im, int x, int y);

void clear_put_image(image *im, int x, int y);
int get_vmode();

#endif
