#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* glib headers */
#include <gio/gio.h>

/* X11 headers*/
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define SERVER_VERSION "1.0"
#define LEAP_X_ABS_MAX 250
#define LEAP_Y_ABS_MAX 200

#define BUS_NAME "org.cbsd.LeapServer"
#define OBJECT_PATH "/org/cbsd/LeapServer"

#define LOGIN1_BUS_NAME			"org.freedesktop.login1"
#define LOGIN1_OBJECT_PATH		"/org/freedesktop/login1"
#define LOGIN1_INTEFACE_NAME	"org.freedesktop.login1.Manager"


gint16 monitorWidth;
gint16 monitorHeigth;
gboolean firstTime = TRUE;

static GDBusNodeInfo *introspectionData = NULL;

/* Introspection data for the service we are exporting */
static const gchar introspectionXML[] =
	"<node>"
	"	<interface name='org.cbsd.LeapServer'>"
	"		<method name='GestureManager'>"
	"			<annotation name='org.cbsd.LeapServer.Annotation' value='OnMethod'/>"
	"			<arg type='s' name='methodName' direction='in'/>"
	"			<arg type='s' name='response' direction='out'/>"
	"		</method>"
	"		<method name='ClickManager'>"
	"			<annotation name='org.cbsd.LeapServer.Annotation' value='OnMethod'/>"
	"			<arg type='n' name='button' direction='in'/>"
	"			<arg type='s' name='response' direction='out'/>"
	"		</method>"
	"		<method name='MotionManager'>"
	"			<annotation name='org.cbsd.LeapServer.Annotation' value='OnMethod'/>"
	"			<arg type='n' name='x' direction='in'/>"
	"			<arg type='n' name='y' direction='in'/>"
	"			<arg type='s' name='response' direction='out'/>"
	"		</method>"
	"		<property name='Version' type='s' access='read' />"
	"	</interface>"
	"</node>";

/* ------- Mouse Manager Functions ------- */

/* Get mouse coordinates */
void getMouseCoordinates(Display *display, gint16 *x, gint16 *y) {
	XEvent event;
	XQueryPointer(display, DefaultRootWindow (display),
					&event.xbutton.root, &event.xbutton.window,
					&event.xbutton.x_root, &event.xbutton.y_root,
					&event.xbutton.x, &event.xbutton.y,
					&event.xbutton.state);
	*x = event.xbutton.x;
	*y = event.xbutton.y;
}

/* Simulate mouse click */
void clickMouse (Display *display, gint16 button) {
	// Create and setting up the event
	XEvent event;
	memset (&event, 0, sizeof (event));
	event.xbutton.button = button;
	event.xbutton.same_screen = True;
	event.xbutton.subwindow = DefaultRootWindow (display);
	while (event.xbutton.subwindow) {
		event.xbutton.window = event.xbutton.subwindow;
		XQueryPointer (display, event.xbutton.window,
		&event.xbutton.root, &event.xbutton.subwindow,
		&event.xbutton.x_root, &event.xbutton.y_root,
		&event.xbutton.x, &event.xbutton.y,
		&event.xbutton.state);
	}

	// Press
	event.type = ButtonPress;
	if (XSendEvent (display, PointerWindow, True, ButtonPressMask, &event) == 0)
		fprintf (stderr, "Error to send the event!\n");
	XFlush (display);
	usleep (1);

	// Release
	event.type = ButtonRelease;
	if (XSendEvent (display, PointerWindow, True, ButtonReleaseMask, &event) == 0)
		fprintf (stderr, "Error to send the event!\n");
	XFlush (display);
	usleep (1);
}

/* Move mouse pointer to x and y coordinate */
void moveMouse (Display *display, gint16 x, gint16 y) {

	printf("Received x: %d y: %d\n\n", x ,y);

	gint16 xScale = monitorWidth/(2*LEAP_X_ABS_MAX);
	gint16 yScale = monitorHeigth/(2*LEAP_Y_ABS_MAX);

	if(x < 0)
		x = LEAP_X_ABS_MAX +x;
	else 
		x += LEAP_X_ABS_MAX;

	if(y < 0)
		y = LEAP_Y_ABS_MAX +y;
	else 
		y += LEAP_Y_ABS_MAX;

	x *= xScale;
	if(x > monitorWidth)
		x = monitorWidth;

	y *= yScale;
	if(y > monitorHeigth)
		y = monitorHeigth;

	printf("Set new mouse position to x: %d y: %d\n\n", x ,y);

	XWarpPointer(display, None, RootWindow(display, DefaultScreen(display)), 0, 0, 0, 0, x, y);

	XFlush (display);
	usleep(1);

}

/* Server helper functions */
void Hibernate(GDBusProxy *proxy, GDBusMethodInvocation *invocation){
	GVariant *result;
	GError *error = NULL;

	/* check is system che be hibernated */
	result = g_dbus_proxy_call_sync(proxy, "CanHibernate", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	const gchar *response;
	g_variant_get(result, "(&s)", &response);

	if (g_strcmp0 (response, "yes") != 0) {
		printf("User cannot Hibernate\n");

		gchar *str;
		str = g_strdup_printf ("CanHibernate(): '%s'.", response);
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		g_free(str);
	}
	else {
		printf("Prepare to hibernate the system\n");

		gchar *str;
		str = g_strdup_printf ("CanHibernate(): '%s'.", response);
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		g_free(str);

		result = g_dbus_proxy_call_sync(proxy, "Hibernate", g_variant_new ("(b)", "true"),
										G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	}

	g_variant_unref(result);
}


void PowerOff(GDBusProxy *proxy, GDBusMethodInvocation *invocation){
	GVariant *result;
	GError *error = NULL;

	/* check is system che be powered off */
	result = g_dbus_proxy_call_sync(proxy, "CanPowerOff", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	const gchar *response;
	g_variant_get(result, "(&s)", &response);

	if (g_strcmp0 (response, "yes") != 0) {
		printf("User cannot PowerOff\n");

		gchar *str;
		str = g_strdup_printf ("CanPowerOff(): '%s'.", response);
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		g_free(str);
	}
	else {
		printf("Prepare to poweroff the system\n");

		gchar *str;
		str = g_strdup_printf ("CanPowerOff(): '%s'.", response);
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		g_free(str);

		result = g_dbus_proxy_call_sync(proxy, "PowerOff", g_variant_new ("(b)", "true"),
										G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	}

	g_variant_unref(result);
}

void Reboot(GDBusProxy *proxy, GDBusMethodInvocation *invocation){
	GVariant *result;
	GError *error = NULL;

	/* check is system che be rebooted */
	result = g_dbus_proxy_call_sync(proxy, "CanReboot", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	const gchar *response;
	g_variant_get(result, "(&s)", &response);

	if (g_strcmp0 (response, "yes") != 0) {
		printf("User cannot Reboot\n");

		gchar *str;
		str = g_strdup_printf ("CanReboot(): '%s'.\n", response);
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		g_free(str);
	}
	else {
		printf("Prepare to reboot the system\n");

		gchar *str;
		str = g_strdup_printf ("CanReboot(): '%s'.", response);
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		g_free(str);

		result = g_dbus_proxy_call_sync(proxy, "Reboot", g_variant_new ("(b)", "true"),
										G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	}

	g_variant_unref(result);
}

/* Methods call handler */
static GVariant* handleGetProperty(GDBusConnection  *connection,
								const gchar    *sender,
								const gchar    *objectPath,
								const gchar    *interfaceName,
								const gchar    *propertyName,
								GError         **error,
								gpointer        userData)
{
	GVariant *ret = NULL;

	if (g_strcmp0 (propertyName, "Version") == 0)
		ret = g_variant_new_string (SERVER_VERSION);

	return ret;
}

static void handleMethodCall (GDBusConnection         *connection,
								const gchar           *sender,
								const gchar           *objectPath,
								const gchar           *interfaceName,
								const gchar           *methodName,
								GVariant              *parameters,
								GDBusMethodInvocation *invocation,
								gpointer               userData) 
{

	if (g_strcmp0 (methodName, "GestureManager") == 0) {

		/* create a Proxy of org.freedesktop.login1 */
		GDBusProxy *proxy;
		GDBusConnection *conn;
		GError *error = NULL;

		conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
		g_assert_no_error(error);

		proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL, LOGIN1_BUS_NAME, 
										LOGIN1_OBJECT_PATH, LOGIN1_INTEFACE_NAME, NULL, &error);
		g_assert_no_error(error);

		/* get methodName from message body */
		const gchar *body;
		g_variant_get (parameters, "(&s)", &body);

		/* call the method invoked by client */
		if(g_strcmp0(body, "Reboot") == 0){
			printf("Client has called Reboot\n");
			Reboot(proxy, invocation);
		}
		if(g_strcmp0(body, "PowerOff") == 0){
			printf("Client has called Reboot\n");
			PowerOff(proxy, invocation);
		}
		if(g_strcmp0(body, "Hibernate") == 0){
			printf("Client has called Hibernate\n");
			Hibernate(proxy, invocation);
		}

		/* unref variables */
		g_object_unref(proxy);
		g_object_unref(conn);

	}

	if (g_strcmp0 (methodName, "ClickManager") == 0) {
		gint16 button;

		g_variant_get (parameters, "(n)", &button);

		// Open X display
		Display *display = XOpenDisplay (NULL);
		if (display == NULL) {
			fprintf (stderr, "Can't open display!\n");
			gchar *str;
			str = g_strdup_printf ("ClickManager(): X11 error -> Can't open display!.");
			g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		}

		clickMouse(display, button);

		gchar *str;
		str = g_strdup_printf ("ClickManager().");
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		g_free(str);

		XCloseDisplay (display);

	}

	if(g_strcmp0 (methodName, "MotionManager") == 0) {
		gint16 x;
		gint16 y;
		g_variant_get (parameters, "(nn)", &x, &y);

		// Open X display
		Display *display = XOpenDisplay (NULL);
		if (display == NULL) {
			fprintf (stderr, "Can't open display!\n");
			gchar *str;
			str = g_strdup_printf ("MotionManager(): X11 error -> Can't open display!.");
			g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		}

		moveMouse(display, x, y);

		gchar *str;
		str = g_strdup_printf ("MotionManager().");
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", str));
		g_free(str);

		XCloseDisplay (display);
	}
}

static const GDBusInterfaceVTable interfaceVTable = {
	handleMethodCall,
	handleGetProperty
	/*handle_set_property*/
};

static void onBusAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
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
	printf("%s acquired\n", BUS_NAME);
}

static void onNameLost() {
	exit(1);
}

int main(void) {

	/* Acquire information about monitor */
	Display *display;
	Screen *screen;

	display = XOpenDisplay(NULL);
	screen = ScreenOfDisplay(display, 0);

	printf("Monitor Pixels: %dx%d\n", screen->width, screen->height);
	monitorWidth = screen->width;
	monitorHeigth = screen->height;

	/* Move mouse pointer in the monitor center */
	moveMouse(display, screen->width/2, screen->height/2);

	/* Instanciate bus on DBUS */
	guint busId;
	GMainLoop *loop;

	introspectionData = g_dbus_node_info_new_for_xml (introspectionXML, NULL);
	g_assert (introspectionData != NULL);

	busId = g_bus_own_name(G_BUS_TYPE_SESSION, BUS_NAME, G_BUS_NAME_OWNER_FLAGS_NONE, onBusAcquired, onNameAcquired, onNameLost, NULL, NULL);
	
	loop = g_main_loop_new(NULL, false);

	g_main_loop_run(loop);

	g_bus_unown_name(busId);

	return EXIT_SUCCESS;

}