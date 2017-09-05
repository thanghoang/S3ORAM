LOCAL_PATH := $(call my-dir)
SRC_PATH = /scratch/simulation_thanghoang/SSORAM/S3ORAM
NDK_PATH = /scratch/simulation_thanghoang/android-ndk-r15c
# NDK_DEBUG_IMPORTS := 1

#########################################################
# GNUport library

include $(CLEAR_VARS)

GNUPORT_INCL     := $(NDK_PATH)/sources/cxx-stl/gnu-libstdc++/4.9/include
GNUPORT_INCL     += $(NDK_PATH)/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH_ABI)/include
GNUPORT_LIB      := $(NDK_PATH)/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH_ABI)
LOCAL_MODULE := gnustd_shared
LOCAL_SRC_FILES := $(GNUPORT_LIB)/libgnustl_shared.so
LOCAL_CPP_FEATURES += rtti exceptions

LOCAL_EXPORT_CPPFLAGS :=
LOCAL_EXPORT_C_INCLUDES := $(GNUPORT_INCL)

include $(PREBUILT_SHARED_LIBRARY)

LOCAL_SHARED_LIBRARIES  := gnustd_shared


#########################################################
#zmq
include $(CLEAR_VARS)

LOCAL_MODULE          := zmq
LOCAL_MODULE_FILENAME := zmq_static
LOCAL_SRC_FILES := libzmq.a

include $(PREBUILT_STATIC_LIBRARY)
#############################################
# S3ORAM
include $(CLEAR_VARS)

APP_STL         := gnustl_shared c++_static c++_shared
APP_MODULES     := s3oram_client gnustd_shared

LOCAL_C_INCLUDES   := $(GNUPORT_INCL) $(ZMQ_INCL) $(LOCAL_PATH) $(ZMQPP_INCL) $(ZMQ3_INCL)

LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_CPP_FLAGS    := -Wl,--exclude-libs,ALL -DLTC_OMAC

LOCAL_LDLIBS            := -llog -landroid -lgnustl_shared 
#LOCAL_SHARED_LIBRARIES  := stlport_shared
#LOCAL_SHARED_LIBRARIES  := gnustl_shared
LOCAL_STATIC_LIBRARIES  := zmq
LOCAL_LDLIBS      += -L$(NDK_PATH)/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH_ABI) 
LOCAL_WHOLE_STATIC_LIBRARIES :=  zmq

LOCAL_MODULE    := s3oram_client
LOCAL_SRC_FILES := $(SRC_PATH)/main_android.cpp \
$(SRC_PATH)/Utils.cpp \
$(SRC_PATH)/S3ORAM.cpp \
$(SRC_PATH)/ClientS3ORAM.cpp \
LOCAL_CPPFLAGS  += -std=c++11 

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
include $(BUILD_EXECUTABLE)
