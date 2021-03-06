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
pthread_cond_t gestureCond = PTHREAD_COND_INITIALIZER;

bool gestureDetected = false;

Leap::Controller controller;
Leap::GestureList gestures;

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

	//g_object_unref(proxy);
	g_object_unref(conn);

	return proxy;
}

void invokeServerMethod(GDBusProxy *proxy, const gchar* methodName, const gchar* remoteMethod, gint16 x, gint16 y) {
	
	gchar *str;
	GVariant *result;
	GVariant *parameters;
	GError *error = NULL;

	if(g_strcmp0 (methodName, "GestureManager") == 0) {

		parameters = g_variant_new("(s)", remoteMethod);

		g_printf("Calling GestureManager(%s)\n", remoteMethod);

		result = g_dbus_proxy_call_sync(proxy, "GestureManager", parameters, 
										G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	}

	if(g_strcmp0 (methodName, "ClickManager") == 0) { 

		parameters = g_variant_new("(n)", x);

		g_printf("Calling ClickManager(Button%d)\n", x);
		result = g_dbus_proxy_call_sync(proxy, "ClickManager", parameters, 
										G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);	
	}

	if(g_strcmp0 (methodName, "MotionManager") == 0) {
		parameters = g_variant_new("(nn)", x, y);

		g_printf("Calling MotionManager(x: %d, y: %d)\n", x, y);
		result = g_dbus_proxy_call_sync(proxy, "MotionManager", parameters, 
										G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	}		

	g_assert_no_error(error);
	g_variant_get(result, "(s)", &str);
	g_printf("The server answered: '%s'\n\n", str);
	g_free(str);
	g_variant_unref(result);
}	

void* mouseThread(void* arg) {

	TaskPar *tp;

	GDBusProxy *proxy;

	Leap::Frame frame;
	Leap::HandList hands;

	tp = (TaskPar *) arg;
	set_period(tp);

	proxy = createProxy();

	controller.enableGesture(Leap::Gesture::TYPE_CIRCLE);
	controller.enableGesture(Leap::Gesture::TYPE_SWIPE);
	controller.enableGesture(Leap::Gesture::TYPE_KEY_TAP);

	controller.config().setFloat("Gesture.KeyTap.MinDownVelocity", 10.0);
	controller.config().setFloat("Gesture.KeyTap.HistorySeconds", .2);
	controller.config().setFloat("Gesture.KeyTap.MinDistance", 3.0);
	controller.config().save();

	while(true) {

		frame = controller.frame();

		hands = frame.hands();

		pthread_mutex_lock(&mutex);
        gestures = frame.gestures();
        pthread_mutex_unlock(&mutex);
		

		for(Leap::HandList::const_iterator h = hands.begin(); h != hands.end(); h++) {
    		const Leap::Hand hand = *h;
			if(hand.isRight()) {
				Leap::Vector position = hand.palmPosition();
				
				gint16 x = (int)position.x;
				gint16 y = (int)position.z;

				invokeServerMethod(proxy, "MotionManager", NULL, x, y);
			}
		}

		if (!gestures.isEmpty()) {
			pthread_mutex_lock(&mutex);
        	gestureDetected = true;
        	pthread_cond_signal(&gestureCond);
        	pthread_mutex_unlock(&mutex);
		}

		wait_for_period(tp);
	}

}

void* gestureThread(void* arg) {

	TaskPar *tp;

	tp = (TaskPar *) arg;
	set_period(tp);

	GDBusProxy *proxy;
	proxy = createProxy();

	while(true) {

		pthread_mutex_lock(&mutex);
        while (!gestureDetected)
            pthread_cond_wait(&gestureCond, &mutex);
        Leap::GestureList tmpGestures = gestures;
        pthread_mutex_unlock(&mutex);

		for(Leap::GestureList::const_iterator gl = tmpGestures.begin(); gl != tmpGestures.end(); gl++) {
    		const Leap::Gesture gesture = *gl;

    		Leap::HandList hands = gesture.hands();

    		for(Leap::HandList::const_iterator hl = hands.begin(); hl != hands.end(); hl ++){
    			Leap::Hand hand = *hl;
    			if(hand.isRight()){
    				if(gesture.type() == Leap::Gesture::TYPE_KEY_TAP) { 
						printf("Detected mouse click\n");

						Leap::PointableList pointables;
						 
						pointables = hand.pointables();

						int i = 0;
						int min = 1000;
						int fingerIndex = -1;					

						for(Leap::PointableList::const_iterator pl = pointables.begin(); pl != pointables.end(); pl++){
							Leap::Pointable pointable = *pl;

							int position = pointable.tipPosition().y;
							std::cout << "position y: " << position << std::endl;
							if(position < min){
								min = position;
								fingerIndex = i;
							}

							i++;
						}
						
						if(fingerIndex == Leap::Finger::TYPE_THUMB)
							invokeServerMethod(proxy, "ClickManager", NULL, 1, 0);
						else if(fingerIndex == Leap::Finger::TYPE_RING)
							invokeServerMethod(proxy, "ClickManager", NULL, 3, 0);
	
						continue;
					}
    			}
    			else { // if hand is left
					switch(gesture.type()) {
						//Handle circle gestures
						case Leap::Gesture::TYPE_CIRCLE:
							if(gesture.state() == Leap::Gesture::STATE_UPDATE) {
								printf("Detected Gesture: TYPE_CIRCLE\n");
								invokeServerMethod(proxy, "GestureManager", "Reboot", 0, 0);
							}
				            break;
				        //Handle key tap gestures
				        case Leap::Gesture::TYPE_KEY_TAP:
							printf("Detected Gesture: TYPE_KEY_TAP\n");
			        		invokeServerMethod(proxy, "GestureManager", "Hibernate", 0, 0);
				            break;
				        //Handle screen tap gestures
				        case Leap::Gesture::TYPE_SCREEN_TAP: 
							printf("Detected Gesture: TYPE_SCREEN_TAP\n");
							invokeServerMethod(proxy, "GestureManager", "Hibernate", 0, 0);     
				            break;
			            //Handle swipe gestures
				        case Leap::Gesture::TYPE_SWIPE:
				        	if(gesture.state() == Leap::Gesture::STATE_UPDATE) {
				        		printf("Detected Gesture: TYPE_SWIPE\n");
				        		invokeServerMethod(proxy, "GestureManager", "PowerOff", 0, 0);
				        	}  
				            break;
				        //Handle unrecognized gestures
				        default:
				            break;
					}
    			}
    		}

		}

		pthread_mutex_lock(&mutex);
        gestureDetected = false;
        pthread_mutex_unlock(&mutex);

		wait_for_period(tp);
	}

}

bool leap_dbus_main_loop() {

	pthread_t mouseT;
	pthread_t gestureT;

	TaskPar mousePar;
	TaskPar gesturePar;

	/* Set tasks parameters as period, deadline, priority */
	set_taskpar(&mousePar, NULL, NULL, 60, 60, 30);
	set_taskpar(&gesturePar, NULL, NULL, 60, 60, 30);

	/* Create threads */
	pthread_create(&mouseT, NULL, mouseThread, &mousePar);
	pthread_create(&gestureT, NULL, gestureThread, &gesturePar);
	
	/* Wait for threads */
	pthread_join(mouseT,NULL);
	pthread_join(gestureT,NULL);

	return false;
}

int main(int argc,char * argv[]) {	
	
	bool clientIsFinished = false;

	while(!clientIsFinished){

		clientIsFinished = leap_dbus_main_loop();
	}
	
	return 0;
}