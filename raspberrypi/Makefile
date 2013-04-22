# gpSP makefile
# Gilead Kutnick - Exophase
# pandora port - notaz
# respberry pi - DPR

# Global definitions

CC        = gcc

OBJS      = rpi.o main.o cpu.o memory.o video.o input.o sound.o gui.o \
            cheats.o zip.o  arm_stub.o  warm.o cpu_threaded.o\
	    gles_video.o video_blend.o

BIN       = gpsp

# Platform specific definitions 

VPATH      += .. ../arm
CFLAGS     += -DARM_ARCH -DRPI_BUILD -Wall
CFLAGS     += -O3 -mfpu=vfp
CFLAGS     += `sdl-config --cflags`
CFLAGS     += -I$(SDKSTAGE)/opt/vc/include -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads
CFLAGS     += -I/opt/vc/include/interface/vmcs_host/linux

# expecting to have PATH set up to get correct sdl-config first

LIBS       += `sdl-config --libs`
LIBS       += -ldl -lpthread -lz
LIBS       += -L$(SDKSTAGE)/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm  -lrt

# Compilation:

all: $(BIN)

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<


cpu.o cpu_threaded.o: CFLAGS += -Wno-unused-variable -Wno-unused-label

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(BIN)

clean:
	rm -f *.o $(BIN)
