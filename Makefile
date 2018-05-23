
CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
CFLAGS_GIO  = $(shell pkg-config --cflags --libs gio-2.0)
CFLAGS_GLIB  = $(shell pkg-config --cflags --libs glib-2.0)

CFLAGS = -g -Wall -Werror


all: dbus-server dbus-client

server: server.c
	gcc $< -o $@ $(CFLAGS) $(CFLAGS_GIO)

dbus-client: dbus-client.c
	gcc $< -o $@ $(CFLAGS) $(CFLAGS_GIO)

clean:
	rm -f server
	rm -f dbus-client


.PHONY: all clean
