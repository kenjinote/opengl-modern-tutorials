/**
 * GLEW compatibility stubs for Android
 * Copyright (C) 2012  Sylvain Beucler
 * Released under the same license as GLEW
 */

#include "glew.h"

#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "wikibooks-android_wrapper", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "wikibooks-android_wrapper", __VA_ARGS__))

GLenum glewInit () {
  LOGI("glewInit");
  return GLEW_OK;
}

const GLubyte* glewGetErrorString (GLenum error) {
  LOGI("glewGetErrorString");
  return "[GLEW STUB]";
}
