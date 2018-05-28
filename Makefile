CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
CFLAGS_GIO  = $(shell pkg-config --cflags --libs gio-2.0)
CFLAGS_GLIB  = $(shell pkg-config --cflags --libs glib-2.0)

CFLAGS = -g -Wall -Werror

LEAP_LIBRARY := lib/x64/libLeap.so -Wl,-rpath,lib/x64


all: server client

server: server.c
	gcc $< -o $@ $(CFLAGS) $(CFLAGS_GIO)

client: client.c
	gcc $< -o $@ $(CFLAGS) $(CFLAGS_GIO) -I/include $(LEAP_LIBRARY)

clean:
	rm -f server
	rm -f client


.PHONY: all clean
