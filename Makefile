CC := g++
CFLAGS := -g -Wall -Werror

CFLAGS_GIO  := $(shell pkg-config --cflags --libs gio-2.0)
CFLAGS_GLIB  := $(shell pkg-config --cflags --libs glib-2.0)
CFLAGS_X11 := $(shell pkg-config --cflags --libs x11)
CFLAGS_X11_EXTENSION := $(shell pkg-config --cflags --libs xtst)

# libLeap.so location
LEAP_LIBRARY := leap/lib/x64/libLeap.so 
CFLAGS_LEAP := -Wl,-rpath,leap/lib/x64

# librealt.so location and name
REALT_NAME := realt
REALT_LIBRARY := realt/lib/lib$(REALT_NAME).so
CFLAGS_REALT := -Wl,-rpath,realt/lib

LIBREALT := realt/lib/lib$(REALT_NAME).so

# build all process
all: d_server d_client

# build shared realt library in the lib folder
realt/lib/lib$(REALT_NAME).so: $(wildcard realt/src/*.c)
	mkdir -p $(@D)
	$(CC) $^ -fPIC -shared -Wl,-soname,lib$(REALT_NAME).so -o $@

# build dbus-server
d_server: server.c
	gcc $< -o $@ $(CFLAGS) $(CFLAGS_GIO) $(CFLAGS_X11) $(CFLAGS_X11_EXTENSION)

# build dbus-client
d_client: client.c $(LIBREALT)
	$(CC) $< -o $@ $(CFLAGS) $(CFLAGS_GIO) -Ileap/include $(LEAP_LIBRARY) $(CFLAGS_LEAP) -Irealt/include $(REALT_LIBRARY) $(CFLAGS_REALT)

# clean project folder
clean:
	rm -f d_server
	rm -f d_client
	rm -f -r realt/lib

.PHONY: all clean librealt
