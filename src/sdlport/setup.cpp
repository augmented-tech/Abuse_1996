/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2001 Anthony Kruize <trandor@labyrinth.net.au>
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *  Copyright (c) 2016 Antonio Radojkovic <antonior.software@gmail.com>
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

#ifdef WIN32
# include <Windows.h>
# include <ShlObj.h>
# include <direct.h>
# define strcasecmp _stricmp
#endif
#ifdef __APPLE__
# include <CoreFoundation/CoreFoundation.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <string>
#include "SDL.h"

#include "specs.h"
#include "keys.h"
#include "setup.h"
#include "errorui.h"
#include "sound.h"

extern int sfx_volume, music_volume;
void apply_game_fov();
void video_set_linear_filter(bool on);
void video_set_grab(bool on);

//AR
#include <fstream>
#include <sstream>
#include <vector>
extern Settings settings;
//

extern int xres, yres;					//video.cpp
extern int sfx_volume, music_volume;	//loader.cpp
unsigned int scale;						//AR was static, removed for external

//AR tmp, until I figure out compiling stuff and cmake...
int AR_ToInt(std::string value)
{
	int n = 1;

	std::stringstream stream(value);
	stream >> n;

	return n;
}

bool AR_ToBool(std::string value)
{
	bool n = false;

	std::stringstream stream(value);
	stream >> n;

	return n;
}

bool AR_GetAttr(std::string line, std::string &attr, std::string &value)
{
	attr = value = "";

	std::size_t found = line.find("=");

	//no "="
	if(found==std::string::npos || found==line.size()-1) return false;
	
	attr = line.substr(0,found);
	value = line.substr(found+1,line.size()-1);

	//empty attribute or value
	if(attr.empty() || value.empty()) return false;
	
	return true;
}
//

Settings::Settings()
{
	//screen
	this->fullscreen		= 0;		// start in window (F10 toggles desktop fullscreen)
	this->borderless		= false;
	this->vsync				= true;		// vsync for tear-free present
	this->xres				= 640;		// hires title/menu art is authored at 640x400
	this->yres				= 400;
	this->scale				= 2;		// 1280x800 window, integer scaled
	this->linear_filter		= false;    // nearest-neighbor pixels
	this->hires				= 1;
	this->render_interp		= 1.0f;
	this->classic_fov		= 0;

	//sound
	this->mono				= false;	// disable stereo sound
	this->no_sound			= false;	// disable sound
	this->no_music			= false;	// disable music	
	this->volume_sound		= 127;
	this->volume_music		= 127;
	this->soundfont			= "";

	//random
	this->local_save		= true;
	this->grab_input		= false;	// don't grab the input
	this->editor			= false;	// disable editor mode
	this->physics_update	= 65;		// original 65ms/15 FPS
	this->mouse_scale		= 0;		// match desktop
	this->big_font			= false;
	//
	this->bullet_time		= false;
	this->bullet_time_add	= 1.2f;

	this->player_touching_console = false;
	this->config_path		= "";
	this->in_game			= false;

	this->cheat_god = cheat_bullettime = false;
	
	//player controls
	this->up		= key_value("w");
    this->down		= key_value("s");
    this->left		= key_value("a");
    this->right		= key_value("d");
    this->up_2		= key_value("UP");
    this->down_2	= key_value("DOWN");
    this->left_2	= key_value("LEFT");
    this->right_2	= key_value("RIGHT");
	this->b1		= key_value("SHIFT_L");	//special
	this->b2		= key_value("SPACE");	//fire
	this->b2_2		= key_value("f");		//alternate fire
    this->b3		= key_value("q");		//weapons
    this->b4		= key_value("e");
	this->bt		= key_value("CTRL_L");	//special2, bulettime

	//controller settings
	this->ctr_aim			= true;		// analog aim / stick move when a pad is present
	this->ctr_stick_aim		= 1;		// modern 60 Hz latch; set classic to revert to 2016 ring
	this->ctr_aim_correctx	= 0;
	this->ctr_cd			= 100;
	this->ctr_rst_s			= 10;
	this->ctr_rst_dz		= 5000;		// aiming
	this->ctr_lst_dzx		= 10000;	// move left right
	this->ctr_lst_dzy		= 25000;	// up/jump, down/use
	this->ctr_aim_x			= 0;
	this->ctr_aim_y			= 0;
	this->ctr_aim_latched	= false;
	this->ctr_aim_angle		= 0;
	this->ctr_mouse_x		= 0;
	this->ctr_mouse_y		= 0;

	//controller buttons
	this->ctr_a = "up";
	this->ctr_b = "down";
	this->ctr_x = "b3";
	this->ctr_y = "b4";
	//
	this->ctr_lst = "b1";
	this->ctr_rst = "down";
	//
	this->ctr_lsr = "b2";
	this->ctr_rsr = "b3";
	//
	this->ctr_ltg = "bt";
	this->ctr_rtg = "b2";
	//
	this->ctr_f5 = SDL_CONTROLLER_BUTTON_LEFTSTICK;
	this->ctr_f9 = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
}

//////////
////////// CREATE DEFAULT "config.txt" FILE
//////////

bool Settings::CreateConfigFile(std::string file_path)
{
	std::ofstream out(file_path.c_str());
	if(!out.is_open())
	{
		std::string tmp = "ERROR - CreateConfigFile() - Failed to create \"" + file_path + "\"\n";
		printf(tmp.c_str());

		return false;
	}
	
	out << "; Abuse-SDL configuration file (v0.9a)" << std::endl;
	out << std::endl;
	//
	out << "; SCREEN SETTINGS" << std::endl;
	out << std::endl;
	out << ";0 - window, 1 - fullscreen window, 2 - fullscreen" << std::endl;
	out << "fullscreen=" << this->fullscreen << std::endl;
	out << "borderless=" << this->borderless << std::endl;
	out << "vsync=" << this->vsync << std::endl;
	out << std::endl;
	out << "; Game screen size (original 320x200, use 640x400 with hires=1)" << std::endl;
	out << "screen_width=" << this->xres << std::endl;
	out << "screen_height=" << this->yres << std::endl;
	out << std::endl;
	out << "; Scale window" << std::endl;
	out << "scale=" << this->scale << std::endl;
	out << std::endl;	
	out << "; Enable high resolution screens, buttons and font" << std::endl;
	out << "hires=" << this->hires << std::endl;
	out << "big_font=" << this->big_font << std::endl;
	out << std::endl;
	out << "; Use linear texture filter (nearest is default)" << std::endl;
	out << "linear_filter=" << this->linear_filter << std::endl;
	out << std::endl;
	out << "; Gameplay FOV: wide = current camera, classic = original 320x200 scaled to the window" << std::endl;
	out << "fov=" << (this->classic_fov ? "classic" : "wide") << std::endl;
	out << std::endl;
	//
	out << "; SOUND SETTINGS" << std::endl;
	out << std::endl;
	out << "; Volume (0-127)" << std::endl;
	out << "volume_sound=" << this->volume_sound << std::endl;
	out << "volume_music=" << this->volume_music << std::endl;
	out << std::endl;	
	out << "; Disable music and sound effects" << std::endl;
	out << "no_music=" << this->no_music << std::endl;
	out << "no_sound=" << this->no_sound << std::endl;
	out << std::endl;
	out << "; Use mono audio only" << std::endl;
	out << "mono=" << this->mono << std::endl;
	out << std::endl;
	out << "; Optional SoundFont (.sf2). Empty = auto (Application Support/abuse/soundfont.sf2, then bundled Vintage Dreams)" << std::endl;
	out << "soundfont=" << (this->soundfont.empty() ? "auto" : this->soundfont) << std::endl;
	out << std::endl;
	//
	out << "; RANDOM SETTINGS" << std::endl;
	out << std::endl;
	out << "; Enable editor mode" << std::endl;
	out << "editor=" << this->editor << std::endl;
	out << std::endl;	
	out << "; Grab the mouse and keyboard to the window" << std::endl;
	out << "grab_input=" << this->grab_input << std::endl;
	out << std::endl;
	out << "; Fullscreen mouse scaling (0 - match desktop, 1 - match game screen)" << std::endl;
	out << "mouse_scale=" << this->mouse_scale << std::endl;
	out << std::endl;	
	out << "; Physics update time in ms (65ms/15FPS original)" << std::endl;
	out << "physics_update=" << this->physics_update << std::endl;
	out << std::endl;	
	out << "; Bullet time (%)" << std::endl;
	out << "bullet_time=" << (int)(this->bullet_time_add*100) << std::endl;
	out << std::endl;
	out << "local_save=" << this->local_save << std::endl;	
	out << std::endl;
	//
	out << "; PLAYER CONTROLS" << std::endl;
	out << std::endl;
	out << "; Key mappings" << std::endl;
	out << "left=a" << std::endl;
	out << "right=d" << std::endl;
	out << "up=w" << std::endl;
	out << "down=s" << std::endl;
	out << "special=SHIFT_L" << std::endl;
	out << "fire=SPACE" << std::endl;
	out << "fire_2=f" << std::endl;
	out << "weapon_prev=q" << std::endl;
	out << "weapon_next=e" << std::endl;
	out << "special2=CTRL_L" << std::endl;
	out << "; Number keys 1-9 select weapons directly" << std::endl;
	out << std::endl;
	//
	out << "; Alternative key mappings (only the following controls can have two keyboard bindings)" << std::endl;
	out << "left_2=LEFT" << std::endl;
	out << "right_2=RIGHT" << std::endl;
	out << "up_2=UP" << std::endl;
	out << "down_2=DOWN" << std::endl;	
	out << std::endl;
	//
	out << "; CONTROLLER SETTINGS" << std::endl;
	out << std::endl;
	out << "; Enable aiming" << std::endl;
	out << "ctr_aim=" << this->ctr_aim << std::endl;
	out << std::endl;
	out << "; Right stick aim: modern = 60Hz angle latch (controller only; ctr_rst_s unused), classic = 2016 15Hz ring" << std::endl;
	out << "ctr_stick_aim=" << (this->ctr_stick_aim == 0 ? "classic" : "modern") << std::endl;
	out << std::endl;
	out << "; Correct crosshair position (x)" << std::endl;
	out << "ctr_aim_x=" << this->ctr_aim_correctx << std::endl;
	out << std::endl;
	out << "; Crosshair distance from player" << std::endl;
	out << "ctr_cd=" << this->ctr_cd << std::endl;
	out << std::endl;
	out << "; Right stick/aiming sensitivity" << std::endl;
	out << "ctr_rst_s=" << this->ctr_rst_s << std::endl;
	out << std::endl;
	out << "; Right stick/aiming dead zone" << std::endl;
	out << "ctr_rst_dz=" << this->ctr_rst_dz << std::endl;
	out << std::endl;
	out << "; Left stick/movement dead zones" << std::endl;
	out << "ctr_lst_dzx=" << this->ctr_lst_dzx << std::endl;
	out << "ctr_lst_dzy=" << this->ctr_lst_dzy << std::endl;
	out << std::endl;	
	//
	out << "; Button mappings (don't use buttons for left/right movement)" << std::endl;
	out << "up=ctr_a" << std::endl;	
	out << "down=ctr_b" << std::endl;
	out << "special=ctr_left_shoulder" << std::endl;	
	out << "special=ctr_left_stick" << std::endl;
	out << "special2=ctr_left_trigger" << std::endl;
	out << "fire=ctr_right_shoulder" << std::endl;
	out << "fire=ctr_right_trigger" << std::endl;
	out << "fire=ctr_right_stick" << std::endl;
	out << "weapon_prev=ctr_x" << std::endl;
	out << "weapon_next=ctr_y" << std::endl;
	out << "quick_save=ctr_left_stick" << std::endl;
	out << "quick_load=ctr_right_stick" << std::endl;
	
	out.close();

	printf("Default \"config.txt\" created\n");
	
	return true;

	/*
	#if !((defined __APPLE__) || (defined WIN32))
	fputs( "; Location of the datafiles\ndatadir=", fd );
	fputs( ASSETDIR "\n\n", fd );
	#endif	
	*/
}

//////////
////////// READ CONFIG FILE
//////////

bool Settings::ReadConfigFile(std::string folder)
{
	std::string file_path = folder + "config.txt";
	this->config_path = file_path;

	std::ifstream filein(file_path.c_str());
	if(!filein.is_open())
	{
		std::string tmp = "ERROR - ReadConfigFile() - Failed to open \"" + file_path + "\"\n";
		printf(tmp.c_str());

		//try to create it
		return CreateConfigFile(file_path);
	}

	std::string line;
	bool saw_ctr_stick_aim = false;
	while(std::getline(filein,line))
	{
		//stop reading file
		if(line=="exit")
		{
			filein.close();
			return true;
		}

		//skip empty line or ";" which marks a comment
		if(line.empty() || line[0]==';') continue;

		std::string attr, value;

		if(!AR_GetAttr(line,attr,value))
		{
			printf("Config : ignoring bad line \"%s\"\n", line.c_str());
			continue;
		}

		//screen
		if(attr=="fullscreen")			this->fullscreen = AR_ToInt(value);
		else if(attr=="borderless")		this->borderless = AR_ToBool(value);
		else if(attr=="vsync")			this->vsync = AR_ToBool(value);
		else if(attr=="screen_width")	this->xres = AR_ToInt(value);
		else if(attr=="screen_height")	this->yres = AR_ToInt(value);
		else if(attr=="scale")			this->scale = AR_ToInt(value);
		else if(attr=="linear_filter")	this->linear_filter = AR_ToBool(value);
		else if(attr=="hires")			this->hires = AR_ToInt(value);
		else if(attr=="fov")
		{
			std::string v = value;
			for(size_t i = 0; i < v.size(); i++)
				if(v[i] >= 'A' && v[i] <= 'Z')
					v[i] = (char)(v[i] - 'A' + 'a');
			this->classic_fov = (v=="classic" || v=="1") ? 1 : 0;
		}

		//sound
		else if(attr=="mono")			this->mono = AR_ToBool(value);
		else if(attr=="soundfont")
		{
			if(value=="auto" || value=="0")
				this->soundfont = "";
			else
				this->soundfont = value;
		}
		else if(attr=="no_sound")		this->no_sound = AR_ToBool(value);
		else if(attr=="no_music")		this->no_music = AR_ToBool(value);
		else if(attr=="volume_sound")	this->volume_sound = AR_ToInt(value);		
		else if(attr=="volume_music")	this->volume_music = AR_ToInt(value);
		
		//random
		else if(attr=="local_save")		this->local_save = AR_ToBool(value);
		else if(attr=="grab_input")		this->grab_input = AR_ToBool(value);
		else if(attr=="editor")			this->editor = AR_ToBool(value);
		else if(attr=="physics_update")
		{
			this->physics_update = 65;
		}
		else if(attr=="mouse_scale")	this->mouse_scale = AR_ToInt(value);
		else if(attr=="big_font")		this->big_font = AR_ToBool(value);
		else if(attr=="bullet_time")	this->bullet_time_add = AR_ToInt(value)/100.0f;
		
		//player controls
		else if(attr=="up")
		{
			if(!ControllerButton(attr,value))	this->up = key_value(value.c_str());
		}
		else if(attr=="down")
		{
			if(!ControllerButton(attr,value))	this->down = key_value(value.c_str());
		}
		else if(attr=="left")
		{
			if(!ControllerButton(attr,value))	this->left = key_value(value.c_str());
		}
		else if(attr=="right")
		{
			if(!ControllerButton(attr,value))	this->right = key_value(value.c_str());
		}		
		//
		else if(attr=="special")
		{
			if(!ControllerButton(attr,value))	this->b1 = key_value(value.c_str());
		}
		else if(attr=="fire")
		{
			if(!ControllerButton(attr,value))	this->b2 = key_value(value.c_str());
		}
		else if(attr=="fire_2")
		{
			this->b2_2 = key_value(value.c_str());
		}
		else if(attr=="weapon_prev")
		{
			if(!ControllerButton(attr,value))	this->b3 = key_value(value.c_str());
		}
		else if(attr=="weapon_next")
		{
			if(!ControllerButton(attr,value))	this->b4 = key_value(value.c_str());
		}
		else if(attr=="special2")
		{
			if(!ControllerButton(attr,value))	this->bt = key_value(value.c_str());
		}
		//
		else if(attr=="up_2")		this->up_2 = key_value(value.c_str());
		else if(attr=="down_2")		this->down_2 = key_value(value.c_str());
		else if(attr=="left_2")		this->left_2 = key_value(value.c_str());
		else if(attr=="right_2")	this->right_2 = key_value(value.c_str());
		
		//controller settings
		else if(attr=="ctr_aim")		this->ctr_aim = AR_ToBool(value);
		else if(attr=="ctr_stick_aim")
		{
			saw_ctr_stick_aim = true;
			std::string v = value;
			for(size_t i = 0; i < v.size(); i++)
				if(v[i] >= 'A' && v[i] <= 'Z')
					v[i] = (char)(v[i] - 'A' + 'a');
			if(v=="classic" || v=="0" || v=="2016")
				this->ctr_stick_aim = 0;
			else if(v=="modern" || v=="1" || v=="claude")
				this->ctr_stick_aim = 1;
			else
				printf("Stick aim : unknown ctr_stick_aim=%s (using %s)\n",
					value.c_str(), this->ctr_stick_aim == 0 ? "classic" : "modern");
		}
		else if(attr=="ctr_aim_x")		this->ctr_aim_correctx = AR_ToInt(value);
		else if(attr=="ctr_cd")			this->ctr_cd = AR_ToInt(value);
		else if(attr=="ctr_rst_s")		this->ctr_rst_s = AR_ToInt(value);
		else if(attr=="ctr_rst_dz")		this->ctr_rst_dz = AR_ToInt(value);
		else if(attr=="ctr_lst_dzx")	this->ctr_lst_dzx = AR_ToInt(value);
		else if(attr=="ctr_lst_dzy")	this->ctr_lst_dzy = AR_ToInt(value);
		else if(attr=="quick_save" || attr=="quick_load")
		{
			int b = 0;

			if(value=="ctr_a")						b = SDL_CONTROLLER_BUTTON_A;
			else if(value=="ctr_b")					b = SDL_CONTROLLER_BUTTON_B;
			else if(value=="ctr_x")					b = SDL_CONTROLLER_BUTTON_X;
			else if(value=="ctr_y")					b = SDL_CONTROLLER_BUTTON_Y;
			else if(value=="ctr_left_stick")		b = SDL_CONTROLLER_BUTTON_LEFTSTICK;
			else if(value=="ctr_right_stick")		b = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
			else if(value=="ctr_left_shoulder")		b = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
			else if(value=="ctr_right_shoulder")	b = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;

			if(attr=="quick_save") this->ctr_f5 = b;
			else this->ctr_f9 = b;
		}		
		else
		{
			printf("Config : ignoring unknown \"%s\"\n", line.c_str());
		}
	}

	filein.close();

	if(!saw_ctr_stick_aim)
	{
		std::ofstream out(file_path.c_str(), std::ios::app);
		if(out.is_open())
		{
			out << std::endl;
			out << "; Right stick aim: modern = 60Hz angle latch (controller only; ctr_rst_s unused), classic = 2016 15Hz ring" << std::endl;
			out << "ctr_stick_aim=" << (this->ctr_stick_aim == 0 ? "classic" : "modern") << std::endl;
		}
	}

	printf("Stick aim : %s (F8 toggles controller aim; set ctr_stick_aim=classic to revert)\n",
		this->ctr_stick_aim == 0 ? "classic" : "modern");

	return true;

	/*
	if( strcasecmp( result, "datadir" ) == 0 )
	{
	result = strtok( NULL, "\n" );
	set_filename_prefix( result );
	}
	*/
}

static std::string ar_cvar_lower(std::string s)
{
	for(size_t i = 0; i < s.size(); i++)
		if(s[i] >= 'A' && s[i] <= 'Z')
			s[i] = (char)(s[i] - 'A' + 'a');
	return s;
}

bool Settings::GetCvar(std::string key, std::string &value)
{
	key = ar_cvar_lower(key);
	char buf[128];
	if(key=="fullscreen")		{ snprintf(buf, sizeof(buf), "%d", fullscreen); value = buf; return true; }
	if(key=="borderless")		{ value = borderless ? "1" : "0"; return true; }
	if(key=="vsync")			{ value = vsync ? "1" : "0"; return true; }
	if(key=="screen_width")		{ snprintf(buf, sizeof(buf), "%d", (int)xres); value = buf; return true; }
	if(key=="screen_height")	{ snprintf(buf, sizeof(buf), "%d", (int)yres); value = buf; return true; }
	if(key=="scale")			{ snprintf(buf, sizeof(buf), "%d", (int)scale); value = buf; return true; }
	if(key=="linear_filter")	{ value = linear_filter ? "1" : "0"; return true; }
	if(key=="hires")			{ snprintf(buf, sizeof(buf), "%d", hires); value = buf; return true; }
	if(key=="fov")				{ value = classic_fov ? "classic" : "wide"; return true; }
	if(key=="mono")				{ value = mono ? "1" : "0"; return true; }
	if(key=="no_sound")			{ value = no_sound ? "1" : "0"; return true; }
	if(key=="no_music")			{ value = no_music ? "1" : "0"; return true; }
	if(key=="volume_sound")		{ snprintf(buf, sizeof(buf), "%d", volume_sound); value = buf; return true; }
	if(key=="volume_music")		{ snprintf(buf, sizeof(buf), "%d", volume_music); value = buf; return true; }
	if(key=="soundfont")		{ value = soundfont.empty() ? (sound_current_font()[0] ? sound_current_font() : "auto") : soundfont; return true; }
	if(key=="local_save")		{ value = local_save ? "1" : "0"; return true; }
	if(key=="grab_input")		{ value = grab_input ? "1" : "0"; return true; }
	if(key=="editor")			{ value = editor ? "1" : "0"; return true; }
	if(key=="physics_update")	{ snprintf(buf, sizeof(buf), "%d", (int)physics_update); value = buf; return true; }
	if(key=="mouse_scale")		{ snprintf(buf, sizeof(buf), "%d", (int)mouse_scale); value = buf; return true; }
	if(key=="big_font")			{ value = big_font ? "1" : "0"; return true; }
	if(key=="ctr_aim")			{ value = ctr_aim ? "1" : "0"; return true; }
	if(key=="ctr_stick_aim")	{ value = ctr_stick_aim == 0 ? "classic" : "modern"; return true; }
	if(key=="ctr_aim_x")		{ snprintf(buf, sizeof(buf), "%d", ctr_aim_correctx); value = buf; return true; }
	if(key=="ctr_cd")			{ snprintf(buf, sizeof(buf), "%d", ctr_cd); value = buf; return true; }
	if(key=="ctr_rst_s")		{ snprintf(buf, sizeof(buf), "%d", ctr_rst_s); value = buf; return true; }
	if(key=="ctr_rst_dz")		{ snprintf(buf, sizeof(buf), "%d", ctr_rst_dz); value = buf; return true; }
	if(key=="ctr_lst_dzx")		{ snprintf(buf, sizeof(buf), "%d", ctr_lst_dzx); value = buf; return true; }
	if(key=="ctr_lst_dzy")		{ snprintf(buf, sizeof(buf), "%d", ctr_lst_dzy); value = buf; return true; }
	return false;
}

bool Settings::SetCvar(std::string key, std::string value, std::string &reply)
{
	key = ar_cvar_lower(key);
	std::string v = ar_cvar_lower(value);

	if(key=="physics_update")
	{
		reply = "physics_update locked (15Hz)";
		return true;
	}
	if(key=="screen_width" || key=="screen_height" || key=="hires" || key=="big_font" || key=="vsync" || key=="fullscreen")
	{
		if(key=="screen_width") xres = (short)AR_ToInt(value);
		else if(key=="screen_height") yres = (short)AR_ToInt(value);
		else if(key=="hires") hires = AR_ToInt(value);
		else if(key=="big_font") big_font = AR_ToBool(value);
		else if(key=="vsync") vsync = AR_ToBool(value);
		else fullscreen = AR_ToInt(value);
		reply = key + " set, restart";
		return true;
	}
	if(key=="fov")
	{
		if(v.empty() || v=="toggle")
			classic_fov = classic_fov ? 0 : 1;
		else if(v=="classic" || v=="1")
			classic_fov = 1;
		else if(v=="wide" || v=="0")
			classic_fov = 0;
		else
		{
			reply = "fov classic|wide";
			return true;
		}
		apply_game_fov();
		reply = classic_fov ? "fov classic" : "fov wide";
		return true;
	}
	if(key=="ctr_aim")
	{
		ctr_aim = AR_ToBool(value);
		reset_stick_aim();
		if(!ctr_aim)
			release_controller_move_keys();
		reply = ctr_aim ? "ctr_aim=1" : "ctr_aim=0";
		return true;
	}
	if(key=="ctr_stick_aim")
	{
		if(v=="classic" || v=="0" || v=="2016")
			ctr_stick_aim = 0;
		else if(v=="modern" || v=="1")
			ctr_stick_aim = 1;
		else
		{
			reply = "ctr_stick_aim classic|modern";
			return true;
		}
		ctr_aim_latched = false;
		reply = ctr_stick_aim == 0 ? "stick classic" : "stick modern";
		return true;
	}
	if(key=="ctr_cd")
	{
		ctr_cd = AR_ToInt(value);
		if(ctr_cd<20) ctr_cd=20;
		if(ctr_cd>200) ctr_cd=200;
		char b[32];
		snprintf(b, sizeof(b), "ctr_cd=%d", ctr_cd);
		reply = b;
		return true;
	}
	if(key=="ctr_rst_s") { ctr_rst_s = AR_ToInt(value); reply = value; return true; }
	if(key=="ctr_rst_dz") { ctr_rst_dz = AR_ToInt(value); reply = "ctr_rst_dz set"; return true; }
	if(key=="ctr_lst_dzx") { ctr_lst_dzx = AR_ToInt(value); reply = "ctr_lst_dzx set"; return true; }
	if(key=="ctr_lst_dzy") { ctr_lst_dzy = AR_ToInt(value); reply = "ctr_lst_dzy set"; return true; }
	if(key=="ctr_aim_x") { ctr_aim_correctx = AR_ToInt(value); reply = "ctr_aim_x set"; return true; }
	if(key=="linear_filter") { video_set_linear_filter(AR_ToBool(value)); reply = linear_filter ? "filter linear" : "filter nearest"; return true; }
	if(key=="grab_input") { video_set_grab(AR_ToBool(value)); reply = grab_input ? "grab on" : "grab off"; return true; }
	if(key=="volume_sound")
	{
		volume_sound = AR_ToInt(value);
		if(volume_sound<0) volume_sound=0;
		if(volume_sound>127) volume_sound=127;
		sfx_volume = volume_sound;
		Mix_Volume(-1, volume_sound);
		reply = "volume_sound set";
		return true;
	}
	if(key=="volume_music")
	{
		volume_music = AR_ToInt(value);
		if(volume_music<0) volume_music=0;
		if(volume_music>127) volume_music=127;
		music_volume = volume_music;
		Mix_VolumeMusic(volume_music);
		reply = "volume_music set";
		return true;
	}
	if(key=="no_music") { no_music = AR_ToBool(value); reply = no_music ? "no_music=1" : "no_music=0"; return true; }
	if(key=="no_sound") { no_sound = AR_ToBool(value); reply = no_sound ? "no_sound=1" : "no_sound=0"; return true; }
	if(key=="mono") { mono = AR_ToBool(value); reply = "mono set, restart"; return true; }
	if(key=="borderless") { borderless = AR_ToBool(value); reply = "borderless set, restart"; return true; }
	if(key=="scale") { scale = (short)AR_ToInt(value); reply = "scale set, use F11/F12"; return true; }
	if(key=="mouse_scale") { mouse_scale = (short)AR_ToInt(value); reply = "mouse_scale set"; return true; }
	if(key=="editor") { editor = AR_ToBool(value); reply = "editor set, restart"; return true; }
	if(key=="local_save") { local_save = AR_ToBool(value); reply = "local_save set, restart"; return true; }
	if(key=="soundfont")
	{
		if(v=="auto")
		{
			soundfont = "";
			reply = "soundfont auto, restart";
			return true;
		}
		if(!sound_set_font(value.c_str()))
		{
			reply = "soundfont not found";
			return true;
		}
		soundfont = value;
		reply = "soundfont next track";
		return true;
	}
	reply = "unknown cvar";
	return false;
}

bool Settings::WriteConfigFile()
{
	if(config_path.empty())
		return false;

	std::ifstream in(config_path.c_str());
	std::vector<std::string> lines;
	if(in.is_open())
	{
		std::string line;
		while(std::getline(in, line))
			lines.push_back(line);
		in.close();
	}

	const char *keys[] = {
		"fov", "soundfont", "ctr_aim", "ctr_stick_aim", "ctr_cd",
		"ctr_rst_s", "ctr_rst_dz", "ctr_lst_dzx", "ctr_lst_dzy",
		"volume_sound", "volume_music", "linear_filter", "grab_input",
		"no_music", "no_sound", "scale", "mouse_scale",
		"screen_width", "screen_height", "hires", "vsync", "fullscreen",
		"big_font", "borderless",
		NULL
	};

	for(int k = 0; keys[k]; k++)
	{
		std::string val;
		if(!GetCvar(keys[k], val))
			continue;
		std::string prefix = std::string(keys[k]) + "=";
		bool found = false;
		for(size_t i = 0; i < lines.size(); i++)
		{
			if(lines[i].compare(0, prefix.size(), prefix) == 0)
			{
				lines[i] = prefix + val;
				found = true;
				break;
			}
		}
		if(!found)
			lines.push_back(prefix + val);
	}

	std::ofstream out(config_path.c_str());
	if(!out.is_open())
		return false;
	for(size_t i = 0; i < lines.size(); i++)
		out << lines[i] << std::endl;
	return true;
}

bool Settings::ControllerButton(std::string c, std::string b)
{
	std::string control = c;

	if(c=="special")			control = "b1";
	else if(c=="fire")			control = "b2";
	else if(c=="weapon_prev")	control = "b3";
	else if(c=="weapon_next")	control = "b4";
	else if(c=="special2")		control = "bt";

	if(b=="ctr_a") {this->ctr_a = control;return true;};
	if(b=="ctr_b") {this->ctr_b = control;return true;};
	if(b=="ctr_x") {this->ctr_x = control;return true;};
	if(b=="ctr_y") {this->ctr_y = control;return true;};
	//
	if(b=="ctr_left_stick")		{this->ctr_lst = control;return true;};
	if(b=="ctr_right_stick")	{this->ctr_rst = control;return true;};
	//
	if(b=="ctr_left_shoulder")	{this->ctr_lsr = control;return true;};
	if(b=="ctr_right_shoulder")	{this->ctr_rsr = control;return true;};
	//
	if(b=="ctr_left_trigger")	{this->ctr_ltg = control;return true;};
	if(b=="ctr_right_trigger")	{this->ctr_rtg = control;return true;};

	return false;
}

//
// Display help
//
void showHelp(const char* executableName)
{
    printf( "\n" );
    printf( "Usage: %s [options]\n", executableName );
    printf( "Options:\n\n" );
    printf( "** Abuse Options **\n" );
    printf( "  -size <arg>       Set the size of the screen\n" );
    printf( "  -edit             Startup in editor mode\n" );
    printf( "  -a <arg>          Use addon named <arg>\n" );
    printf( "  -f <arg>          Load map file named <arg>\n" );
    printf( "  -lisp             Startup in lisp interpreter mode\n" );
    printf( "  -nodelay          Run at maximum speed\n" );
    printf( "\n" );
    printf( "** Abuse-SDL Options **\n" );
    printf( "  -datadir <arg>    Set the location of the game data to <arg>\n" );
    printf( "  -fullscreen       Enable fullscreen mode\n" );
    printf( "  -antialias        Enable anti-aliasing\n" );
    printf( "  -h, --help        Display this text\n" );
    printf( "  -mono             Disable stereo sound\n" );
    printf( "  -nosound          Disable sound\n" );
    printf( "  -scale <arg>      Scale to <arg>\n" );
//    printf( "  -x <arg>          Set the width to <arg>\n" );
//    printf( "  -y <arg>          Set the height to <arg>\n" );
    printf( "\n" );
    printf( "Anthony Kruize <trandor@labyrinth.net.au>\n" );
    printf( "\n" );
}

//
// Parse the command-line parameters
//
void parseCommandLine(int argc, char **argv)
{
	//AR this is called before settings.ReadConfigFile(), so I can override stuff via console

	for(int i=1;i<argc;i++)
	{
		if(!strcasecmp(argv[i],"-remote_save"))
		{
			settings.local_save = false;
		}
	}
}

//
// Setup SDL and configuration
//
void setup( int argc, char **argv )
{
	// Display our name and version
    //printf( "%s %s\n", PACKAGE_NAME, PACKAGE_VERSION );

	//AR
	printf( "%s %s\n", "Abuse", "0.9a" );

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
#ifdef SDL_HINT_JOYSTICK_HIDAPI_PS4
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
#endif
#ifdef SDL_HINT_JOYSTICK_HIDAPI_PS5
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
#endif
#ifdef SDL_HINT_JOYSTICK_HIDAPI_XBOX
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "1");
#endif
#ifdef SDL_HINT_JOYSTICK_THREAD
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
#endif
#ifdef SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
#endif

    // Initialize SDL with video and audio support
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0 )
    {
        show_startup_error( "Unable to initialize SDL : %s\n", SDL_GetError() );
        exit( 1 );
    }
    atexit( SDL_Quit );

    // Set the savegame directory
    char *homedir;
    char *savedir;
    FILE *fd = NULL;

#ifdef WIN32
    // Grab the profile dir
    PWSTR appData;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appData);
    // Create a new chunk of memory to save the savedir in
    size_t savedir_size = lstrlenW(appData) * 2 + 7;
    savedir = (char*) malloc(savedir_size);
    wcstombs(savedir, appData, savedir_size);
    // Append "\Abuse\" to end end of it
    strcat(savedir, "\\Abuse\\");
    // If it doesn't exist, create it
    if ( (fd = fopen(savedir, "r")) == NULL) {
        // FIXME: Add some error checking here
        _mkdir(savedir);
    } else {
        fclose( fd );
    }
    set_save_filename_prefix(savedir);
    CoTaskMemFree(appData);
    free( savedir );
#else
    if( (homedir = getenv( "HOME" )) != NULL )
    {
        savedir = (char *)malloc( strlen( homedir ) + 9 );
        sprintf( savedir, "%s/.abuse/", homedir );
        // Check if we already have a savegame directory
        if( (fd = fopen( savedir, "r" )) == NULL )
        {
            // FIXME: Add some error checking here
            mkdir( savedir, S_IRUSR | S_IWUSR | S_IXUSR );
        }
        else
        {
            fclose( fd );
        }
        set_save_filename_prefix( savedir );
        free( savedir );
    }
    else
    {
        // Warn the user that we couldn't set the savename prefix
        printf( "WARNING: Unable to get $HOME environment variable.\n" );
        printf( "         Savegames will probably fail.\n" );
        // Just use the working directory.
        // Hopefully they have write permissions....
        set_save_filename_prefix( "" );
    }
#endif

    // Set the datadir to a default value
    // (The current directory)
#ifdef __APPLE__
    UInt8 buffer[255];
    CFURLRef bundleurl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFURLRef url = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, bundleurl, CFSTR("Contents/Resources/data"), true);

    if (!CFURLGetFileSystemRepresentation(url, true, buffer, 255))
    {
        exit(1);
    }
    else
    {
        printf("Setting prefix to [%s]\n", buffer);
        set_filename_prefix( (const char*)buffer );
    }
#elif defined WIN32
    // Under Windows, it makes far more sense to assume the data is stored
    // relative to our executable than anywhere else.
    char assetDirName[MAX_PATH];
    GetModuleFileName(NULL, assetDirName, MAX_PATH);
    // Find the first \ or / and cut the path there
    size_t cut_at = -1;
    for (size_t i = 0; assetDirName[i] != '\0'; i++) {
        if (assetDirName[i] == '\\' || assetDirName[i] == '/') {
            cut_at = i;
        }
    }
    if (cut_at >= 0)
        assetDirName[cut_at] = '\0';
    printf("Setting data dir to %s\n", assetDirName);
    set_filename_prefix( assetDirName );
#else
    set_filename_prefix( ASSETDIR );
#endif

	//handle command-line parameters
    parseCommandLine(argc,argv);

	//AR override save game directory to local directory
#ifdef __APPLE__
    const char *sdlPath = SDL_GetPrefPath("abuse", "user");
    if(settings.local_save && sdlPath)
        set_save_filename_prefix(sdlPath);
    else if(settings.local_save)
        set_save_filename_prefix("user/");
#else
	if(settings.local_save) set_save_filename_prefix("user/");
#endif

	printf("Setting save dir to %s\n", get_save_filename_prefix());

    // Load the users configuration
    settings.ReadConfigFile(get_save_filename_prefix());

	// Initialize default settings
	scale = settings.scale;
	xres = settings.xres;
	yres = settings.yres;
	sfx_volume = settings.volume_sound;
	music_volume = settings.volume_music;
}

//
// Get the key binding for the requested function
//
int get_key_binding(char const *dir, int i)
{
	if(strcasecmp(dir,"left")==0)			return settings.left;
	else if(strcasecmp(dir,"right")==0)		return settings.right;
	else if(strcasecmp(dir,"up")==0)		return settings.up;
	else if(strcasecmp(dir,"down")==0)		return settings.down;
	else if(strcasecmp(dir,"left2")==0)		return settings.left_2;
	else if(strcasecmp(dir,"right2")==0)	return settings.right_2;
	else if(strcasecmp(dir,"up2")==0)		return settings.up_2;
	else if(strcasecmp(dir,"down2")==0)		return settings.down_2;
	else if(strcasecmp(dir,"b1")==0)		return settings.b1;
	else if(strcasecmp(dir,"b2")==0)		return settings.b2;
	else if(strcasecmp(dir,"b2_2")==0)	return settings.b2_2;
	else if(strcasecmp(dir,"b3")==0)		return settings.b3;
	else if(strcasecmp(dir,"b4")==0)		return settings.b4;
	else if(strcasecmp(dir,"bt")==0)		return settings.bt;

	return 0;
}

//AR controller
std::string get_ctr_binding(std::string c)
{
    if(c=="ctr_a")			return settings.ctr_a;
	else if(c=="ctr_b")		return settings.ctr_b;
	else if(c=="ctr_x")		return settings.ctr_x;
	else if(c=="ctr_y")		return settings.ctr_y;
	//
	else if(c=="ctr_lst")	return settings.ctr_lst;
	else if(c=="ctr_rst")	return settings.ctr_rst;
	//
	else if(c=="ctr_lsr")	return settings.ctr_lsr;
	else if(c=="ctr_rsh")	return settings.ctr_rsr;
	//
	else if(c=="ctr_ltg")	return settings.ctr_ltg;
	else if(c=="ctr_rtg")	return settings.ctr_rtg;
	
	return "";
}
