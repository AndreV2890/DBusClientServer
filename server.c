#include <gio/gio.h>

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVICE_NAME "org.example.LeapServer"
#define OBJECT_PATH "/org/example/LeapServer"


static GDBusNodeInfo *introspectionData = NULL;

/* Introspection data for the service we are exporting */
static const gchar introspectionXML[] =
  "<node>"
  "  <interface name='org.example.LeapServer'>"
  "    <!--<annotation name='org.example.LeapServer.Annotation' value='OnInterface'/>"
  "    <annotation name='org.example.LeapServer.Annotation' value='AlsoOnInterface'/>-->"
  "    <method name='HelloWorld'>"
  "      <annotation name='org.example.LeapServer.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='greeting' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "    <!--<method name='EmitSignal'>"
  "      <arg type='d' name='speed_in_mph' direction='in'>"
  "        <annotation name='org.example.LeapServer.Annotation' value='OnArg'/>"
  "      </arg>"
  "    </method>-->"
  "    <!--<method name='GimmeStdout'/>-->"
  "    <!--<signal name='VelocityChanged'>"
  "      <annotation name='org.example.LeapServer.Annotation' value='Onsignal'/>"
  "      <arg type='d' name='speed_in_mph'/>"
  "      <arg type='s' name='speed_as_string'>"
  "        <annotation name='org.example.LeapServer.Annotation' value='OnArg_NonFirst'/>"
  "      </arg>"
  "    </signal>-->"
  "    <!--<property type='s' name='FluxCapicitorName' access='read'>"
  "      <annotation name='org.example.LeapServer.Annotation' value='OnProperty'>"
  "        <annotation name='org.example.LeapServer.Annotation' value='OnAnnotation_YesThisIsCrazy'/>"
  "      </annotation>"
  "    </property>"
  "    <property type='s' name='Title' access='readwrite'/>"
  "    <property type='s' name='ReadingAlwaysThrowsError' access='read'/>"
  "    <property type='s' name='WritingAlwaysThrowsError' access='readwrite'/>"
  "    <property type='s' name='OnlyWritable' access='write'/>"
  "    <property type='s' name='Foo' access='read'/>"
  "    <property type='s' name='Bar' access='read'/>-->"
  "  </interface>"
  "</node>";

/* --------------------------------------------------------------------------------------- */

static void handleMethodCall (GDBusConnection       *connection,
								const gchar           *sender,
								const gchar           *objectPath,
								const gchar           *interfaceName,
								const gchar           *methodName,
								GVariant              *parameters,
								GDBusMethodInvocation *invocation,
								gpointer               userData) 
{

	if (g_strcmp0 (methodName, "HelloWorld") == 0) {
		const gchar *greeting;

		g_variant_get (parameters, "(&s)", &greeting);

		if (g_strcmp0 (greeting, "Return Unregistered") == 0) {
			g_dbus_method_invocation_return_error (invocation, G_IO_ERROR, G_IO_ERROR_FAILED_HANDLED, 
						   	"As requested, here's a GError not registered (G_IO_ERROR_FAILED_HANDLED)");
		}
		else if (g_strcmp0 (greeting, "Return Registered") == 0) {
			g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_MATCH_RULE_NOT_FOUND,
         					"As requested, here's a GError that is registered (G_DBUS_ERROR_MATCH_RULE_NOT_FOUND)");
		}
		else if (g_strcmp0 (greeting, "Return Raw") == 0) {
			g_dbus_method_invocation_return_dbus_error (invocation,
		                                      "org.example.LeapServer.SomeErrorName",
		                                      "As requested, here's a raw D-Bus error");
		}
		else {
			gchar *response;
			response = g_strdup_printf ("You greeted me with '%s'. Thanks!", greeting);
			g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", response));
			g_free (response);

			GDBusProxy *proxy;
			GDBusConnection *conn;
			GError *error = NULL;
			GVariant *result;

			conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
			g_assert_no_error(error);

			proxy = g_dbus_proxy_new_sync(conn,
									 G_DBUS_PROXY_FLAGS_NONE,
									 NULL,
									 "org.freedesktop.login1",
									 "/org/freedesktop/login1",
									 "org.freedesktop.login1.Manager",
									 NULL,
									 &error);
			g_assert_no_error(error);

			result = g_dbus_proxy_call_sync(proxy, "Reboot", g_variant_new ("(b)", "true"),
											G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

			g_variant_unref(result);
			g_object_unref(proxy);
			g_object_unref(conn);
		}
	}
}

static const GDBusInterfaceVTable interfaceVTable = {
	handleMethodCall
	/*handle_get_property,
	handle_set_property*/
};

static void onBusAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
	printf("Session Bus acquired\n");
	guint registrationId;

	registrationId = g_dbus_connection_register_object (connection,
	                                                   OBJECT_PATH,
	                                                   introspectionData->interfaces[0],
	                                                   &interfaceVTable,
	                                                   NULL,  /* user_data */
	                                                   NULL,  /* user_data_free_func */
	                                                   NULL); /* GError** */
	
	g_assert (registrationId > 0);
}

static void onNameAcquired() {
	printf("Name acquired\n");
}

static void onNameLost() {
	exit(1);
}

int main(void) {
	
	guint busId;
	GMainLoop *loop;

	introspectionData = g_dbus_node_info_new_for_xml (introspectionXML, NULL);
	g_assert (introspectionData != NULL);

	busId = g_bus_own_name(G_BUS_TYPE_SESSION, SERVICE_NAME, G_BUS_NAME_OWNER_FLAGS_NONE, onBusAcquired, onNameAcquired, onNameLost, NULL, NULL);
	
	loop = g_main_loop_new(NULL, false);

	g_main_loop_run(loop);

	g_bus_unown_name(busId);

	return EXIT_SUCCESS;

}