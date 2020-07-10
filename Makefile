
CFLAGS = -std=c11 -Wall -Werror -O2 -D_GNU_SOURCE
LDFLAGS = -lX11 -lXext -lmagic -lpng -ljpeg
OBJDIR = obj
OBJS = $(OBJDIR)/event.o $(OBJDIR)/fb.o $(OBJDIR)/image.o $(OBJDIR)/jpg.o \
$(OBJDIR)/png.o $(OBJDIR)/render.o $(OBJDIR)/window.o $(OBJDIR)/vpic.o
PROGNAME = vpic

.PHONY: default prepare all clean

default: all

all: prepare $(PROGNAME)

prepare:
	@[ -d $(OBJDIR) ] || mkdir -v $(OBJDIR)

$(PROGNAME): $(OBJS)
	gcc $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(PROGNAME)

$(OBJDIR)/vpic.o: vpic.h vpic.c
	gcc -c $(CFLAGS) vpic.c -o $(OBJDIR)/vpic.o

$(OBJDIR)/event.o: event.c
	gcc -c $(CFLAGS) event.c -o $(OBJDIR)/event.o

$(OBJDIR)/image.o: image.c
	gcc -c $(CFLAGS) image.c -o $(OBJDIR)/image.o

$(OBJDIR)/jpg.o: jpg.c
	gcc -c $(CFLAGS) jpg.c -o $(OBJDIR)/jpg.o

$(OBJDIR)/png.o: png.c
	gcc -c $(CFLAGS) png.c -o $(OBJDIR)/png.o

$(OBJDIR)/render.o: render.c
	gcc -c $(CFLAGS) render.c -o $(OBJDIR)/render.o

$(OBJDIR)/fb.o: fb.c
	gcc -c $(CFLAGS) fb.c -o $(OBJDIR)/fb.o

$(OBJDIR)/window.o: window.c
	gcc -c $(CFLAGS) window.c -o $(OBJDIR)/window.o

clean:
	@rm -rfv $(OBJDIR) $(PROGNAME) 2>/dev/null || true

