# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE           := native-activity
LOCAL_SRC_FILES        += main.cpp
LOCAL_SRC_FILES        += decoder.cpp
LOCAL_SRC_FILES        += network.cpp
LOCAL_SRC_FILES        += sensor.cpp
LOCAL_LDLIBS           := -llog -landroid -lEGL -lGLESv1_CM
LOCAL_STATIC_LIBRARIES := android_native_app_glue

LOCAL_C_INCLUDES       += D:/Dev/android/src/system/frameworks/av/include
LOCAL_C_INCLUDES       += D:/Dev/android/src/system/frameworks/av/media/libstagefright/include
LOCAL_C_INCLUDES       += D:/Dev/android/src/system/system/core/include
LOCAL_C_INCLUDES       += D:/Dev/android/src/system/frameworks/native/include
LOCAL_C_INCLUDES       += D:/Dev/android/src/system/hardware/libhardware/include
LOCAL_C_INCLUDES       += D:/Dev/android/src/system/frameworks/native/include/media/openmax

LOCAL_CFLAGS           += -Wno-multichar

LOCAL_LDLIBS           += -LD:/Dev/android/src/system/lib -LD:/Dev/android/src/system/lib/hw -lstagefright -lutils -lui -lcutils


include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
