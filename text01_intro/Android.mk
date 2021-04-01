# Cf. http://en.wikibooks.org/wiki/OpenGL_Programming/Installation/Android#FreeType
# to cross-compile FreeType.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := native-activity
LOCAL_SRC_FILES := main.cpp GL/glew.c tut.cpp ../common/shader_utils.cpp
LOCAL_CPPFLAGS  := -I/usr/src/glm
LOCAL_CXXFLAGS  := -gstabs+
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2
LOCAL_STATIC_LIBRARIES := android_native_app_glue freetype

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
$(call import-module,freetype)
