Abuse README
============

This is the **2026 Apple Silicon** fork (`augmented-tech/Abuse_1996`) of
Antonio Radojkovic's SDL2 port (https://github.com/antrad/Abuse_1996),
which comes from Xenoveritas (http://github.com/Xenoveritas/abuse) and
the SDL port at http://abuse.zoy.org.

It is a native arm64 Mac build of the 1996 Crack dot Com game. Physics stay
at 15 Hz.

--------

1. Introduction
2. Additional features
3. Configuration
4. Hardcoded keys
5. Building the project
6. Notes
7. Abuse tool
8. Special thanks
9. Feedback
10. Links

--------

## 1. INTRODUCTION

###### The story

"You are Nick Vrenna. It is the year 2009. You have been falsely incarcerated
inside a high security underground prison where illegal genetic experiments
are taking place.

Alan Blake, the head research scientist, has isolated the specific gene which
causes violence and aggression in humans. This genetic sequence, called
"Abuse", is highly infectious, causing horrific transformations and grotesque
side-effects. You are the only person to show immunity to it.

A prison riot erupts, and in the confusion all the cell doors are opened. Soon
everyone, guards and convicts alike, become infected and transform into a
horde of mutants which take over the building.

Your only chance for escape is to don battle armor and reach the Control Room
situated in the structure's deepest level. You must first stop the prison's
Abuse-infected water supply from contaminating the outside world. Freedom and
the fate of the world now depend on you."

###### Abuse name

"We chose Abuse for our first game name because the game involved knowing full well that pressing the
buttons in all those rooms was going to bring down hoards of howlers, but you would do it anyway, abusing yourself."

## 2. ADDITIONAL FEATURES

### This fork (2026, Apple Silicon)

Compared to antrad's 2016 SDL2 port:

  * Native **arm64** Mac build; SDL2 presents through **Metal** (OpenGL/GLee removed)
  * **15 Hz physics unchanged**; interpolated drawing so walk and aim look smooth on Retina
  * Live **FOV**: `fov classic` (320×200, original secrets), `340`, `360`, or `wide`
  * Save-station **slot grid fully visible** in classic FOV
  * Console **`c`** grabs typing (keys no longer fire Nick); `get` / `set` / `save` for `config.txt` cvars
  * **Xbox / DualSense**: move and jump; stick aim `classic` (2016 15 Hz ring) or `modern` (60 Hz latch)
  * **F8** toggles controller aim without killing walk
  * **WASD**, Space to fire, 1–7 weapons, plus original mouse aim
  * **MIDI music** via a SoundFont (place `soundfont.sf2` in `~/Library/Application Support/abuse/user/`, e.g. FluidR3 GM)
  * F10 fullscreen, F11/F12 scale, vsync
  * Original PC text hints; Mac tutorial voice WAVs stay off

On macOS, `config.txt` and saves live in `~/Library/Application Support/abuse/user/`.

### antrad 2016 (from Xenoveritas)

These are the changes compared to Xenoveritas SDL2 version from 2014:

  * Enabled custom resolutions and enabled lights on high resolutions
  * Re-enabled OpenGL rendering to enable vsync
  * Game screen scaling in window and fullscreen mode using F11 and F12
  * Enabled some high resolution images from the 1997 Mac OS release
  * Fixed level music not being played correctly, added "victory" music in the end game screen
  * Fixed health power image, fixed mouse image when choosing initial gamma
  * Added or re-enabled various settings in the config file (borderless window, grab input, editor mode, high resolution images...)
  * Local save game files and configuration files
  * Quick load using F9, quick save using F5 on save consoles
  * Added cheats via chat console: bullettime, god, giveall, flypower, sneakypower, fastpower, healthpower, nopower
  * XBox360 controller support with rebindable buttons
  * Updated abuse-tool so it can extract the images in Abuse SPEC files to modern image formats as individual images, tilemaps or a texture atlas with information about image, tile and animation frame sizes and positions

## 3. CONFIGURATION

Abuse configuration file has been updated in this version. On macOS it is
`~/Library/Application Support/abuse/user/config.txt` (saves and gamma live
there too). Elsewhere it is the local "user" folder as "config.txt".

Useful cvars (console `c`, then `set <name> <value>` and `save`):

- fov - classic | 340 | 360 | wide
- ctr_stick_aim - classic | modern
- soundfont - auto, or a path to a .sf2 file

Lines starting with a ';' are comments. Setting an option to '1' turns it on, and '0' turns it off.
Following settings can be changed via the config file:

- fullscreen - fullscreen or window mode (2 - "real" fullscreen)
- vsync - vertical sync
- screen_width - game screen width
- screen_height - game screen height
- scale - window scale
- linear_filter - use linear texture filter (nearest is default)
- hires - enable high resolution menu buttons and game screens (2 - enable Bungie logo)
- big_font - enable big font
- volume_sound - sound volume
- volume_music - music volume
- mono - use mono audio only
- no_music - disable music
- no_sound - disable sound effects
- local_save - save config and other files locally
- grab_mouse - grab the mouse to the window
- editor - enable editor mode
- physics_update - physics update time in ms
- mouse_scale - mouse to game scaling based on desktop or game screen size

To change the keys used in the game, simply type the key after the option:
- left - move left
- right - move right
- up - jump, climb ladder
- down - use lift, press switch, use save console
- special - use special ability
- special2 - use bullet time cheat (not in the original version)
- fire - fire weapon
- weapon_prev - select previous available weapon
- weapon_next - select next available weapon

The following special keys can also be used:

| Code                          | Represents
|-------------------------------|-----------------------
| `LEFT`, `RIGHT`, `UP`, `DOWN` | Cursor keys and keypad.
| `CTRL_L`, `CTRL_R`            | Left and right Ctrl keys.
| `ALT_L`, `ALT_R`              | Left and right Alt keys.
| `SHIFT_L`, `SHIFT_R`          | Left and right Shift keys.
| `F1` - `F10`                  | Function keys 1 through 10.
| `TAB`                         | Tab key.
| `BACKSPACE`                   | Backspace key.
| `ENTER`                       | Enter key
| `INSERT`, `DEL`               | Insert and Delete keys.
| `PAGEUP`, `PAGEDOWN`          | Page Up and Page Down keys.
| `CAPS`, `NUM_LOCK`            | Caps-Lock and Num-Lock keys.
| `SPACE`                       | Spacebar.

The default key settings are as follows:

| Action      | Bound to
|-------------|---------
| Left        | Left arrow, A
| Right       | Right arrow, D
| Up/Jump     | Up arrow, W
| Down/Use    | Down arrow, S
| Prev Weapon | Left or Right Ctrl
| Next Weapon | Insert

The mouse controls your aim, with Left button for fire and Right button for special.
The mouse wheel can be used for changing weapons.

The game has almost full controller support now, there are several settings you can change in the config file:
- ctr_aim - enable aiming with the right stick
- ctr_cd - crosshair distance from player
- ctr_rst_s - right stick/aiming sensitivity
- ctr_rst_dz - right stick/aiming dead zone
- ctr_lst_dzx - left stick left/right movement dead zones
- ctr_lst_dzy - left stick up/down movement dead zones

To bind controller buttons to in game action use the following names for the buttons;
- ctr_a, ctr_b, ctr_x, ctr_y
- ctr_left_shoulder, ctr_right_shoulder
- ctr_left_trigger, ctr_right_trigger
- ctr_left_stick, ctr_right_stick

See "5. Hardcoded keys" for the hardcoded controller bindings.

## 4. Hardcoded keys

There are several keys in the game that are hardcoded to some function originally or were added during porting:

- 1-7 - weapon selection
- right control - previous weapon
- insert - next weapon
- numpad 2, 4, 5, 6, 8 - player movement 
- escape, space, enter - reset level on death
- c - cheat/chat console
- p - pause game
- F1 - show help/controls screen
- F5 - quick save on save consoles (1 or "save0001.spe" is the dedicated quick save slot)
- F6 - toggle window input grab
- F7 - toggle mouse scale type
- F8 - toggle controller aim (walk/jump stay on)
- F9 - quick load
- F10 - toggle window/fullscreen mode
- F11 - scale window/screen up
- F12 - scale window/screen down
- print screen - take a screenshot

Controller defaults:

- D-pad, left stick - move left, right, up, down in game and in menus
- home - show help/controls screen
- back - behaves like the escape key
- start - behaves like enter key

## 5. BUILDING THE PROJECT

This fork (Apple Silicon):

  * CMake 3.16 or above
  * SDL2 2.0.12 or above (Metal renderer)
  * SDL_mixer 2.0.0 or above (FluidSynth for HMI/MIDI)
  * No GLee / OpenGL

    cmake -S . -B build && cmake --build build --target abuse

OpenCV is only needed for abuse-tool. See BUILDING.md for the older SDL/OpenGL notes; they do not apply to this Mac build.

## 6. NOTES

###### Increased difficulty:
	The game is designed to be played in 320x200 resolution. Playing in higher resolution reveals some secreat areas. Enemies get triggered sooner,
	so you may end up fighting more aliens then it was intended. Since in higher resolution you can see a larger area and it makes the game easier,
	you can think of it as a "difficulty readjustment" feature and not a bug.
	
###### Recommended game screen resolutions:
	 To scale the game properly on 1920x1080 screens you can use these values:
		- 320x180 with scale set to 6
		- 384x216 with scale set to 5
		- 480x270 with scale set to 4
		- 640x360 with scale set to 3
		- 960x540 with scale set to 2

###### Using cheat/chat console:
	The mouse cursor must be inside the console window to be able to get input, because windows in the engine are only active then.
	Press enter/return when the console is clear, or type "quit" or "exit" to close the console window.

###### Quick save/load:
	Save slot 1 or "save0001.spe" is the dedicated quick save slot. Quick load always loads the last save game, so if you start a new level or
	manually save the game after you used the quick save, it will load that and not the quick save.
	Some in game save consoles act as a switch to open doors, so if you just quick save the door will not open, you need to press the down/use key.

###### Sound and music
	Music volume lowers the sound effects volume too, it is probably an issue with the sound library. Also, sometimes when you disable the music
	in the config file, the sound will not play too. You re-enable the music, start the game, then quit and disable it again and launch the game.
	I have no idea why this happens and I checked everything, must be the library.

###### Low rendering speed:
	If I understand the rendering process correctly, everything is first rendered to a in game image format by copying bytes of data from the image buffers.
	Then those are again copied to SDL_Surface, then SDL_BlitSurface is called to convert the pixel values to OpenGL format. The resulting SDL_Surface pixel buffer
	is then used to set the pixels of the OpenGL texture using glTexImage2D. Just then we finally render to the backbuffer and update the screen.
	There must be a way to skip some of these steps, you can check out "sdlport/video.cpp" in my Github repository if you have an idea how.

###### 15 FPS:
	The game is designed to run at 15 FPS, higher or lower framerate speeds up or slows down the game. I haven't figured out how physics, animations and
	other time based stuff work, to make the game world update normally at higer framerate. While the game is rendered at higher framerate,
	I have set up a timer that limits the game physics update speed with a default value of 65 ms(15 FPS).

## 7. ABUSE TOOL

Abuse-tool is a low-level tool to edit Abuse SPEC (.spe) files and to extract PSX image files contained in them to modern image formats
as individual images, tilemaps or a texture atlas.

Usage 1: abuse-tool \<spec_file\> \<command\> [args...]

Commands and arguments:
- list - list the contents of a SPEC file
- get     \<id\> - dump entry \<id\> to stdout
- getpcx  \<id\> - dump PCX image \<id\> to "abuse-tool" folder
- put     \<id\> \<type\> \<name\> - insert file \<name\> of type \<type\> at position \<id\>
- putpcx  \<id\> \<type\> \<name\> - insert PCX image \<name\> of type \<type\> at position \<id\>
- rename  \<id\> \<name\> - rename entry \<id\> to \<name\>
- type    \<id\> \<type\> - set entry \<id\> type to \<type\>
- move    \<id1\> \<id2\> - move entry \<id1\> to position \<id2\>
- del     \<id\> - delete entry \<id\>

Usage 2: abuse-tool

To extract the images to modern image formats the program reads the settings and list of files from "abuse-tool/extract.txt"
when being launched. More info can be found there. Extracted images will be stored at the location of the original files.

## 8. SPECIAL THANKS

To everybody who worked on Abuse and its ports; from the people at Crack Dot Com who made the original game,
to Xenoveritas and others who kept it alive for 20 years.

## 9. FEEDBACK

This 2026 Apple Silicon fork: https://github.com/augmented-tech/Abuse_1996

Antonio's 2016 port:  
email: antonior.software@gmail.com  
blog: http://antonior-software.blogspot.hr

## 10. LINKS

###### Info about the game
[Moby Games page] (http://www.mobygames.com/game/abuse)  
[Abuse homepage] (http://web.archive.org/web/20010517011228/http://abuse2.com)  
[Free Abuse(Frabs) homepage] (http://web.archive.org/web/20010124070000/http://www.cs.uidaho.edu/~cass0664/fRaBs)  
[Abuse fan page] (http://web.archive.org/web/19970701080256/http://games.3dreview.com/abuse/index.html)  

###### Downloads
[Abuse Desura download] (http://www.desura.com/games/abuse/download)  
[Free Abuse(Frabs) download] (http://www.dosgames.com/g_act.php)  

###### Source code releases
[Original source code] (https://archive.org/details/abuse_sourcecode)  
[Anthony Kruize Abuse SDL port (2001)] (http://web.archive.org/web/20070205093016/http://www.labyrinth.net.au/~trandor/abuse)  
[Jeremy Scott Windows port (2001)] (http://web.archive.org/web/20051023123223/http://www.webpages.uidaho.edu/~scot4875)  
[Sam Hocevar Abuse SDl port (2011)] (http://abuse.zoy.org)  
[Xenoveritas SDL2 port (2014)] (http://github.com/Xenoveritas/abuse)  

###### Bonus
[Gameplay video] (http://www.youtube.com/watch?v=0Q0SbdDfnFI)  
[HMI to MIDI converter] (http://www.ttdpatch.net/midi/games.html)  

--------

Thank you for playing Abuse!

--------

Abuse, Copyright 1995 Crack dot Com
