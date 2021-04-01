LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := native-activity
LOCAL_SRC_FILES := GL/glew.c ../../common/shader_utils.cpp \
	main.cpp \
	$(subst jni/,,$(wildcard jni/src/*.cpp jni/src/*.c))
LOCAL_CPPFLAGS  := -I/usr/src/glm
LOCAL_CXXFLAGS  := -gstabs+
LOCAL_LDLIBS    := -llog -landroid -lGLESv2 -lEGL
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
