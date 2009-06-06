# gpSP makefile
# Gilead Kutnick - Exophase
# GP2X port(ion) - Z

# Global definitions

ifeq ($(WIZ),1)
PREFIX  = /opt/arm-openwiz-linux-gnu
CC        = $(PREFIX)/bin/arm-openwiz-linux-gnu-gcc
STRIP     = $(PREFIX)/bin/arm-openwiz-linux-gnu-strip
else
PREFIX  = /opt/open2x/gcc-4.1.1-glibc-2.3.6
CC        = $(PREFIX)/bin/arm-open2x-linux-gcc
STRIP     = $(PREFIX)/bin/arm-open2x-linux-strip
endif

OBJS      = main.o cpu.o memory.u video.o input.o sound.o gp2x.o gui.o        \
            cheats.o zip.o cpu_threaded.z arm_stub.o video_blend.o            \
            warm.o sys_cacheflush.o upscale_aspect.o
BIN       = gpsp.gpe 

# Platform specific definitions 

VPATH      += ..
CFLAGS     += -DARM_ARCH -DGP2X_BUILD
ifeq ($(WIZ),1)
CFLAGS     += -DWIZ_BUILD
endif
# NOTE: -funroll-loops will slow down compiling considerably
CFLAGS     += -O3 -std=c99 -msoft-float -funsigned-char -fno-common           \
              -fno-builtin                                                    \

INCLUDES   = `$(PREFIX)/bin/sdl-config --cflags` -I$(PREFIX)/include
LIBS       = `$(PREFIX)/bin/sdl-config --libs`               \
              -lm -ldl -lpthread -lz
ifneq ($(WIZ),1)
LIBS       += -static
endif

CFLAGS     += -ggdb
LIBS       += -ggdb

# Compilation:

.SUFFIXES: .c

%.z: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.u: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	$(CC) $(ASFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.s
	$(CC) $(ASFLAGS) $(INCLUDES) -c -o $@ $<

all:	$(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(BIN)  
#	$(STRIP) $(BIN)

clean:
	rm -f *.o *.u *.z $(BIN) 

