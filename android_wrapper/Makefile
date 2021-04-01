# Overridable .mk:
APP_BUILD_SCRIPT=jni/Android.mk

all:
	ndk-build NDK_DEBUG=1 V=1 NDK_MODULE_PATH=$$NDK_MODULE_PATH:. APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT)
	android update project --name wikibooks-opengl --path . --target "android-10"
	ant debug

install:
	#adb install -r bin/wikibooks-opengl-debug.apk
	ant installd
	adb shell am start -a android.intenon.MAIN -n org.wikibooks.OpenGL/android.app.NativeActivity

clean:
	ndk-build clean NDK_MODULE_PATH=$$NDK_MODULE_PATH:.
	ant clean
	rm -rf obj/ libs/ build.xml proguard.cfg proguard-project.txt project.properties local.properties
