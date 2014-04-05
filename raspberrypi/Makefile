# gpSP makefile
# Gilead Kutnick - Exophase
# pandora port - notaz
# raspberry pi - DPR

# Global definitions

CC        = gcc

OBJS      = rpi.o main.o cpu.o memory.o video.o input.o sound.o gui.o \
            cheats.o zip.o  arm_stub.o  warm.o cpu_threaded.o\
	    gles_video.o video_blend.o

BIN       = gpsp

# Platform specific definitions 

VPATH      += .. ../arm
CFLAGS     += -DARM_ARCH -DRPI_BUILD -Wall
CFLAGS	   += -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6j -fgnu89-inline
CFLAGS     += -I$(SDKSTAGE)/opt/vc/include -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads
CFLAGS     += -I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux

LIBS       += -ldl -lpthread -lz
LIBS       += -L$(SDKSTAGE)/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm  -lrt

# Dependencies
ifeq ($(origin SDL_CFLAGS) $(origin SDL_LDLIBS), undefined undefined)
  SDL_CONFIG ?= sdl-config
  ifeq ($(shell which $(SDL_CONFIG) 2>/dev/null),)
    $(error No SDL development libraries found!)
  endif
  SDL_CFLAGS += $(shell $(SDL_CONFIG) --cflags)
  SDL_LIBS += $(shell $(SDL_CONFIG) --libs)
endif
CFLAGS += $(SDL_CFLAGS)
LIBS += $(SDL_LIBS)

# Compilation:

all: $(BIN)

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<


cpu.o cpu_threaded.o: CFLAGS += -Wno-unused-variable -Wno-unused-label

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(BIN)

clean:
	rm -f *.o $(BIN)
