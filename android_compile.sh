#!/bin/sh

/home/andrew/android-ndk/ndk-build NDK_PROJECT_PATH=./ APP_BUILD_SCRIPT=./Android.mk SYSROOT=~/android-ndk/platforms/android-9/arch-arm NDK_ROOT=/home/andrew/android-ndk NDK_APPLICATION_MK=./Application.mk

adb push /home/andrew/vk_messager/libs/armeabi-v7a/vk_messager /data/local/