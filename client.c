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

/* librealt header */
#include "my_thread_lib.h"

/* define dbus Server interface */
#define SERVER_BUS_NAME 		"org.cbsd.LeapServer"
#define SERVER_OBJECT_PATH 		"/org/cbsd/LeapServer"
#define SERVER_INTERFACE_NAME	"org.cbsd.LeapServer"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
Leap::Controller controller;

GDBusProxy* createProxy() {

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

	//g_object_unref(proxy);
	//g_object_unref(conn);

	return proxy;
}

void invokeServerMethod(GDBusProxy *proxy, const gchar* methodName) {
	GVariant *result;
	GError *error = NULL;
	const gchar *str;
	GVariant *parameters;

	parameters = g_variant_new("(s)", methodName);

	g_printf("Calling GestureManager(%s)\n", methodName);
	result = g_dbus_proxy_call_sync(proxy, "GestureManager", parameters, 
									G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	g_assert_no_error(error);
	g_variant_get(result, "(&s)", &str);
	g_printf("The server answered: '%s'\n\n", str);
	g_variant_unref(result);
}	


void* rightHandThread(void* arg) {

	TaskPar *tp;

	Leap::Frame f;
	Leap::HandList handsInFrame;

	tp = (TaskPar *) arg;
	set_period(tp);

	while(true)
	{
		pthread_mutex_lock(&mutex);
		f = controller.frame();
		pthread_mutex_unlock(&mutex);
		
		handsInFrame = f.hands();
		
		if (deadline_miss(tp)) {
			printf("rightHandThread: %d\n",tp->dmiss);
		}
		wait_for_period(tp);
	}

}

void* leftHandThread(void* arg) {

	TaskPar *tp;

	Leap::Frame frame;
	Leap::HandList hands;
	Leap::GestureList gestures;

	tp = (TaskPar *) arg;
	set_period(tp);

	GDBusProxy *proxy;
	proxy = createProxy();

	controller.enableGesture(Leap::Gesture::TYPE_CIRCLE);
	controller.enableGesture(Leap::Gesture::TYPE_SWIPE);
	controller.enableGesture(Leap::Gesture::TYPE_KEY_TAP);
	controller.enableGesture(Leap::Gesture::TYPE_SCREEN_TAP);

	while(true) {

		pthread_mutex_lock(&mutex);
		frame = controller.frame();
		pthread_mutex_unlock(&mutex);

		hands = frame.hands();
		gestures = frame.gestures();

		for(Leap::HandList::const_iterator h = hands.begin(); h != hands.end(); h++) {
    		const Leap::Hand hand = *h;
			if(hand.isLeft() && !(gestures.isEmpty())) {

				Leap::Gesture gesture = gestures[0];
				
				switch(gesture.type()) {
					//Handle circle gestures
					case Leap::Gesture::TYPE_CIRCLE:
						if(gesture.state() == Leap::Gesture::STATE_STOP) {
							printf("Detected Gesture: TYPE_CIRCLE\n");
							invokeServerMethod(proxy, "Reboot");
						}
			            break;
			        //Handle key tap gestures
			        case Leap::Gesture::TYPE_KEY_TAP:
						printf("Detected Gesture: TYPE_KEY_TAP\n");
		        		invokeServerMethod(proxy, "Hibernate");
			            break;
			        //Handle screen tap gestures
			        case Leap::Gesture::TYPE_SCREEN_TAP: 
						printf("Detected Gesture: TYPE_SCREEN_TAP\n");
						invokeServerMethod(proxy, "Hibernate");     
			            break;
		            //Handle swipe gestures
			        case Leap::Gesture::TYPE_SWIPE:
			        	if(gesture.state() == Leap::Gesture::STATE_STOP) {
			        		printf("Detected Gesture: TYPE_SWIPE\n");
			        		invokeServerMethod(proxy, "PowerOff");
			        	}  
			            break;
			        //Handle unrecognized gestures
			        default:
			            break;
				}
			}
		}

		if (deadline_miss(tp)) {
			printf("leftHandThread: %d\n",tp->dmiss);
		}
		wait_for_period(tp);
	}

}

bool leap_dbus_main_loop() {
	pthread_t rHandT;
	pthread_t lHandT;
	TaskPar rHandPar;
	TaskPar lHandPar;

	set_taskpar(&rHandPar, NULL, NULL, 30, 30, 30);
	set_taskpar(&lHandPar, NULL, NULL, 30, 30, 30);

	/* Create threads */
	pthread_create(&lHandT, NULL, leftHandThread, &lHandPar);
	pthread_create(&rHandT, NULL, rightHandThread, &rHandPar);
	
	/* Wait for threads */
	pthread_join(lHandT,NULL);
	pthread_join(rHandT,NULL);

	return false;
}

int main(void) {
	
	bool clientIsFinished = false;

	while(!clientIsFinished){

		clientIsFinished = leap_dbus_main_loop();
	}
	
	return 0;
}