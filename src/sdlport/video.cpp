/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2001 Anthony Kruize <trandor@labyrinth.net.au>
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *	Copyright (c) 2016 Antonio Radojkovic <antonior.software@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include "SDL.h"

#include "common.h"

#include "filter.h"
#include "video.h"
#include "image.h"
#include "setup.h"
#include "errorui.h"

//AR all rendering goes to "main_screen" all over the code
//then using put_part_image() it is rendered to "surface"...I think
//then it is rendered to screen using update_window_done() where the surface is first copied to a OGL texture

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Surface *surface = NULL;
SDL_Surface *screen = NULL;
image *main_screen = NULL;

int mouse_xpad, mouse_ypad, mouse_xscale, mouse_yscale;
int xres, yres;

extern palette *lastl;
extern Settings settings;
int g_allow_present = 1;
float g_present_shift_x = 0;
float g_present_shift_y = 0;
int g_hud_split = 0;
int g_fov_present = 0;

void calculate_mouse_scaling();

void classic_present_size(int &pw, int &ph)
{
    pw = settings.fov_w;
    if(pw != 340 && pw != 360)
        pw = 320;
    if(pw > xres)
        pw = xres;
    if(pw < 1)
        pw = 320;
    ph = 200;
    if(ph > yres)
        ph = yres;
    if(ph < 1)
        ph = 200;
}

void present_src_size(int &pw, int &ph)
{
    pw = xres;
    ph = yres;
    if(g_fov_present)
        classic_present_size(pw, ph);
    if(pw < 1) pw = 1;
    if(ph < 1) ph = 1;
}

void visible_canvas_size(int &w, int &h)
{
    w = xres;
    h = yres;
    if(g_fov_present)
        classic_present_size(w, h);
    if(w < 1) w = 1;
    if(h < 1) h = 1;
}

ivec2 visible_center(ivec2 size)
{
    int w, h;
    visible_canvas_size(w, h);
    return ivec2((w - size.x) / 2, (h - size.y) / 2);
}

void video_set_linear_filter(bool on)
{
    settings.linear_filter = on;
#if SDL_VERSION_ATLEAST(2,0,12)
    if(texture)
        SDL_SetTextureScaleMode(texture, on ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
#endif
}

void video_set_grab(bool on)
{
    settings.grab_input = on;
    if(window)
        SDL_SetWindowGrab(window, on ? SDL_TRUE : SDL_FALSE);
}

SDL_DisplayMode desktop;
int window_w = 320, window_h = 200;
bool ar_fullscreen = false;
int ogl_scale = 1;
int ogl_w = 320, ogl_h = 200;

static void compute_present_rect(SDL_Rect *dest, int *out_w, int *out_h)
{
    int ow = window_w, oh = window_h;
    if(renderer)
        SDL_GetRendererOutputSize(renderer, &ow, &oh);
    if(ow < 1) ow = 1;
    if(oh < 1) oh = 1;

    int pw, ph;
    present_src_size(pw, ph);
    int iscale = ow / pw;
    int iscale_y = oh / ph;
    if(iscale_y < iscale)
        iscale = iscale_y;
    if(iscale < 1)
        iscale = 1;

    dest->w = pw * iscale;
    dest->h = ph * iscale;
    dest->x = (ow - dest->w) / 2;
    dest->y = (oh - dest->h) / 2;
    if(out_w) *out_w = ow;
    if(out_h) *out_h = oh;
}

void window_to_game(int wx, int wy, int &gx, int &gy)
{
    SDL_Rect dest;
    int ow, oh;
    compute_present_rect(&dest, &ow, &oh);
    if(window_w < 1) window_w = 1;
    if(window_h < 1) window_h = 1;
    if(dest.w < 1) dest.w = 1;
    if(dest.h < 1) dest.h = 1;

    float ox = wx * ((float)ow / (float)window_w);
    float oy = wy * ((float)oh / (float)window_h);
    int pw, ph;
    present_src_size(pw, ph);
    gx = (int)((ox - dest.x) * (float)pw / (float)dest.w);
    gy = (int)((oy - dest.y) * (float)ph / (float)dest.h);
    if(gx < 0) gx = 0;
    if(gy < 0) gy = 0;
    if(gx > pw - 1) gx = pw - 1;
    if(gy > ph - 1) gy = ph - 1;
}

void game_to_window(int gx, int gy, int &wx, int &wy)
{
    SDL_Rect dest;
    int ow, oh;
    compute_present_rect(&dest, &ow, &oh);
    if(window_w < 1) window_w = 1;
    if(window_h < 1) window_h = 1;
    float dpi_x = (float)ow / (float)window_w;
    float dpi_y = (float)oh / (float)window_h;
    int pw, ph;
    present_src_size(pw, ph);
    wx = (int)((dest.x + gx * (float)dest.w / (float)pw) / dpi_x);
    wy = (int)((dest.y + gy * (float)dest.h / (float)ph) / dpi_y);
}

static void apply_renderer_scale()
{
    if(!renderer)
        return;

    SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, settings.linear_filter ? "linear" : "nearest");
}

void video_change_settings(int scale_add, bool toggle_fullscreen);

//
// set_mode()
// Set the video mode
//
void set_mode(int argc, char **argv)
{
	int displayIndex = 0;
	desktop.w = 320;
	desktop.h = 200;

	if(SDL_GetDesktopDisplayMode(0,&desktop)!=0) printf("ERROR - failed to get display info\n");

	//AR scale window
    window_w = xres*scale;
	window_h = yres*scale;

	//fullscreen "scale"
	ogl_w = window_w;
	ogl_h = window_h;

	int window_type = 0;

	if(settings.fullscreen==1)		window_type |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if(settings.fullscreen==2)	window_type |= SDL_WINDOW_FULLSCREEN;

	if(settings.borderless)			window_type |= SDL_WINDOW_BORDERLESS;
    
	// FIXME: Set the icon for this window.  Looks nice on taskbars etc.
    //SDL_WM_SetIcon(SDL_LoadBMP("abuse.bmp"), NULL);

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, settings.vsync ? "1" : "0");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, settings.linear_filter ? "linear" : "nearest");
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    window = SDL_CreateWindow("Abuse",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        window_w, window_h,
        window_type|SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_RESIZABLE);

    if(!window)
    {
        show_startup_error("Video : Unable to create window : %s", SDL_GetError());
        exit(1);
    }

    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    if(settings.vsync)
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;

    renderer = SDL_CreateRenderer(window, -1, renderer_flags);
    if(!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if(!renderer)
    {
        show_startup_error("Video : Unable to create renderer : %s", SDL_GetError());
        exit(1);
    }

    SDL_RendererInfo rendererInfo;
    SDL_GetRendererInfo(renderer, &rendererInfo);
    int out_w = window_w, out_h = window_h;
    SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
    printf("Video : renderer=%s flags=0x%x vsync=%d %dx%d scale=%d window=%dx%d drawable=%dx%d\n",
        rendererInfo.name, rendererInfo.flags, settings.vsync ? 1 : 0,
        xres, yres, scale, window_w, window_h, out_w, out_h);

    apply_renderer_scale();

	screen = SDL_CreateRGBSurface(SDL_SWSURFACE, xres , yres , 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
#else
                0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
#endif
    if(screen == NULL)
    {
        show_startup_error("Video : Unable to create 32-bit surface: %s", SDL_GetError());
        exit(1);
    }

    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        xres, yres);
    if(texture == NULL)
    {
        show_startup_error("Video : Unable to create texture: %s", SDL_GetError());
        exit(1);
    }
#if SDL_VERSION_ATLEAST(2,0,12)
    SDL_SetTextureScaleMode(texture, settings.linear_filter ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
#endif
	
	// Create our 8-bit surface
    surface = SDL_CreateRGBSurface(0, xres, yres, 8, 0, 0, 0, 0);
    if(surface == NULL)
    {
        // Our surface is no good, we have to bail.
        show_startup_error("Video : Unable to create 8-bit surface: %s", SDL_GetError());
        exit(1);
    }

	// Create the screen image
    main_screen = new image(ivec2(xres, yres), NULL, 2);
    if(main_screen == NULL)
    {
        // Our screen image is no good, we have to bail.
        show_startup_error("Video : Unable to create screen image.");
        exit(1);
    }
    main_screen->clear();
	
	//hide the mouse cursor and set up the mouse
	if(settings.grab_input) SDL_SetWindowGrab(window,SDL_TRUE);
    SDL_ShowCursor(0);
    calculate_mouse_scaling(); 

	if(settings.fullscreen!=0) video_change_settings(0,true);

	//print some info
	//AR shows 640x480 when the size is lower than that... ???
	/*SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(window, &mode);
    printf("Video : %dx%d %dbpp\n", mode.w, mode.h, SDL_BITSPERPIXEL(mode.format));*/

    update_dirty(main_screen);
}

void video_change_settings(int scale_add, bool toggle_fullscreen)
{
	//AR
	if(toggle_fullscreen)
	{
		ar_fullscreen = !ar_fullscreen;
		if(ar_fullscreen)
		{
			SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP);
			SDL_GetWindowSize(window,&window_w,&window_h);
			
			//OGL texture rendering size, scale while if fits
			int scl = 1;
			while(1!=0)
				if(xres*scl<desktop.w && yres*scl<desktop.h)
				{
					ogl_scale = scl;
					ogl_w = xres*ogl_scale;
					ogl_h = yres*ogl_scale;					
					scl++;
				}
				else break;
		}
		else SDL_SetWindowFullscreen(window,0);
	}

	static int overscale = 0;
	
	if(!ar_fullscreen)
	{
		//scale window
		int new_scale = scale + scale_add;

		if(new_scale>0 && xres*new_scale<=desktop.w && yres*new_scale<=desktop.h)
		{
			//scale windows if it fits on screen
			scale = new_scale;
			SDL_SetWindowSize(window,xres*scale,yres*scale);
		}
	}
	else
	{
		//scale OGL texture rendering size
		int new_scale = ogl_scale + scale_add;

		if(overscale==2 && scale_add==-1) overscale = 0;

		if(new_scale>0)
		{
			if(xres*new_scale>=desktop.w || yres*new_scale>=desktop.h)
			{
				if(overscale==0)
				{
					if(yres*((float)desktop.w/xres)<desktop.h)
					{
						//limit scale by monitor width
						ogl_w = desktop.w;
						ogl_h = yres*((float)desktop.w/xres);
					}
					else
					{
						//limit scale by monitor height
						ogl_w = xres*((float)desktop.h/yres);
						ogl_h = desktop.h;
					}
					overscale = 1;
					ogl_scale = new_scale;
				}
				else if(overscale==1)
				{
					//match screen to desktop/monitor size
					ogl_w = desktop.w;
					ogl_h = desktop.h;

					overscale = 2;
					ogl_scale = new_scale;
				}				
			}
			else if(xres*new_scale<=desktop.w && yres*new_scale<=desktop.h)
			{
				//scale and keep aspect
				ogl_scale = new_scale;
				ogl_w = xres*ogl_scale;
				ogl_h = yres*ogl_scale;
				overscale = 0;
			}
		}
	}

	SDL_GetWindowSize(window,&window_w,&window_h);
	SDL_SetWindowPosition(window,desktop.w/2-window_w/2,desktop.h/2-window_h/2);
    apply_renderer_scale();

    calculate_mouse_scaling();
}

void calculate_mouse_scaling()
{
	if(!ar_fullscreen || settings.mouse_scale==0)
	{
		// We need to determine the appropriate mouse scaling
		float scale_x = window_w/xres;
		float scale_y = window_h/yres;

		// Re-calculate the mouse scaling
		mouse_xscale = (window_w << 16) / xres;
		mouse_yscale = (window_h << 16) / yres;

		// And calculate the padding
		mouse_xpad = scale_x;
		mouse_ypad = scale_y;
	}
	else
	{
		// We need to determine the appropriate mouse scaling
		float scale_x = ogl_w/xres;
		float scale_y = ogl_h/yres;

		// Re-calculate the mouse scaling
		mouse_xscale = (ogl_w << 16) / xres;
		mouse_yscale = (ogl_h << 16) / yres;

		// And calculate the padding
		mouse_xpad = scale_x;
		mouse_ypad = scale_y;
	}
}

//
// close_graphics()
// Shutdown the video mode
//
void close_graphics()
{
    if(lastl) delete lastl;
    lastl = NULL;

	if(surface) SDL_FreeSurface(surface);
    if(screen) SDL_FreeSurface(screen);
    if(texture) SDL_DestroyTexture(texture);
    if(renderer) SDL_DestroyRenderer(renderer);
    if(window) SDL_DestroyWindow(window);
    texture = NULL;
    renderer = NULL;
    window = NULL;

    delete main_screen;
}

// put_part_image()
// Draw only dirty parts of the image
//
void put_part_image(image *im, int x, int y, int x1, int y1, int x2, int y2)
{
    int xe, ye;
    SDL_Rect srcrect, dstrect;
    int ii, jj;
    int srcx, srcy, xstep, ystep;
    Uint8 *dpixel;
    Uint16 dinset;

    if(y > yres || x > xres)
        return;

    CHECK(x1 >= 0 && x2 >= x1 && y1 >= 0 && y2 >= y1);

    // Adjust if we are trying to draw off the screen
    if(x < 0)
    {
        x1 += -x;
        x = 0;
    }
    srcrect.x = x1;
    if(x + (x2 - x1) >= xres)
        xe = xres - x + x1 - 1;
    else
        xe = x2;

    if(y < 0)
    {
        y1 += -y;
        y = 0;
    }
    srcrect.y = y1;
    if(y + (y2 - y1) >= yres)
        ye = yres - y + y1 - 1;
    else
        ye = y2;

    if(srcrect.x >= xe || srcrect.y >= ye)
        return;

    // Scale the image onto the surface
    srcrect.w = xe - srcrect.x;
    srcrect.h = ye - srcrect.y;
    dstrect.x = x;
    dstrect.y = y;
    dstrect.w = srcrect.w;
    dstrect.h = srcrect.h;

    xstep = (srcrect.w << 16) / dstrect.w;
    ystep = (srcrect.h << 16) / dstrect.h;

    srcy = ((srcrect.y) << 16);
    dinset = ((surface->w - dstrect.w)) * surface->format->BytesPerPixel;

    // Lock the surface if necessary
    if(SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

    dpixel = (Uint8 *)surface->pixels;
    dpixel += (dstrect.x + ((dstrect.y) * surface->w)) * surface->format->BytesPerPixel;

    // Update surface part
    srcy = srcrect.y;
    dpixel = ((Uint8 *)surface->pixels) + y * surface->w + x ;
    for(ii=0 ; ii < srcrect.h; ii++)
    {
        memcpy(dpixel, im->scan_line(srcy) + srcrect.x , srcrect.w);
        dpixel += surface->w;
        srcy ++;
    }

    // Unlock the surface if we locked it.
    if(SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
}

//
// load()
// Set the palette
//
void palette::load()
{
    if(lastl)
        delete lastl;
    lastl = copy();

    // Force to only 256 colours.
    // Shouldn't be needed, but best to be safe.
    if(ncolors > 256)
        ncolors = 256;

#ifdef WIN32
	// FIXME: Really, this applies to anything that doesn't allow dynamic stack allocation
	SDL_Color colors[256];
#else
    SDL_Color colors[ncolors];
#endif
    for(int ii = 0; ii < ncolors; ii++)
    {
        colors[ii].r = red(ii);
        colors[ii].g = green(ii);
        colors[ii].b = blue(ii);
        colors[ii].a = 255;
    }
    SDL_SetPaletteColors(surface->format->palette, colors, 0, ncolors);

    // Applying a palette mid-frame (damage/heal flashes) used to SDL_RenderPresent
    // a half-drawn, unlit buffer and block on vsync. The real present is at flush.
    if(g_allow_present)
        update_window_done();
}

//
// load_nice()
//
void palette::load_nice()
{
    load();
}

// ---- support functions ----

void update_window_done()
{
	SDL_BlitSurface(surface, NULL, screen, NULL);
    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);

    if(window)
        SDL_GetWindowSize(window, &window_w, &window_h);

    SDL_Rect dest;
    int ow = 0, oh = 0;
    compute_present_rect(&dest, &ow, &oh);
    int pw, ph;
    present_src_size(pw, ph);
    int iscale = pw > 0 ? dest.w / pw : 1;
    if(iscale < 1) iscale = 1;

    bool subpixel = (g_present_shift_x != 0.f || g_present_shift_y != 0.f)
        && g_hud_split > 0 && g_hud_split < ph;

    if(subpixel || dest.x > 0 || dest.y > 0 || dest.w < ow || dest.h < oh)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

#if SDL_VERSION_ATLEAST(2,0,10)
    if(subpixel)
    {
        SDL_Rect play_clip = dest;
        play_clip.h = g_hud_split * iscale;

        SDL_Rect src_w = { 0, 0, pw, g_hud_split };
        SDL_FRect dst_w;
        dst_w.x = (float)dest.x - g_present_shift_x * (float)iscale;
        dst_w.y = (float)dest.y - g_present_shift_y * (float)iscale;
        dst_w.w = (float)dest.w;
        dst_w.h = (float)(g_hud_split * iscale);

        SDL_RenderSetClipRect(renderer, &play_clip);
        SDL_RenderCopyF(renderer, texture, &src_w, &dst_w);
        SDL_RenderSetClipRect(renderer, NULL);

        SDL_Rect src_h = { 0, g_hud_split, pw, ph - g_hud_split };
        SDL_Rect dst_h = { dest.x, dest.y + g_hud_split * iscale,
            dest.w, (ph - g_hud_split) * iscale };
        SDL_RenderCopy(renderer, texture, &src_h, &dst_h);
    }
    else
#endif
    {
        SDL_Rect src = { 0, 0, pw, ph };
        SDL_RenderCopy(renderer, texture, (pw == xres && ph == yres) ? NULL : &src, &dest);
    }

    SDL_RenderPresent(renderer);
}