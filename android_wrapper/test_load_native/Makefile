all:
	android update project --name test_native_activity --path . --target "android-10"
	ant debug

install:
	ant installd
#	adb shell am start -a android.intenon.MAIN -n com.example.test_native_activity/com.example.test_native_activity.Main

clean:
	ant clean
	rm -rf obj/ libs/ build.xml proguard.cfg proguard-project.txt project.properties local.properties
