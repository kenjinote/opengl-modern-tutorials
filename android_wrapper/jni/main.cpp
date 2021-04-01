/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012 Sylvain Beucler
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "wikibooks-android_wrapper", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "wikibooks-android_wrapper", __VA_ARGS__))


/**
 * Our saved state data.
 */
struct saved_state {
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct vpad_state {
    bool on;
    bool left;
    bool right;
    bool up;
    bool down;
};
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;

    int miniglutInit;
    int keyboard_metastate;
    bool need_redisplay;
    struct vpad_state vpad;
    bool in_mmotion;
};


/* <MiniGLUT> */
#include "GL/glut.h"
void miniglutDefaultOnReshapeCallback(int width, int height) {
    LOGI("miniglutDefaultOnReshapeCallback: w=%d, h=%d", width, height);
    glViewport(0, 0, width, height);
}
static void (*miniglutDisplayCallback)(void) = NULL;
static void (*miniglutIdleCallback)(void) = NULL;
static void (*miniglutReshapeCallback)(int,int) = miniglutDefaultOnReshapeCallback;
static void (*miniglutSpecialCallback)(int,int,int) = NULL;
static void (*miniglutSpecialUpCallback)(int,int,int) = NULL;
static void (*miniglutMouseCallback)(int,int,int,int) = NULL;
static void (*miniglutMotionCallback)(int,int) = NULL;

static unsigned int miniglutDisplayMode = 0;
static struct engine engine;
#include <sys/time.h>  /* gettimeofday */
static long miniglutStartTimeMillis = 0;
static int miniglutTermWindow = 0;
#include <stdlib.h>  /* exit */
#include <stdio.h>  /* BUFSIZ */

/* Copied from android_native_app_glue.c to inject missing resize
   event */
#include <unistd.h>
static void android_app_write_cmd(struct android_app* android_app, int8_t cmd) {
    if (write(android_app->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd)) {
        LOGI("Failure writing android_app cmd: %s\n", strerror(errno));
    }
}

#include <android/native_window.h>
static void onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window) {
    struct android_app* android_app = (struct android_app*)activity->instance;
    LOGI("onNativeWindowResized: %p\n", activity);
    // Sent an event to the queue so it gets handled in the app thread
    // after other waiting events, rather than asynchronously in the
    // native_app_glue event thread:
    android_app_write_cmd(android_app, APP_CMD_WINDOW_RESIZED);
}
static void onContentRectChanged(ANativeActivity* activity, const ARect* rect) {
    LOGI("onContentRectChanged: l=%d,t=%d,r=%d,b=%d", rect->left, rect->top, rect->right, rect->bottom);
    // Make Android realize the screen size changed, needed when the
    // GLUT app refreshes only on event rather than in loop.  Beware
    // that we're not in the GLUT thread here, but in the event one.
    glutPostRedisplay();
}
/* </MiniGLUT> */


/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    LOGI("engine_init_display");
    engine->miniglutInit = 1;

    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    // Ensure OpenGLES 2.0 context (mandatory)
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
	    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	    EGL_DEPTH_SIZE, (miniglutDisplayMode & GLUT_DEPTH) ? 24 : 0,
            EGL_NONE
    };
    EGLint w, h, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    // TODO : apply miniglutDisplayMode
    //        (GLUT_DEPTH already applied in attribs[] above)

    eglInitialize(display, 0, 0);

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    // Ensure OpenGLES 2.0 context (mandatory)
    static const EGLint ctx_attribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    return 0;
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
/* Cf. http://developer.android.com/reference/android/view/KeyEvent.html */
enum {
    AKEYCODE_MOVE_HOME=122,
    AKEYCODE_MOVE_END=123,
    AKEYCODE_F1=131,
    AKEYCODE_F2=132,
    AKEYCODE_F3=133,
};
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    // TODO: repeated events have 2 issues:
    // - down and up events happen most often at the exact same time
    //   (down/up/wait/down/up rather than down/wait/down/wait/up
    // - getRepeatCount always returns 0
    LOGI("engine_handle_input - type=%d action=%d repeat=%d down=%lld",
	 AInputEvent_getType(event), AKeyEvent_getAction(event),
	 AKeyEvent_getRepeatCount(event), AKeyEvent_getDownTime(event));
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
	int32_t action = AMotionEvent_getAction(event);
	float x = AMotionEvent_getX(event, 0);
	float y = AMotionEvent_getY(event, 0);
	LOGI("motion %.01f,%.01f action=%d", x, y, AMotionEvent_getAction(event));

	/* Virtual arrows PAD */
	// Don't interfere with existing mouse move event
	if (!engine->in_mmotion) {
	    struct vpad_state prev_vpad = engine->vpad;
	    engine->vpad.left = engine->vpad.right
		= engine->vpad.up = engine->vpad.down = false;
	    if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_MOVE) {
		if ((x > 0 && x < 100) && (y > (engine->height - 100) && y < engine->height))
		    engine->vpad.left = true;
		if ((x > 200 && x < 300) && (y > (engine->height - 100) && y < engine->height))
		    engine->vpad.right = true;
		if ((x > 100 && x < 200) && (y > (engine->height - 100) && y < engine->height))
		    engine->vpad.down = true;
		if ((x > 100 && x < 200) && (y > (engine->height - 200) && y < (engine->height - 100)))
		    engine->vpad.up = true;
	    }
	    if (action == AMOTION_EVENT_ACTION_DOWN && 
		(engine->vpad.left || engine->vpad.right || engine->vpad.down || engine->vpad.up))
		engine->vpad.on = true;
	    if (action == AMOTION_EVENT_ACTION_UP)
		engine->vpad.on = false;
	    if (prev_vpad.left != engine->vpad.left
		|| prev_vpad.right != engine->vpad.right
		|| prev_vpad.up != engine->vpad.up
		|| prev_vpad.down != engine->vpad.down
		|| prev_vpad.on != engine->vpad.on) {
		if (miniglutSpecialCallback != NULL) {
		    if (prev_vpad.left == false && engine->vpad.left == true)
			miniglutSpecialCallback(GLUT_KEY_LEFT, x, y);
		    else if (prev_vpad.right == false && engine->vpad.right == true)
			miniglutSpecialCallback(GLUT_KEY_RIGHT, x, y);
		    else if (prev_vpad.up == false && engine->vpad.up == true)
			miniglutSpecialCallback(GLUT_KEY_UP, x, y);
		    else if (prev_vpad.down == false && engine->vpad.down == true)
			miniglutSpecialCallback(GLUT_KEY_DOWN, x, y);
		}
		if (miniglutSpecialUpCallback != NULL) {
		    if (prev_vpad.left == true && engine->vpad.left == false)
			miniglutSpecialUpCallback(GLUT_KEY_LEFT, x, y);
		    if (prev_vpad.right == true && engine->vpad.right == false)
			miniglutSpecialUpCallback(GLUT_KEY_RIGHT, x, y);
		    if (prev_vpad.up == true && engine->vpad.up == false)
			miniglutSpecialUpCallback(GLUT_KEY_UP, x, y);
		    if (prev_vpad.down == true && engine->vpad.down == false)
			miniglutSpecialUpCallback(GLUT_KEY_DOWN, x, y);
		}
		return 1;
	    }
	}

	/* Normal mouse events */
	if (!engine->vpad.on) {
	    engine->state.x = x;
	    engine->state.y = y;
	    LOGI("Changed mouse position: %d,%d", x, y);
	    if (action == AMOTION_EVENT_ACTION_DOWN && miniglutMouseCallback != NULL) {
		engine->in_mmotion = true;
		miniglutMouseCallback(GLUT_LEFT_BUTTON, GLUT_DOWN, x, y);
	    } else if (action == AMOTION_EVENT_ACTION_UP && miniglutMouseCallback != NULL) {
		engine->in_mmotion = false;
		miniglutMouseCallback(GLUT_LEFT_BUTTON, GLUT_UP, x, y);
	    } else if (action == AMOTION_EVENT_ACTION_MOVE && miniglutMotionCallback != NULL) {
		miniglutMotionCallback(x, y);
	    }
	}

	return 1;
    } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
	// Note: Android generates repeat events when key is left
	// pressed - just what like GLUT expects
	engine->keyboard_metastate = AKeyEvent_getMetaState(event);
	if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN) {
	    int32_t code = AKeyEvent_getKeyCode(event);
	    LOGI("keydown, code: %d", code);
	    if (miniglutSpecialCallback != NULL) {
		switch (code) {
		case AKEYCODE_F1:
		    miniglutSpecialCallback(GLUT_KEY_F1, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_F2:
		    miniglutSpecialCallback(GLUT_KEY_F2, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_F3:
		    miniglutSpecialCallback(GLUT_KEY_F3, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_MOVE_HOME:
		    miniglutSpecialCallback(GLUT_KEY_HOME, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_MOVE_END:
		    miniglutSpecialCallback(GLUT_KEY_END, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_DPAD_UP:
		    miniglutSpecialCallback(GLUT_KEY_UP, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_DPAD_DOWN:
		    miniglutSpecialCallback(GLUT_KEY_DOWN, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_DPAD_LEFT:
		    miniglutSpecialCallback(GLUT_KEY_LEFT, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_DPAD_RIGHT:
		    miniglutSpecialCallback(GLUT_KEY_RIGHT, engine->state.x, engine->state.y);
		    break;
		default:
		    // Let the system handle other keyevent (in
		    // particular the Back key)
		    return 0;
		}
		return 1;
	    }
	} else if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP) {
	    int32_t code = AKeyEvent_getKeyCode(event);
	    LOGI("keyup, code: %d", code);
	    if (miniglutSpecialUpCallback != NULL) {
		switch (code) {
		case AKEYCODE_F1:
		    miniglutSpecialUpCallback(GLUT_KEY_F1, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_F2:
		    miniglutSpecialUpCallback(GLUT_KEY_F2, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_F3:
		    miniglutSpecialUpCallback(GLUT_KEY_F3, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_MOVE_HOME:
		    miniglutSpecialUpCallback(GLUT_KEY_HOME, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_MOVE_END:
		    miniglutSpecialUpCallback(GLUT_KEY_END, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_DPAD_UP:
		    miniglutSpecialUpCallback(GLUT_KEY_UP, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_DPAD_DOWN:
		    miniglutSpecialUpCallback(GLUT_KEY_DOWN, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_DPAD_LEFT:
		    miniglutSpecialUpCallback(GLUT_KEY_LEFT, engine->state.x, engine->state.y);
		    break;
		case AKEYCODE_DPAD_RIGHT:
		    miniglutSpecialUpCallback(GLUT_KEY_RIGHT, engine->state.x, engine->state.y);
		    break;
		default:
		    // Let the system handle other keyevent (in
		    // particular the Back key)
		    return 0;
		}
		return 1;
	    }
	}
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            //engine_term_display(engine);
            miniglutTermWindow = 1;
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                        engine->accelerometerSensor, (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
            }
            break;
        case APP_CMD_CONFIG_CHANGED:
	    // Handle rotation / orientation change

	    // Not needed:
	    // sthing = AConfiguration_getOrientation(app->config);

	    // Too early, should be called in onSurfaceChanged, but it
	    // seems we're not called by android_app_NativeActivity.cpp :/
	    // The screen we have in app->window is not rotated yet
	    // onNativeWindowResized(app->activity, app->window);
	    break;
    case APP_CMD_WINDOW_RESIZED:
	{
	    int32_t newWidth = ANativeWindow_getWidth(engine->app->window);
	    int32_t newHeight = ANativeWindow_getHeight(engine->app->window);
	    LOGI("APP_CMD_WINDOW_RESIZED-engine: w=%d, h=%d", newWidth, newHeight);
	    engine->width = newWidth;
	    engine->height = newHeight;
	    engine->need_redisplay = true;
	    miniglutReshapeCallback(newWidth, newHeight);
	}
    }
}

void print_info_paths(struct android_app* state_param) {
    // Get usable JNI context
    JNIEnv* env = state_param->activity->env;
    JavaVM* vm = state_param->activity->vm;
    vm->AttachCurrentThread(&env, NULL);

    jclass activityClass = env->GetObjectClass(state_param->activity->clazz);

    jclass fileClass = env->FindClass("java/io/File");
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");

    {
	// /data/data/org.wikibooks.OpenGL/files
	jmethodID method = env->GetMethodID(activityClass, "getFilesDir", "()Ljava/io/File;");
	jobject file = env->CallObjectMethod(state_param->activity->clazz, method);
	jstring jpath = (jstring)env->CallObjectMethod(file, getAbsolutePath);
	const char* dir = env->GetStringUTFChars(jpath, NULL);
	LOGI("%s", dir);
	env->ReleaseStringUTFChars(jpath, dir);
    }

    {
	// /data/data/org.wikibooks.OpenGL/cache
	jmethodID method = env->GetMethodID(activityClass, "getCacheDir", "()Ljava/io/File;");
	jobject file = env->CallObjectMethod(state_param->activity->clazz, method);
	jstring jpath = (jstring)env->CallObjectMethod(file, getAbsolutePath);
	const char* dir = env->GetStringUTFChars(jpath, NULL);
	LOGI("%s", dir);
	env->ReleaseStringUTFChars(jpath, dir);
    }

    // getExternalCacheDir -> ApplicationContext: unable to create external cache directory

    {
	// /data/app/org.wikibooks.OpenGL-X.apk
	// /mnt/asec/org.wikibooks.OpenGL-X/pkg.apk
	jmethodID method = env->GetMethodID(activityClass, "getPackageResourcePath", "()Ljava/lang/String;");
	jstring jpath = (jstring)env->CallObjectMethod(state_param->activity->clazz, method);
	const char* dir = env->GetStringUTFChars(jpath, NULL);
	LOGI("%s", dir);
	env->ReleaseStringUTFChars(jpath, dir);
    }

    {
	// /data/app/org.wikibooks.OpenGL-X.apk
	// /mnt/asec/org.wikibooks.OpenGL-X/pkg.apk
	jmethodID method = env->GetMethodID(activityClass, "getPackageCodePath", "()Ljava/lang/String;");
	jstring jpath = (jstring)env->CallObjectMethod(state_param->activity->clazz, method);
	const char* dir = env->GetStringUTFChars(jpath, NULL);
	LOGI("%s", dir);
	env->ReleaseStringUTFChars(jpath, dir);
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
struct android_app* state;
#include <unistd.h>
#include <android/asset_manager.h>
extern int main(int argc, char* argv[]);
void android_main(struct android_app* state_param) {
    LOGI("android_main");
    
    // Make sure glue isn't stripped.
    app_dummy();

    state = state_param;

    // Register window resize callback
    state_param->activity->callbacks->onNativeWindowResized = onNativeWindowResized;
    state_param->activity->callbacks->onContentRectChanged = onContentRectChanged;

    // Get usable JNI context
    JNIEnv* env = state_param->activity->env;
    JavaVM* vm = state_param->activity->vm;
    vm->AttachCurrentThread(&env, NULL);

    // Get a handle on our calling NativeActivity instance
    jclass activityClass = env->GetObjectClass(state_param->activity->clazz);

    // Get path to cache dir (/data/data/org.wikibooks.OpenGL/cache)
    jmethodID getCacheDir = env->GetMethodID(activityClass, "getCacheDir", "()Ljava/io/File;");
    jobject file = env->CallObjectMethod(state_param->activity->clazz, getCacheDir);
    jclass fileClass = env->FindClass("java/io/File");
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring jpath = (jstring)env->CallObjectMethod(file, getAbsolutePath);
    const char* app_dir = env->GetStringUTFChars(jpath, NULL);

    // chdir in the application cache directory
    LOGI("app_dir: %s", app_dir);
    chdir(app_dir);
    env->ReleaseStringUTFChars(jpath, app_dir);
    print_info_paths(state_param);

    // Pre-extract assets, to avoid Android-specific file opening
    AAssetManager* mgr = state_param->activity->assetManager;
    AAssetDir* assetDir = AAssetManager_openDir(mgr, "");
    const char* filename = (const char*)NULL;
    while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
	AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_STREAMING);
	char buf[BUFSIZ];
	int nb_read = 0;
	FILE* out = fopen(filename, "w");
	while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0)
	    fwrite(buf, nb_read, 1, out);
	fclose(out);
	AAsset_close(asset);
    }
    AAssetDir_close(assetDir);

    // Call user's main
    char progname[5] = "self";
    char* argv[] = {progname};
    main(1, argv);

    // Destroy OpenGL context
    engine_term_display(&engine);

    LOGI("android_main: end");
    exit(0);
}

void process_events() {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

	// We loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                            &event, 1) > 0) {
		      ; // Don't spam the logs
		      //LOGI("accelerometer: x=%f y=%f z=%f",
                      //          event.acceleration.x, event.acceleration.y,
                      //          event.acceleration.z);
                    }
                }
            }
        }
}

void glutMainLoop() {
    LOGI("glutMainLoop");

    int32_t lastWidth = -1;
    int32_t lastHeight = -1;

    // loop waiting for stuff to do.
    while (1) {
        process_events();

	// Check if we are exiting.
	//if (state->destroyRequested != 0) {
	//    break;
	//}

	// Check if we are exiting.
	if (miniglutTermWindow != 0) {
	    // GLUT doesn't provide a callback to restore a lost
	    // context, so we just quit the application
	    break;
	}

	// I can't seem to get onSurfaceChanged or
	// onNativeWindowResized events with Android 2.3's
	// NativeActivity.  I suspect a bug.  Let's work-around it:
	int32_t newWidth = ANativeWindow_getWidth(engine.app->window);
	int32_t newHeight = ANativeWindow_getHeight(engine.app->window);
	if (newWidth != lastWidth || newHeight != lastHeight) {
	    lastWidth = newWidth;
	    lastHeight = newHeight;
	    onNativeWindowResized(engine.app->activity, engine.app->window);
	    // Process new resize event :)
	    continue;
	}
	
	if (miniglutIdleCallback != NULL)
	    miniglutIdleCallback();

	if (miniglutDisplayCallback != NULL)
	    if (engine.need_redisplay) {
		engine.need_redisplay = false;
		miniglutDisplayCallback();
	    }
    }

    LOGI("glutMainLoop: end");
}
//END_INCLUDE(all)


void glutInit( int* pargc, char** argv ) {
    LOGI("glutInit");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    miniglutStartTimeMillis = tv.tv_sec * 1000 + tv.tv_usec/1000;
}

void glutInitContextVersion(int majorVersion, int minorVersion) {
    LOGI("glutInitContextVersion");
}

void glutInitDisplayMode( unsigned int displayMode ) {
    LOGI("glutInitDisplayMode");
    miniglutDisplayMode = displayMode;
}

void glutInitWindowSize( int width, int height ) {
    LOGI("glutInitWindowSize");
    // TODO?
}

int glutCreateWindow( const char* title ) {
    LOGI("glutCreateWindow");
    static int window_id = 0;
    if (window_id == 0) {
	window_id++;
    } else {
	// Only one full-screen window
	return 0;
    }

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
            state->looper, LOOPER_ID_USER, NULL, NULL);

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    // Wait until window is available and OpenGL context is created
    while (engine.miniglutInit == 0)
	process_events();

    if (engine.display != EGL_NO_DISPLAY)
	return window_id;
    else
	return 0;
}

void glutDisplayFunc( void (* callback)( void ) ) {
    LOGI("glutDisplayFunc");
    miniglutDisplayCallback = callback;
}

void glutIdleFunc( void (* callback)( void ) ) {
    LOGI("glutIdleFunc");
    miniglutIdleCallback = callback;
}

void glutReshapeFunc(void(*callback)(int,int)) {
    LOGI("glutReshapeFunc");
    miniglutReshapeCallback = callback;
}

void glutSpecialFunc(void(*callback)(int,int,int)) {
    LOGI("glutSpecialFunc");
    miniglutSpecialCallback = callback;
}

void glutSpecialUpFunc(void(*callback)(int,int,int)) {
    LOGI("glutSpecialUpFunc");
    miniglutSpecialUpCallback = callback;
}

void glutMouseFunc(void (* callback)( int, int, int, int )) {
    LOGI("glutMouseFunc");
    miniglutMouseCallback = callback;
}

void glutMotionFunc(void (* callback)( int, int )) {
    LOGI("glutMotionFunc");
    miniglutMotionCallback = callback;
}

void glutSwapBuffers() {
    //LOGI("glutSwapBuffers");
    eglSwapBuffers(engine.display, engine.surface);
}

void glutWarpPointer(int x, int y) {
    // No-op - no mouse pointer
}
void glutSetCursor(int cursor) {
    // No-op - no mouse pointer
}
void glutPassiveMotionFunc( void (* callback)( int, int ) ) {
    // No-op - no mouse pointer
}

int glutGet( GLenum query ) {
    //LOGI("glutGet");
    if (query == GLUT_ELAPSED_TIME) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long cur_time = tv.tv_sec * 1000 + tv.tv_usec/1000;
	//LOGI("glutGet: %d", (int) cur_time - miniglutStartTimeMillis);
	return cur_time - miniglutStartTimeMillis;
    } else if (query == GLUT_WINDOW_WIDTH) {
	return engine.width;
    } else if (query == GLUT_WINDOW_HEIGHT) {
	return engine.height;
    }
}

/**
 * Get keyboard ctrl/shift/alt state
 */
/** Cf. http://developer.android.com/reference/android/view/KeyEvent.html */
enum { AMETA_CTRL_ON = 0x00001000 };
int glutGetModifiers() {
    int state = 0;
    if (engine.keyboard_metastate & AMETA_SHIFT_ON)
	state = state | GLUT_ACTIVE_SHIFT;
    if (engine.keyboard_metastate & AMETA_CTRL_ON)
	state |= GLUT_ACTIVE_CTRL;
    if (engine.keyboard_metastate & AMETA_ALT_ON)
	state |= GLUT_ACTIVE_ALT;
    return state;
}

void glutPostRedisplay() {
    engine.need_redisplay = true;
}

// Local Variables: ***
// c-basic-offset:4 ***
// End: ***
