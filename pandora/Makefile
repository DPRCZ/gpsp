# gpSP makefile
# Gilead Kutnick - Exophase
# pandora port - notaz

# Global definitions

CC        = $(CROSS_COMPILE)gcc

OBJS      = pnd.o main.o cpu.o memory.o video.o input.o sound.o gui.o \
            cheats.o zip.o cpu_threaded.o arm_stub.o video_blend.o warm.o \
            neon_scale2x.o neon_scale3x.o neon_eagle2x.o \
            linux/fbdev.o linux/xenv.o
BIN       = gpsp

# Platform specific definitions 

VPATH      += .. ../arm
CFLAGS     += -DARM_ARCH -DPND_BUILD -Wall
CFLAGS     += -mcpu=cortex-a8 -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=neon -ffast-math
CFLAGS     += -ggdb
ifndef DEBUG
CFLAGS     += -O2
endif

# expecting to have PATH set up to get correct sdl-config first
CFLAGS     += `sdl-config --cflags`
LIBS       += `sdl-config --libs`
LIBS       += -ldl -lpthread -lz

# Compilation:

all:	$(BIN) picorestore

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

cpu.o cpu_threaded.o: CFLAGS += -Wno-unused-variable -Wno-unused-label

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(BIN)

clean:
	rm -f *.o $(BIN) picorestore

# ----------- release -----------

PND_MAKE ?= $(HOME)/dev/pnd/src/pandora-libraries/testdata/scripts/pnd_make.sh

rel: gpsp gpsp.sh gpsp.pxml gba_icon.png picorestore readme.txt ../game_config.txt ../COPYING.DOC
	rm -rf out
	mkdir out
	cp $^ out/
	$(PND_MAKE) -p gpsp.pnd -d out -x out/gpsp.pxml -i out/gba_icon.png -c
