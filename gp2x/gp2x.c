/*  
    Parts used from cpuctrl, Copyright (C) 2005  Hermes/PS2Reality
    Portions Copyright (C) 2009 notaz

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#define _GNU_SOURCE 1
#include "../common.h"
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <linux/input.h>
#include "gp2x.h"
#include "pollux_dpc_set.h"

static u32 gpsp_gp2x_dev_audio;
static u32 gpsp_gp2x_dev;
#ifdef POLLUX_BUILD
static u32 gpsp_gp2x_indev;
static u32 saved_405c, saved_4060, saved_4058;
#endif

static u32 gp2x_audio_volume = 74/2;

static volatile u16 *gpsp_gp2x_memregs;
static volatile u32 *gpsp_gp2x_memregl;

u32 button_plat_mask_to_config[PLAT_BUTTON_COUNT] =
{
  GP2X_UP,
  GP2X_LEFT,
  GP2X_DOWN,
  GP2X_RIGHT,
  GP2X_START,
  GP2X_SELECT,
  GP2X_L,
  GP2X_R,
  GP2X_A,
  GP2X_B,
  GP2X_X,
  GP2X_Y,
#if defined(POLLUX_BUILD) && !defined(WIZ_BUILD)
  0,
  0,
  GP2X_PUSH,
  GP2X_HOME,
#else
  GP2X_VOL_DOWN,
  GP2X_VOL_UP,
  GP2X_PUSH,
  GP2X_VOL_MIDDLE
#endif
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
  BUTTON_ID_VOLDOWN,            // Vol down
  BUTTON_ID_VOLUP,              // Vol up
  BUTTON_ID_FPS,                // Push
  BUTTON_ID_MENU                // Vol middle / Home
};

#ifdef POLLUX_BUILD
#include <linux/fb.h>
void *gpsp_gp2x_screen;
#define fb_buf_count 4
static u32 fb_paddr[fb_buf_count];
static void *fb_vaddr[fb_buf_count];
static u32 fb_work_buf;
static int fb_buf_use;
static int fbdev;

static void fb_video_init()
{
  struct fb_fix_screeninfo fbfix;
  unsigned int r;
  int i, ret;

  fbdev = open("/dev/fb0", O_RDWR);
  if (fbdev < 0) {
    perror("can't open fbdev");
    exit(1);
  }

  ret = ioctl(fbdev, FBIOGET_FSCREENINFO, &fbfix);
  if (ret == -1)
  {
    perror("ioctl(fbdev) failed");
    exit(1);
  }

  printf("framebuffer: \"%s\" @ %08lx\n", fbfix.id, fbfix.smem_start);
  fb_paddr[0] = fbfix.smem_start;

  fb_vaddr[0] = mmap(0, 320*240*2*fb_buf_count, PROT_READ|PROT_WRITE,
    MAP_SHARED, gpsp_gp2x_dev, fb_paddr[0]);
  if (fb_vaddr[0] == MAP_FAILED)
  {
    perror("mmap(fb_vaddr) failed");
    exit(1);
  }
  memset(fb_vaddr[0], 0, 320*240*2*fb_buf_count);

  printf("  %p -> %08x\n", fb_vaddr[0], fb_paddr[0]);
  for (i = 1; i < fb_buf_count; i++)
  {
    fb_paddr[i] = fb_paddr[i-1] + 320*240*2;
    fb_vaddr[i] = (char *)fb_vaddr[i-1] + 320*240*2;
    printf("  %p -> %08x\n", fb_vaddr[i], fb_paddr[i]);
  }
  fb_work_buf = 0;
  fb_buf_use = fb_buf_count;

  saved_405c = gpsp_gp2x_memregl[0x405c>>2];
  saved_4060 = gpsp_gp2x_memregl[0x4060>>2];
  saved_4058 = gpsp_gp2x_memregl[0x4058>>2];

  // set mode; program both MLCs so that TV-out works
  gpsp_gp2x_memregl[0x405c>>2] = gpsp_gp2x_memregl[0x445c>>2] = 2;
  gpsp_gp2x_memregl[0x4060>>2] = gpsp_gp2x_memregl[0x4460>>2] = 320 * 2;

  r = gpsp_gp2x_memregl[0x4058>>2];
  r = (r & 0xffff) | (0x4432 << 16) | 0x10;
  gpsp_gp2x_memregl[0x4058>>2] = r;

  r = gpsp_gp2x_memregl[0x4458>>2];
  r = (r & 0xffff) | (0x4432 << 16) | 0x10;
  gpsp_gp2x_memregl[0x4458>>2] = r;

  pollux_video_flip();
  warm_change_cb_upper(WCB_C_BIT|WCB_B_BIT, 1);
}

void pollux_video_flip()
{
  gpsp_gp2x_memregl[0x406C>>2] =
  gpsp_gp2x_memregl[0x446C>>2] = fb_paddr[fb_work_buf];
  gpsp_gp2x_memregl[0x4058>>2] |= 0x10;
  gpsp_gp2x_memregl[0x4458>>2] |= 0x10;
  fb_work_buf++;
  if (fb_work_buf >= fb_buf_use)
    fb_work_buf = 0;
  gpsp_gp2x_screen = fb_vaddr[fb_work_buf];
}

void fb_use_buffers(int count)
{
  if (count < 1)
    count = 1;
  else if (count > fb_buf_count)
    count = fb_buf_count;
  fb_buf_use = count;
  memset(fb_vaddr[0], 0, 320*240*2*count);
}

void wiz_lcd_set_portrait(int y)
{
  char *dpc_settings;
#ifdef WIZ_BUILD
  static int old_y = -1;
  int cmd[2] = { 0, 0 };

  if (old_y == y)
    return;
  cmd[0] = y ? 6 : 5;
  ioctl(fbdev, _IOW('D', 90, int[2]), cmd);
  gpsp_gp2x_memregl[0x4004>>2] = y ? 0x013f00ef : 0x00ef013f;
  gpsp_gp2x_memregl[0x4000>>2] |= 1 << 3;
  old_y = y;

  /* the above ioctl resets LCD timings, so set them here */
#endif

  dpc_settings = getenv("pollux_dpc_set");
  if (dpc_settings != NULL)
    pollux_dpc_set(gpsp_gp2x_memregs, dpc_settings);
}

static void fb_video_exit()
{
  /* switch to default fb mem, turn portrait off */
  gpsp_gp2x_memregl[0x406c>>2] = fb_paddr[0];
  gpsp_gp2x_memregl[0x405c>>2] = saved_405c;
  gpsp_gp2x_memregl[0x4060>>2] = saved_4060;
  gpsp_gp2x_memregl[0x4058>>2] = saved_4058 | 0x10;
  wiz_lcd_set_portrait(0);
  close(fbdev);
}

static int wiz_gamepak_fd = -1;
static u32 wiz_gamepak_size;

static void wiz_gamepak_cleanup()
{
  if (wiz_gamepak_size)
    munmap(gamepak_rom, wiz_gamepak_size);
  if (wiz_gamepak_fd >= 0)
    close(wiz_gamepak_fd);
  gamepak_rom = NULL;
  wiz_gamepak_size = 0;
  wiz_gamepak_fd = -1;
}

u32 wiz_load_gamepak(char *name)
{
  char *dot_position = strrchr(name, '.');
  u32 ret;

  if (!strcasecmp(dot_position, ".zip"))
  {
    if (wiz_gamepak_fd >= 0)
    {
      wiz_gamepak_cleanup();
      printf("switching to ROM malloc\n");
      init_gamepak_buffer();
    }
    return load_file_zip(name);
  }

  if (wiz_gamepak_fd < 0)
  {
    extern void *gamepak_memory_map;
    free(gamepak_rom);
    free(gamepak_memory_map);
    gamepak_memory_map = NULL;
    printf("switching to ROM mmap\n");
  }
  else
    wiz_gamepak_cleanup();

  wiz_gamepak_fd = open(name, O_RDONLY|O_NOATIME, S_IRUSR);
  if (wiz_gamepak_fd < 0)
  {
    perror("wiz_load_gamepak: open failed");
    return -1;
  }

  ret = lseek(wiz_gamepak_fd, 0, SEEK_END);
  wiz_gamepak_size = gamepak_ram_buffer_size = ret;

  gamepak_rom = mmap(0, ret, PROT_READ, MAP_SHARED, wiz_gamepak_fd, 0);
  if (gamepak_rom == MAP_FAILED)
  {
    perror("wiz_load_gamepak: mmap failed");
    return -1;
  }

  return ret;
}

#define KEYBITS_BIT(x) (keybits[(x)/sizeof(keybits[0])/8] & \
  (1 << ((x) & (sizeof(keybits[0])*8-1))))

static int abs_min, abs_max, lzone_step;

static int open_caanoo_pad(void)
{
  long keybits[KEY_CNT / sizeof(long) / 8];
  long absbits[(ABS_MAX+1) / sizeof(long) / 8];
  struct input_absinfo ainfo;
  int fd = -1, i;

  memset(keybits, 0, sizeof(keybits));
  memset(absbits, 0, sizeof(absbits));

  for (i = 0;; i++)
  {
    int support = 0, need;
    int ret;
    char name[64];

    snprintf(name, sizeof(name), "/dev/input/event%d", i);
    fd = open(name, O_RDONLY|O_NONBLOCK);
    if (fd == -1)
      break;

    /* check supported events */
    ret = ioctl(fd, EVIOCGBIT(0, sizeof(support)), &support);
    if (ret == -1) {
      printf("in_evdev: ioctl failed on %s\n", name);
      goto skip;
    }

    need = (1 << EV_KEY) | (1 << EV_ABS);
    if ((support & need) != need)
      goto skip;

    ret = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
    if (ret == -1) {
      printf("in_evdev: ioctl failed on %s\n", name);
      goto skip;
    }

    if (!KEYBITS_BIT(BTN_JOYSTICK))
      goto skip;

    ret = ioctl(fd, EVIOCGABS(ABS_X), &ainfo);
    if (ret == -1)
      goto skip;

    abs_min = ainfo.minimum;
    abs_max = ainfo.maximum;
    lzone_step = (abs_max - abs_min) / 2 / 9;

    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    printf("using \"%s\" (type %08x)\n", name, support);
    break;

skip:
    close(fd);
    fd = -1;
  }

  if (fd == -1) {
    printf("missing input device\n");
    exit(1);
  }

  return fd;
}
#endif // POLLUX_BUILD

void gpsp_plat_init(void)
{
  gpsp_gp2x_dev = open("/dev/mem",   O_RDWR);
  gpsp_gp2x_dev_audio = open("/dev/mixer", O_RDWR);
  gpsp_gp2x_memregl = (u32 *)mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED,
   gpsp_gp2x_dev, 0xc0000000);
  gpsp_gp2x_memregs = (u16 *)gpsp_gp2x_memregl;
  warm_init();
#ifdef POLLUX_BUILD
#ifdef WIZ_BUILD
  gpsp_gp2x_indev = open("/dev/GPIO", O_RDONLY);
#else
  gpsp_gp2x_indev = open_caanoo_pad();
#endif
  fb_video_init();
  (void)open_caanoo_pad;
#endif

  gp2x_sound_volume(1);
}

void gpsp_plat_quit(void)
{
  chdir(main_path);

  warm_finish();
#ifdef POLLUX_BUILD
  wiz_gamepak_cleanup();
  close(gpsp_gp2x_indev);
  fb_video_exit();
#endif
  munmap((void *)gpsp_gp2x_memregl, 0x10000);
  close(gpsp_gp2x_dev_audio);
  close(gpsp_gp2x_dev);

  fcloseall();
  sync();
  exit(0);
}

void gp2x_sound_volume(u32 volume_up)
{
  u32 volume;
  if((volume_up == 0) && (gp2x_audio_volume > 0))
    gp2x_audio_volume--;

  if((volume_up != 0)  && (gp2x_audio_volume < 100))
    gp2x_audio_volume++;

  volume = (gp2x_audio_volume * 0x50) / 100;
  volume = (gp2x_audio_volume << 8) | gp2x_audio_volume;
  ioctl(gpsp_gp2x_dev_audio, SOUND_MIXER_WRITE_PCM, &volume);
}

u32 gpsp_plat_joystick_read(void)
{
#ifdef WIZ_BUILD
  u32 value = 0;
  read(gpsp_gp2x_indev, &value, 4);
  if(value & 0x02)
   value |= 0x05;
  if(value & 0x08)
   value |= 0x14;
  if(value & 0x20)
   value |= 0x50;
  if(value & 0x80)
   value |= 0x41;
  return value;
#elif defined(POLLUX_BUILD)
  // caanoo
  static const int evdev_to_gp2x[] = {
    GP2X_A, GP2X_X, GP2X_B, GP2X_Y, GP2X_L, GP2X_R,
    GP2X_HOME, 0, GP2X_START, GP2X_SELECT, GP2X_PUSH
  };
  int keybits[KEY_CNT / sizeof(int)];
  struct input_absinfo ainfo;
  int lzone = analog_sensitivity_level * lzone_step;
  u32 retval = 0;
  int i, ret;

  ret = ioctl(gpsp_gp2x_indev, EVIOCGKEY(sizeof(keybits)), keybits);
  if (ret == -1) {
    perror("EVIOCGKEY ioctl failed");
    sleep(1);
    return 0;
  }

  for (i = 0; i < sizeof(evdev_to_gp2x) / sizeof(evdev_to_gp2x[0]); i++) {
    if (KEYBITS_BIT(BTN_TRIGGER + i))
      retval |= evdev_to_gp2x[i];
  }

  if (lzone != 0)
    lzone--;

  ret = ioctl(gpsp_gp2x_indev, EVIOCGABS(ABS_X), &ainfo);
  if (ret != -1) {
    if      (ainfo.value <= abs_min + lzone) retval |= GP2X_LEFT;
    else if (ainfo.value >= abs_max - lzone) retval |= GP2X_RIGHT;
  }
  ret = ioctl(gpsp_gp2x_indev, EVIOCGABS(ABS_Y), &ainfo);
  if (ret != -1) {
    if      (ainfo.value <= abs_min + lzone) retval |= GP2X_UP;
    else if (ainfo.value >= abs_max - lzone) retval |= GP2X_DOWN;
  }

  return retval;
#else
  // GP2X
  u32 value = (gpsp_gp2x_memregs[0x1198 >> 1] & 0x00FF);

  if(value == 0xFD)
   value = 0xFA;
  if(value == 0xF7)
   value = 0xEB;
  if(value == 0xDF)
   value = 0xAF;
  if(value == 0x7F)
   value = 0xBE;

  return ~((gpsp_gp2x_memregs[0x1184 >> 1] & 0xFF00) | value |
   (gpsp_gp2x_memregs[0x1186 >> 1] << 16));
#endif
}

u32 gpsp_plat_buttons_to_cursor(u32 buttons)
{
  gui_action_type new_button = CURSOR_NONE;

  if(buttons & GP2X_A)
    new_button = CURSOR_BACK;

  if(buttons & GP2X_X)
    new_button = CURSOR_EXIT;

  if(buttons & GP2X_B)
    new_button = CURSOR_SELECT;

  if(buttons & GP2X_UP)
    new_button = CURSOR_UP;

  if(buttons & GP2X_DOWN)
    new_button = CURSOR_DOWN;

  if(buttons & GP2X_LEFT)
    new_button = CURSOR_LEFT;

  if(buttons & GP2X_RIGHT)
    new_button = CURSOR_RIGHT;

  if(buttons & GP2X_L)
    new_button = CURSOR_L;

  if(buttons & GP2X_R)
    new_button = CURSOR_R;

  return new_button;
}

// Fout = (m * Fin) / (p * 2^s)
void set_FCLK(u32 MHZ)
{
  u32 v;
  u32 mdiv, pdiv, sdiv = 0;
#ifdef POLLUX_BUILD
  int i;
  #define SYS_CLK_FREQ 27
  // m = MDIV, p = PDIV, s = SDIV
  pdiv = 9;
  mdiv = (MHZ * pdiv) / SYS_CLK_FREQ;
  if (mdiv & ~0x3ff)
    return;
  v = (pdiv<<18) | (mdiv<<8) | sdiv;

  gpsp_gp2x_memregl[0xf004>>2] = v;
  gpsp_gp2x_memregl[0xf07c>>2] |= 0x8000;
  for (i = 0; (gpsp_gp2x_memregl[0xf07c>>2] & 0x8000) && i < 0x100000; i++)
    ;

  // must restart sound as it's PLL is shared with CPU one
  sound_exit();
  init_sound(0);
#else
  #define SYS_CLK_FREQ 7372800
  // m = MDIV + 8, p = PDIV + 2, s = SDIV
  pdiv = 3;
  mdiv = (MHZ * pdiv * 1000000) / SYS_CLK_FREQ;
  mdiv &= 0xff;
  v = ((mdiv-8)<<8) | ((pdiv-2)<<2) | sdiv;
  gpsp_gp2x_memregs[0x910>>1] = v;
#endif
}

