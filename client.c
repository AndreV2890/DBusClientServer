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
#include <glib/gprintf.h>
#include <gio/gio.h>

#define SERVER_BUS_NAME 		"org.example.LeapServer"
#define SERVER_OBJECT_PATH 		"/org/example/LeapServer"
#define SERVER_INTERFACE_NAME	"org.example.LeapServer"	


void invokeServerMethod(GDBusProxy *proxy) {
	GVariant *result;
	GError *error = NULL;
	const gchar *str;
	GVariant *parameters;

	parameters = g_variant_new("(s)", "Reboot");

	g_printf("Calling HelloWorld()...\n");
	result = g_dbus_proxy_call_sync(proxy,
					"HelloWorld",
					parameters,
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL,
					&error);
	g_assert_no_error(error);
	g_variant_get(result, "(&s)", &str);
	g_printf("The server answered: '%s'\n", str);
	g_variant_unref(result);
}


int main(void)
{
	GDBusProxy *proxy;
	GDBusConnection *conn;
	GError *error = NULL;

	conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	g_assert_no_error(error);

	proxy = g_dbus_proxy_new_sync(conn,			/* GDBusConnection */
				      G_DBUS_PROXY_FLAGS_NONE,	/* GDBusProxyFlags */
				      NULL,						/* GDBusInterfaceInfo */
				      SERVER_BUS_NAME,			/* name */
				      SERVER_OBJECT_PATH,		/* object path */
				      SERVER_INTERFACE_NAME,	/* interface */
				      NULL,						/* GCancellable */
				      &error);					/* GError */
	g_assert_no_error(error);

	/* invoke server methods */
	invokeServerMethod(proxy);

	g_object_unref(proxy);
	g_object_unref(conn);
	return 0;
}