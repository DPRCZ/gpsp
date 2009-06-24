/*  Parts used from cpuctrl */
/*  cpuctrl for GP2X
    Copyright (C) 2005  Hermes/PS2Reality

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


#define _BSD_SOURCE
#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <unistd.h>
#include "../common.h"
#include "gp2x.h"
#include "warm.h"
#include "pollux_dpc_set.h"

u32 gp2x_audio_volume = 74/2;
u32 gpsp_gp2x_dev_audio = 0;
u32 gpsp_gp2x_dev = 0;
u32 gpsp_gp2x_gpiodev = 0;

static volatile u16 *gpsp_gp2x_memregs;
static volatile u32 *gpsp_gp2x_memregl;

#ifdef WIZ_BUILD
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

  pollux_video_flip();
  warm_change_cb_upper(WCB_C_BIT|WCB_B_BIT, 1);
}

void pollux_video_flip()
{
  gpsp_gp2x_memregl[0x406C>>2] = fb_paddr[fb_work_buf];
  gpsp_gp2x_memregl[0x4058>>2] |= 0x10;
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
  pollux_dpc_set(gpsp_gp2x_memregs, getenv("pollux_dpc_set"));
}

static void fb_video_exit()
{
  /* switch to default fb mem, turn portrait off */
  gpsp_gp2x_memregl[0x406C>>2] = fb_paddr[0];
  gpsp_gp2x_memregl[0x4058>>2] |= 0x10;
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

#endif

static int get_romdir(char *buff, size_t size)
{
  FILE *f;
  char *s;
  int r = -1;
  
  f = fopen("romdir.txt", "r");
  if (f == NULL)
    return -1;

  s = fgets(buff, size, f);
  if (s)
  {
    r = strlen(s);
    while (r > 0 && isspace(buff[r-1]))
      buff[--r] = 0;
  }

  fclose(f);
  return r;
}

void gp2x_init()
{
  char buff[256];

  gpsp_gp2x_dev = open("/dev/mem",   O_RDWR);
  gpsp_gp2x_dev_audio = open("/dev/mixer", O_RDWR);
  gpsp_gp2x_memregl =
   (unsigned long  *)mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED,
   gpsp_gp2x_dev, 0xc0000000);
  gpsp_gp2x_memregs = (unsigned short *)gpsp_gp2x_memregl;
  warm_init();
#ifdef WIZ_BUILD
  gpsp_gp2x_gpiodev = open("/dev/GPIO", O_RDONLY);
  fb_video_init();
#endif

  if (get_romdir(buff, sizeof(buff)) > 0)
    chdir(buff);

  gp2x_sound_volume(1);
}

void gp2x_quit()
{
  char buff1[256], buff2[256];

  getcwd(buff1, sizeof(buff1));
  chdir(main_path);
  if (get_romdir(buff2, sizeof(buff2)) >= 0 &&
    strcmp(buff1, buff2) != 0)
  {
    FILE *f = fopen("romdir.txt", "w");
    if (f != NULL)
    {
      printf("writing romdir: %s\n", buff1);
      fprintf(f, "%s", buff1);
      fclose(f);
    }
  }

  warm_finish();
#ifdef WIZ_BUILD
  close(gpsp_gp2x_gpiodev);
  fb_video_exit();
  wiz_gamepak_cleanup();
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

u32 gpsp_gp2x_joystick_read(void)
{
#ifdef WIZ_BUILD
  u32 value = 0;
  read(gpsp_gp2x_gpiodev, &value, 4);
  if(value & 0x02)
   value |= 0x05;
  if(value & 0x08)
   value |= 0x14;
  if(value & 0x20)
   value |= 0x50;
  if(value & 0x80)
   value |= 0x41;
  return value;
#else
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

// Fout = (m * Fin) / (p * 2^s)
void set_FCLK(u32 MHZ)
{
  u32 v;
  u32 mdiv, pdiv, sdiv = 0;
#ifdef WIZ_BUILD
  #define SYS_CLK_FREQ 27
  // m = MDIV, p = PDIV, s = SDIV
  pdiv = 9;
  mdiv = (MHZ * pdiv) / SYS_CLK_FREQ;
  mdiv &= 0x3ff;
  v = (pdiv<<18) | (mdiv<<8) | sdiv;

  gpsp_gp2x_memregl[0xf004>>2] = v;
  gpsp_gp2x_memregl[0xf07c>>2] |= 0x8000;
  while (gpsp_gp2x_memregl[0xf07c>>2] & 0x8000)
    ;
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

