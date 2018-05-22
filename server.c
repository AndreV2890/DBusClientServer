#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h> /* for glib main loop */


const char *version = "0.1";
GMainLoop *mainloop;

/*const DBusObjectPathVTable server_vtable = {
	.message_function = server_message_handler
};*/

int main(void) {
	DBusConnection *conn;
	DBusError err;
	int rv;

	dbus_error_init(&err);

	/* connect to the daemon bus */
	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (!conn) {
		fprintf(stderr, "Failed to get a session DBus connection: %s\n", err.message);
		dbus_error_free(&err);
		return EXIT_FAILURE;
	}

	/*asks the bus to assign the given name to this connection */
	rv = dbus_bus_request_name(conn, "org.example.LeapServer", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
	if (rv != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		fprintf(stderr, "Failed to request name on bus: %s\n", err.message);
		dbus_error_free(&err);
		return EXIT_FAILURE;
	}

	/*if (!dbus_connection_register_object_path(conn, "/org/example/TestObject", &server_vtable, NULL)) {
		fprintf(stderr, "Failed to register a object path for 'TestObject'\n");
		goto fail;
	}*/

	/*
	 * For the sake of simplicity we're using glib event loop to
	 * handle DBus messages. This is the only place where glib is
	 * used.
	 */
	printf("Starting dbus tiny server v%s\n", version);
	mainloop = g_main_loop_new(NULL, false);
	/* Set up the DBus connection to work in a GLib event loop */
	dbus_connection_setup_with_g_main(conn, NULL);
	/* Start the glib event loop */
	g_main_loop_run(mainloop);

	return EXIT_SUCCESS;

}