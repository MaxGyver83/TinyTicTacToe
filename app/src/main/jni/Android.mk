LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(NDK_DEBUG),1)
	LOCAL_STRIP_MODE := none
endif

LOCAL_MODULE    := tinytictactoe
LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_LDLIBS    := -llog -landroid -lGLESv2 -lEGL -lOpenSLES -lm -u ANativeActivity_onCreate #-lGLESv1_CM

FILE_LIST := $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/lib/*.c)
IGNORE_LIST := $(wildcard $(LOCAL_PATH)/*_x11.c)
FILE_LIST := $(filter-out $(IGNORE_LIST), $(FILE_LIST))

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_CFLAGS    := -Oz -fvisibility=hidden -ffunction-sections -fdata-sections -fno-stack-protector -fno-omit-frame-pointer -flto -opaque-pointers=1
LOCAL_LDFLAGS   := -Wl,--gc-sections -fno-omit-frame-pointer

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)
