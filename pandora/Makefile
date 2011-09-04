# gpSP makefile
# Gilead Kutnick - Exophase
# pandora port - notaz

# Global definitions

CC        = $(CROSS_COMPILE)gcc

OBJS      = pnd.o main.o cpu.o memory.o video.o input.o sound.o gui.o \
            cheats.o zip.o cpu_threaded.o arm_stub.o video_blend.o warm.o \
            linux/fbdev.o linux/xenv.o
BIN       = gpsp

# Platform specific definitions 

VPATH      += .. ../arm
CFLAGS     += -DARM_ARCH -DPND_BUILD
CFLAGS     += -funsigned-char
CFLAGS     += -mcpu=cortex-a8 -mtune=cortex-a8 -mfloat-abi=softfp -ffast-math
CFLAGS     += -fno-common -fno-builtin
CFLAGS     += -ggdb
CFLAGS     += -O2

# expecting to have PATH set up to get correct sdl-config first
CFLAGS     += `sdl-config --cflags`
LIBS       += `sdl-config --libs`
LIBS       += -ldl -lpthread -lz

# Compilation:

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

all:	$(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(BIN)

clean:
	rm -f *.o $(BIN) 

