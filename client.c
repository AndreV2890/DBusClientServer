/*
 * This uses the recommended GLib API for D-Bus: GDBus,
 * which has been distributed with GLib since version 2.26.
 *
 * For details of how to use GDBus, see:
 * https://developer.gnome.org/gio/stable/gdbus-convenience.html
 *
 * dbus-glib also exists but is deprecated.
 */
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/* glib headers */
#include <glib/gprintf.h>
#include <gio/gio.h>

/* Leap motion header */
#include "Leap.h"

/* define dbus Server interface */
#define SERVER_BUS_NAME 		"org.cbsd.LeapServer"
#define SERVER_OBJECT_PATH 		"/org/cbsd/LeapServer"
#define SERVER_INTERFACE_NAME	"org.cbsd.LeapServer"	


void invokeServerMethod(GDBusProxy *proxy) {
	GVariant *result;
	GError *error = NULL;
	const gchar *str;
	GVariant *parameters;

	const gchar *methodName = "Hibernate";
	parameters = g_variant_new("(s)", methodName);

	g_printf("Calling GestureManager(%s)\n", methodName);
	result = g_dbus_proxy_call_sync(proxy, "GestureManager", parameters, 
									G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	g_assert_no_error(error);
	g_variant_get(result, "(&s)", &str);
	g_printf("The server answered: '%s'\n", str);
	g_variant_unref(result);
}


int main(void)
{
	Leap::Controller controller;
	Leap::Frame f;
	Leap::HandList handsInFrame;

	while(true){

		f = controller.frame();
		handsInFrame = f.hands();
		std::cout << "Hands in frame: " << handsInFrame.count() << std::endl;
		

		sleep(1);
	}

    // Keep this process running until Enter is pressed
    std::cout << "Press Enter to quit..." << std::endl;
    std::cin.get();

	GDBusProxy *proxy;
	GDBusConnection *conn;
	GVariant *variant;
	GError *error = NULL;

	conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	g_assert_no_error(error);

	proxy = g_dbus_proxy_new_sync(conn,			// GDBusConnection 
				      G_DBUS_PROXY_FLAGS_NONE,	// GDBusProxyFlags 
				      NULL,						// GDBusInterfaceInfo 
				      SERVER_BUS_NAME,			// name 
				      SERVER_OBJECT_PATH,		// object path 
				      SERVER_INTERFACE_NAME,	// interface 
				      NULL,						// GCancellable 
				      &error);					// GError 
	g_assert_no_error(error);

	/* retrieve server version */
	const gchar *version;
	variant = g_dbus_proxy_get_cached_property(proxy, "Version");
	g_assert(variant != NULL);
	g_variant_get(variant, "s", &version);
	g_variant_unref(variant);
	printf("LeapServer interface version: %s\n", version);

	/* invoke server methods */
	//invokeServerMethod(proxy);

	g_object_unref(proxy);
	g_object_unref(conn);
	
	return 0;
}