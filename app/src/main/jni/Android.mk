LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := tinytictactoe
LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_LDLIBS    := -llog -landroid -lGLESv2 -lEGL -lOpenSLES -lm -u ANativeActivity_onCreate #-lGLESv1_CM

FILE_LIST := $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/lib/*.c)
FILE_LIST := $(filter-out $(LOCAL_PATH)/main_x11.c $(LOCAL_PATH)/audio_x11.c, $(FILE_LIST))

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_CFLAGS    := -Oz -fvisibility=hidden -ffunction-sections -fdata-sections -fno-stack-protector -fomit-frame-pointer -flto -opaque-pointers=1 -g
LOCAL_LDFLAGS   := -Wl,--gc-sections -s -g

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)
