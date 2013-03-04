/* gameplaySP - raspberry backend
 *
 * Copyright (C) 2013 DPR <pribyl.email@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "../common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "gles_video.h"
#include "rpi.h"
#include "bcm_host.h"

u32 gamepad_config_map[PLAT_BUTTON_COUNT] =
{
  BUTTON_ID_UP,                 // Up
  BUTTON_ID_LEFT,               // Left
  BUTTON_ID_DOWN,               // Down
  BUTTON_ID_RIGHT,              // Right
  BUTTON_ID_START,              // Start
  BUTTON_ID_SELECT,             // Select
  BUTTON_ID_L,                  // Ltrigger
  BUTTON_ID_R,                  // Rtrigger
  BUTTON_ID_FPS,                // A
  BUTTON_ID_A,                  // B
  BUTTON_ID_B,                  // X
  BUTTON_ID_MENU,               // Y
  BUTTON_ID_SAVESTATE,          // 1
  BUTTON_ID_LOADSTATE,          // 2
  BUTTON_ID_FASTFORWARD,        // 3
  BUTTON_ID_NONE,               // 4
  BUTTON_ID_MENU                // Space
};


#define MAX_VIDEO_MEM (480*270*2)
static int video_started=0;
static uint16_t * video_buff;


void gpsp_plat_init(void)
{
  int ret, w, h, fd;
  //const char *layer_fb_name;
  SDL_Surface* myVideoSurface;

  bcm_host_init();

  ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);
  if (ret != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    exit(1);
  }

  myVideoSurface = SDL_SetVideoMode( 0, 0, 16,  SDL_SWSURFACE);
    // Print out some information about the video surface
    if (myVideoSurface == NULL) {
	fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
	exit(1);
    }
  SDL_ShowCursor(0);
  fb_set_mode(240, 160, 0, 0, 0, 0);
  screen_scale = 3;
}

void gpsp_plat_quit(void)
{
    if (video_started) {
	video_close();
	free(video_buff);
       video_started=0;
    }
  SDL_Quit();
}


void *fb_flip_screen(void)
{
  video_draw(video_buff);
  return video_buff;
}

void fb_wait_vsync(void)
{
}

void fb_set_mode(int w, int h, int buffers, int scale,int filter, int filter2)
{
    if (video_started) {
	video_close();
	free(video_buff);
    }
    video_buff=malloc(w*h*sizeof(uint16_t));
    memset(video_buff,0,w*h*sizeof(uint16_t));
    video_init(w,h,filter);
    video_started=1;
}
// vim:shiftwidth=2:expandtab
