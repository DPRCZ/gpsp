/* gameplaySP - pandora backend
 *
 * Copyright (C) 2011 notaz <notasas@gmail.com>
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
#include <X11/keysym.h>
#include "linux/omapfb.h" //
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../arm/neon_scale2x.h"
#include "../arm/neon_scale3x.h"
#include "../arm/neon_eagle2x.h"
#include "linux/fbdev.h"
#include "linux/xenv.h"

enum gpsp_key {
  GKEY_UP       = 1 << 0,
  GKEY_LEFT     = 1 << 1,
  GKEY_DOWN     = 1 << 2,
  GKEY_RIGHT    = 1 << 3,
  GKEY_START    = 1 << 4,
  GKEY_SELECT   = 1 << 5,
  GKEY_L        = 1 << 6,
  GKEY_R        = 1 << 7,
  GKEY_A        = 1 << 8,
  GKEY_B        = 1 << 9,
  GKEY_X        = 1 << 10,
  GKEY_Y        = 1 << 11,
  GKEY_1        = 1 << 12,
  GKEY_2        = 1 << 13,
  GKEY_3        = 1 << 14,
  GKEY_4        = 1 << 15,
  GKEY_MENU     = 1 << 16,
};

u32 button_plat_mask_to_config[PLAT_BUTTON_COUNT] =
{
  GKEY_UP,
  GKEY_LEFT,
  GKEY_DOWN,
  GKEY_RIGHT,
  GKEY_START,
  GKEY_SELECT,
  GKEY_L,
  GKEY_R,
  GKEY_A,
  GKEY_B,
  GKEY_X,
  GKEY_Y,
  GKEY_1,
  GKEY_2,
  GKEY_3,
  GKEY_4,
  GKEY_MENU,
};

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

static const u32 xk_to_gkey[] = {
  XK_Up, XK_Left, XK_Down, XK_Right, XK_Alt_L, XK_Control_L,
  XK_Shift_R, XK_Control_R, XK_Home, XK_End, XK_Page_Down, XK_Page_Up,
  XK_1, XK_2, XK_3, XK_4, XK_space,
};

static const u8 gkey_to_cursor[32] = {
  [0 ... 31] = CURSOR_NONE,
  [0] = CURSOR_UP, CURSOR_LEFT, CURSOR_DOWN, CURSOR_RIGHT, CURSOR_NONE, CURSOR_NONE,
  CURSOR_L, CURSOR_R, CURSOR_SELECT, CURSOR_SELECT, CURSOR_EXIT, CURSOR_BACK,
};

struct vout_fbdev *fb;
static void *fb_current;
static int bounce_buf[400 * 272 * 2 / 4];
static int src_w = 240, src_h = 160;
static enum software_filter {
  SWFILTER_NONE = 0,
  SWFILTER_SCALE2X,
  SWFILTER_SCALE3X,
  SWFILTER_EAGLE2X,
} sw_filter;

// (240*3 * 160*3 * 2 * 3)
#define MAX_VRAM_SIZE (400*2 * 272*2 * 2 * 3)


static int omap_setup_layer(int fd, int enabled, int x, int y, int w, int h)
{
  struct omapfb_plane_info pi = { 0, };
  struct omapfb_mem_info mi = { 0, };
  int ret;

  ret = ioctl(fd, OMAPFB_QUERY_PLANE, &pi);
  if (ret != 0) {
    perror("QUERY_PLANE");
    return -1;
  }

  ret = ioctl(fd, OMAPFB_QUERY_MEM, &mi);
  if (ret != 0) {
    perror("QUERY_MEM");
    return -1;
  }

  /* must disable when changing stuff */
  if (pi.enabled) {
    pi.enabled = 0;
    ret = ioctl(fd, OMAPFB_SETUP_PLANE, &pi);
    if (ret != 0)
      perror("SETUP_PLANE");
  }

  /* alloc enough for 3x scaled tripple buffering */
  if (mi.size < MAX_VRAM_SIZE) {
    mi.size = MAX_VRAM_SIZE;
    ret = ioctl(fd, OMAPFB_SETUP_MEM, &mi);
    if (ret != 0) {
      perror("SETUP_MEM");
      return -1;
    }
  }

  pi.pos_x = x;
  pi.pos_y = y;
  pi.out_width = w;
  pi.out_height = h;
  pi.enabled = enabled;

  ret = ioctl(fd, OMAPFB_SETUP_PLANE, &pi);
  if (ret != 0) {
    perror("SETUP_PLANE");
    return -1;
  }

  return 0;
}

void gpsp_plat_init(void)
{
  int ret, w, h, fd;
  const char *layer_fb_name;

  ret = SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
  if (ret != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    exit(1);
  }

  layer_fb_name = getenv("FBDEV_LAYER");
  if (layer_fb_name == NULL)
    layer_fb_name = "/dev/fb1";

  ret = xenv_init();
  if (ret != 0) {
    fprintf(stderr, "xenv_init failed with %d\n", ret);
    exit(1);
  }

  // must set the layer up first to be able to use it
  fd = open(layer_fb_name, O_RDWR);
  if (fd == -1) {
    fprintf(stderr, "%s: ", layer_fb_name);
    perror("open");
    exit(1);
  }

  ret = omap_setup_layer(fd, 0, 0, 0, 400, 272);
  close(fd);
  if (ret != 0) {
    fprintf(stderr, "failed to set up layer, exiting.\n");
    exit(1);
  }

  // double of original menu size
  w = 400*2;
  h = 272*2;
  fb = vout_fbdev_init("/dev/fb1", &w, &h, 16, 3);
  if (fb == NULL) {
    fprintf(stderr, "vout_fbdev_init failed\n");
    exit(1);
  }

  // default to 3x scale
  screen_scale = 2;
}

void gpsp_plat_quit(void)
{
  xenv_finish();
  omap_setup_layer(vout_fbdev_get_fd(fb), 0, 0, 0, 0, 0);
  vout_fbdev_finish(fb);
  SDL_Quit();
}

u32 gpsp_plat_joystick_read(void)
{
  static int gkeystate;
  int key, is_down, i;
  int gkey = -1;
  
  key = xenv_update(&is_down);
  for (i = 0; i < sizeof(xk_to_gkey) / sizeof(xk_to_gkey[0]); i++) {
    if (key == xk_to_gkey[i]) {
      gkey = i;
      break;
    }
  }

  if (gkey >= 0) {
    if (is_down)
      gkeystate |= 1 << gkey;
    else
      gkeystate &= ~(1 << gkey);
  }
  
  return gkeystate;
}

u32 gpsp_plat_buttons_to_cursor(u32 buttons)
{
  int i;

  if (buttons == 0)
    return CURSOR_NONE;

  for (i = 0; (buttons & 1) == 0; i++, buttons >>= 1)
    ;

  return gkey_to_cursor[i];
}

static void set_filter(int is_filtered)
{
  static int was_filtered = -1;
  char buf[128];

  if (is_filtered == was_filtered)
    return;

  snprintf(buf, sizeof(buf), "sudo -n /usr/pandora/scripts/op_videofir.sh %s",
    is_filtered ? "default" : "none");
  system(buf);
  was_filtered = is_filtered;
}

void *fb_flip_screen(void)
{
  void *ret = bounce_buf;
  void *s = bounce_buf;

  switch (sw_filter) {
  case SWFILTER_SCALE2X:
    neon_scale2x_16_16(s, fb_current, src_w, src_w*2, src_w*2*2, src_h);
    break;
  case SWFILTER_SCALE3X:
    neon_scale3x_16_16(s, fb_current, src_w, src_w*2, src_w*3*2, src_h);
    break;
  case SWFILTER_EAGLE2X:
    neon_eagle2x_16_16(s, fb_current, src_w, src_w*2, src_w*2*2, src_h);
    break;
  case SWFILTER_NONE:
  default:
    break;
  }

  fb_current = vout_fbdev_flip(fb);
  if (sw_filter == SWFILTER_NONE)
    ret = fb_current;

  return ret;
}

void fb_wait_vsync(void)
{
  vout_fbdev_wait_vsync(fb);
}

void fb_set_mode(int w, int h, int buffers, int scale,
 int filter, int filter2)
{
  int lx, ly, lw = w, lh = h;
  int multiplier;

  switch (scale) {
    case 0:
      lw = w;
      lh = h;
      break;
    case 1:
      lw = w * 2;
      lh = h * 2;
      break;
    case 2:
      lw = w * 3;
      lh = h * 3;
      break;
    case 3:
      lw = 800;
      lh = 480;
      break;
    case 15:
      lw = 800;
      lh = 480;
      break;
    default:
      fprintf(stderr, "unknown scale: %d\n", scale);
      break;
  }
  if (lw > 800)
    lw = 800;
  if (lh > 480)
    lh = 480;
  lx = 800 / 2 - lw / 2;
  ly = 480 / 2 - lh / 2;

  omap_setup_layer(vout_fbdev_get_fd(fb), 1, lx, ly, lw, lh);
  set_filter(filter);

  sw_filter = filter2;
  if (w != 240) // menu
    sw_filter = SWFILTER_SCALE2X;

  switch (sw_filter) {
  case SWFILTER_SCALE2X:
    multiplier = 2;
    break;
  case SWFILTER_SCALE3X:
    multiplier = 3;
    break;
  case SWFILTER_EAGLE2X:
    multiplier = 2;
    break;
  case SWFILTER_NONE:
  default:
    multiplier = 1;
    break;
  }

  fb_current = vout_fbdev_resize(fb, w * multiplier, h * multiplier,
                                 16, 0, 0, 0, 0, buffers);
  src_w = w;
  src_h = h;
}

// vim:shiftwidth=2:expandtab
