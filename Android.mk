
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -O3 -frtti -w -g -fexceptions -fpermissive

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/android_9/openssl/include \
	$(LOCAL_PATH)/rapidjson-master/include \
	$(LOCAL_PATH)

LOCAL_SRC_FILES :=  \
	main.cpp \
	queryurl.cpp

LOCAL_MODULE := vk_messager

LOCAL_SHARED_LIBRARIES := \
	crypto_lib \
	ssl_lib

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := crypto_lib
LOCAL_SRC_FILES := $(LOCAL_PATH)/android_9/openssl/lib/libcrypto.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ssl_lib
LOCAL_SRC_FILES := $(LOCAL_PATH)/android_9/openssl/lib/libssl.so
include $(PREBUILT_SHARED_LIBRARY)