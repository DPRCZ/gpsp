#ifndef GP2X_H
#define GP2X_H

enum
{
  GP2X_UP       = 1 << 0,
  GP2X_LEFT     = 1 << 2,
  GP2X_DOWN     = 1 << 4,
  GP2X_RIGHT    = 1 << 6,
  GP2X_START    = 1 << 8,  // Wiz: Menu, Caanoo: I
  GP2X_SELECT   = 1 << 9,  // Caanoo: II
  GP2X_L        = 1 << 10,
  GP2X_R        = 1 << 11,
  GP2X_A        = 1 << 12,
  GP2X_B        = 1 << 13,
  GP2X_X        = 1 << 14,
  GP2X_Y        = 1 << 15,
#ifdef WIZ_BUILD
  GP2X_VOL_UP   = 1 << 16,
  GP2X_VOL_DOWN = 1 << 17,
  GP2X_PUSH     = 1 << 18,
#elif defined(POLLUX_BUILD)
  GP2X_HOME     = 1 << 16,
  GP2X_PUSH     = 1 << 17,
  GP2X_VOL_UP   = 1 << 30, // dummy
  GP2X_VOL_DOWN = 1 << 29,
#else
  GP2X_VOL_DOWN = 1 << 22,
  GP2X_VOL_UP   = 1 << 23,
  GP2X_PUSH     = 1 << 27,
#endif
  GP2X_VOL_MIDDLE = (1 << 24), // fake, menu enter
};

void gpsp_plat_init(void);
void gpsp_plat_quit(void);

u32 gpsp_plat_joystick_read(void);
u32 gpsp_plat_buttons_to_cursor(u32 buttons);

#define PLAT_BUTTON_COUNT 16
#define PLAT_MENU_BUTTON 15
extern u32 button_plat_mask_to_config[PLAT_BUTTON_COUNT];

void gp2x_sound_volume(u32 volume_up);
void gp2x_quit();

void set_FCLK(u32 MHZ);

void upscale_aspect(u16 *dst, u16 *src);

/* wiz only */
extern void *gpsp_gp2x_screen;
void fb_use_buffers(int count);
void pollux_video_flip();
void wiz_lcd_set_portrait(int y);
u32  wiz_load_gamepak(char *name);

void do_rotated_blit(void *dst, void *linesx4, u32 y);
void upscale_aspect_row(void *dst, void *linesx3, u32 row);

#endif
