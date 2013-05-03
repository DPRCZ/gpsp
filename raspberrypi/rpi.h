/*
 * gpSP - Raspberry Pi port
 *
 * Copyright (C) 2013 DPR <pribyl.email@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#define TRESHOLD 3200
void gpsp_plat_init(void);
void gpsp_plat_quit(void);

#define PLAT_BUTTON_COUNT 10
#define PLAT_KEY_COUNT    10

#define PLAT_MENU_BUTTON -1 // have one hardcoded
//extern u32 button_plat_mask_to_config[PLAT_BUTTON_COUNT];

void *fb_flip_screen(void);
void fb_set_mode(int w, int h, int buffers, int scale, int filter, int filter2);
void fb_wait_vsync(void);

enum {
  JOY_ASIX_YM,
  JOY_ASIX_YP,
  JOY_ASIX_XM,
  JOY_ASIX_XP,
  JOY_BUTTON_1,
  JOY_BUTTON_2,
  JOY_BUTTON_3,
  JOY_BUTTON_4,
  JOY_BUTTON_5,
  JOY_BUTTON_6,
  JOY_BUTTON_7,
  JOY_BUTTON_8,
  JOY_BUTTON_9,
  JOY_BUTTON_10,
  JOY_BUTTON_11,
  JOY_BUTTON_12,
  JOY_BUTTON_13,
  JOY_BUTTON_14,
  JOY_BUTTON_15,
  JOY_BUTTON_16,
};

int get_joystick(void);
int get_keyboard(void);
int key_map(SDLKey key_sym);
int joy_map(u32 button);
