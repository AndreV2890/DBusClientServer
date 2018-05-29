CC := g++
CFLAGS = -g -Wall -Werror

CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
CFLAGS_GIO  = $(shell pkg-config --cflags --libs gio-2.0)
CFLAGS_GLIB  = $(shell pkg-config --cflags --libs glib-2.0)

# libLeap.so location
LEAP_LIBRARY := leap/lib/x64/libLeap.so -Wl,-rpath,leap/lib/x64

# librealt.so location and name
REALT_NAME = realt
REALT_LIBRARY := leap/lib/x64/libLeap.so -Wl,-rpath,leap/lib/x64

# build all process
all: d_server d_client

# build shared realt library in the lib folder
librealt: realt/lib/lib$(REALT_NAME).so

realt/lib/lib$(REALT_NAME).so: $(wildcard realt/src/*.c)
	mkdir -p $(@D)
	$(CC) $^ -fPIC -shared -Wl,-soname,lib$(REALT_NAME).so -o $@

# build dbus-server
d_server: server.c
	gcc $< -o $@ $(CFLAGS) $(CFLAGS_GIO)

# build dbus-client
d_client: client.c librealt
	$(CC) $< -o $@ $(CFLAGS) $(CFLAGS_GIO) -Ileap/include $(LEAP_LIBRARY) -Irealt/include $(REALT_LIBRARY)

# clean project folder
clean:
	rm -f server
	rm -f client
	rm -f -r realt/lib

.PHONY: all clean client server librealt
