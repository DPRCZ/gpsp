# gpSP makefile
# Gilead Kutnick - Exophase
# GP2X port(ion) - Z

# Global definitions

CC        = $(CROSS_COMPILE)gcc

OBJS      = main.o cpu.o memory.u video.o input.o sound.o gp2x.o gui.o        \
            cheats.o zip.o cpu_threaded.z arm_stub.o video_blend.o            \
            warm.o upscale_aspect.o
ifeq ($(WIZ),1)
POLLUX    = 1
OBJS      += pollux_dpc_set.o
BIN       = gpsp_wiz
endif
ifeq ($(CAANOO),1)
POLLUX    = 1
OBJS      += pollux_dpc_set.o
BIN       = gpsp_caanoo
endif
ifeq ($(BIN),)
BIN       = gpsp_gp2x
endif

-include Makefile.local

# Platform specific definitions 

VPATH      += .. ../arm
CFLAGS     += -DARM_ARCH -DGP2X_BUILD
ifeq ($(WIZ),1)
CFLAGS     += -DWIZ_BUILD
endif
ifeq ($(POLLUX),1)
CFLAGS     += -DPOLLUX_BUILD
endif
CFLAGS     += -std=c99 -msoft-float -funsigned-char -Wall -ggdb
ifndef DEBUG
CFLAGS     += -O2
endif

INCLUDES   = `sdl-config --cflags`
LIBS       = `sdl-config --libs` \
              -lm -ldl -lpthread -lz
ifeq ($(WIZ)$(CAANOO),)
LIBS       += -static
endif

# Compilation:

.SUFFIXES: .c

all:    $(BIN)

cpu.o cpu_threaded.z: CFLAGS += -Wno-unused-variable -Wno-unused-label

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

$(BIN):	$(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(BIN)

clean:
	rm -f *.o *.u *.z $(BIN) 

rel: gpsp_caanoo gpsp_caanoo.ini gpsp_caanoo.gpe warm_2.6.24.ko \
		../COPYING.DOC ../readme.txt readme_gp2x.txt ../game_config.txt
	rm -rf out
	mkdir -p out/gpsp_caanoo
	cp $^ out/gpsp_caanoo/
	mv out/gpsp_caanoo/gpsp_caanoo.ini out/
	mv out/gpsp_caanoo/readme_gp2x.txt out/gpsp_caanoo/readme_caanoo.txt
	echo -n '/mnt/sd' > out/gpsp_caanoo/romdir.txt
	cd out && zip -9 -r ../gpsp_caanoo.zip *
