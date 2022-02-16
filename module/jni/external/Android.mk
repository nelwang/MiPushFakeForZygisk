LOCAL_PATH := $(call my-dir)

# libxhook.a
include $(CLEAR_VARS)
LOCAL_MODULE:= libxhook
LOCAL_C_INCLUDES := $(LOCAL_PATH)/xhook/libxhook/jni
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_CFLAGS := -Wall -Wextra -Werror -fvisibility=hidden 
LOCAL_CONLYFLAGS := -std=c11
LOCAL_SRC_FILES := \
    xhook/libxhook/jni/xh_log.c \
    xhook/libxhook/jni/xh_version.c \
    xhook/libxhook/jni/xh_jni.c \
    xhook/libxhook/jni/xhook.c \
    xhook/libxhook/jni/xh_core.c \
    xhook/libxhook/jni/xh_util.c \
    xhook/libxhook/jni/xh_elf.c
include $(BUILD_STATIC_LIBRARY)
