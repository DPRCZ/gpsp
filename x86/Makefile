# gpSP makefile
# Gilead Kutnick - Exophase

# Global definitions

CC        = gcc
STRIP     = strip
AS        = as

PREFIX    = /usr
OBJS      = main.o cpu.o memory.o video.o input.o sound.o \
	    cpu_threaded.o gui.o x86_stub.o cheats.o zip.o
BIN       ?= gpsp.exe 

# Platform specific definitions 

VPATH      += ..
CFLAGS     += -DPC_BUILD -m32
INCLUDES   = -I${PREFIX}/include `sdl-config --cflags`
LIBS       = -L${PREFIX}/lib `sdl-config --libs` -mconsole -lz -m32

# Compilation:

.SUFFIXES: .c .S

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

%.o: %.S
	${CC} ${CFLAGS} -c -o $@ $<

all:	${OBJS}
	${CC} ${OBJS} ${LIBS} -o ${BIN}  
	${STRIP} ${BIN}

clean:
	rm -f *.o ${BIN} 

