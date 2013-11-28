LOCAL_PATH:= $(call my-dir)

ifndef WPA_SUPPLICANT_VERSION
WPA_SUPPLICANT_VERSION := VER_2_1_DEVEL_WCS
endif
ifeq ($(WPA_SUPPLICANT_VERSION),VER_2_1_DEVEL_WCS)
include $(call all-subdir-makefiles)
endif
