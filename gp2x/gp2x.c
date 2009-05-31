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


#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "../common.h"
#include "gp2x.h"
#include "warm.h"

extern int main_cpuspeed(int argc, char *argv[]);
extern SDL_Surface* screen;

u32 gp2x_audio_volume = 74/2;
u32 gpsp_gp2x_dev_audio = 0;
u32 gpsp_gp2x_dev = 0;
u32 gpsp_gp2x_gpiodev = 0;

static volatile u16 *gpsp_gp2x_memregs;
static volatile u32 *gpsp_gp2x_memregl;
unsigned short *gp2x_memregs;

s32 gp2x_load_mmuhack()
{
  s32 mmufd = open("/dev/mmuhack", O_RDWR);

  if(mmufd < 0)
  {
    system("/sbin/insmod mmuhack.o");
    mmufd = open("/dev/mmuhack", O_RDWR);
  }

  if(mmufd < 0)
    return -1;

  close(mmufd);
  return 0;
}

#ifdef WIZ_BUILD
#include <linux/fb.h>
void *gpsp_gp2x_screen;
static u32 fb_paddr[3];
static void *fb_vaddr[3];
static u32 fb_work_buf;
const int fb_buf_count = 3;
static int fb_buf_use = 3;

static void fb_video_init()
{
  struct fb_fix_screeninfo fbfix;
  int i, ret;
  int fbdev;

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
  close(fbdev);

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
  warm_cache_op_all(WOP_D_CLEAN);
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
}

static void fb_video_exit()
{
  /* switch to default fb mem */
  gpsp_gp2x_memregl[0x406C>>2] = fb_paddr[0];
  gpsp_gp2x_memregl[0x4058>>2] |= 0x10;
}
#endif

void gp2x_init()
{
  gpsp_gp2x_dev = open("/dev/mem",   O_RDWR);
  gpsp_gp2x_dev_audio = open("/dev/mixer", O_RDWR);
  gpsp_gp2x_memregl =
   (unsigned long  *)mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED,
   gpsp_gp2x_dev, 0xc0000000);
  gpsp_gp2x_memregs = (unsigned short *)gpsp_gp2x_memregl;
#ifdef WIZ_BUILD
  gpsp_gp2x_gpiodev = open("/dev/GPIO", O_RDONLY);
  warm_init();
  fb_video_init();
#endif

//  clear_screen(0);
//  main_cpuspeed(0, NULL);
  gp2x_memregs = (void *)gpsp_gp2x_memregs;
  cpuctrl_init();
  gp2x_sound_volume(1);
}

void gp2x_quit()
{
#ifdef WIZ_BUILD
  close(gpsp_gp2x_gpiodev);
  fb_video_exit();
#endif
  munmap((void *)gpsp_gp2x_memregl, 0x10000);
  close(gpsp_gp2x_dev_audio);
  close(gpsp_gp2x_dev);

  //chdir("/usr/gp2x");
  //execl("gp2xmenu", "gp2xmenu", NULL);
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

#ifdef WIZ_BUILD
void cpuctrl_init(void)
{
}

void set_FCLK(u32 MHZ)
{
}
#endif

