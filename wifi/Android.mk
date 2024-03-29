# Copyright 2006 The Android Open Source Project
#LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH1=\"$(WIFI_DRIVER_MODULE_PATH1)\"
#LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH2=\"$(WIFI_DRIVER_MODULE_PATH2)\"
#LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH3=\"$(WIFI_DRIVER_MODULE_PATH3)\"
#LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_ARG=\"$(WIFI_DRIVER_MODULE_ARG)\"
#LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_NAME1=\"$(WIFI_DRIVER_MODULE_NAME1)\"
#LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_NAME2=\"$(WIFI_DRIVER_MODULE_NAME2)\"
#LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_NAME3=\"$(WIFI_DRIVER_MODULE_NAME3)\"

ifdef WIFI_FIRMWARE_LOADER
LOCAL_CFLAGS += -DWIFI_FIRMWARE_LOADER=\"$(WIFI_FIRMWARE_LOADER)\"
endif
ifdef WIFI_DRIVER_FW_PATH_STA
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_STA=\"$(WIFI_DRIVER_FW_PATH_STA)\"
endif
ifdef WIFI_DRIVER_FW_PATH_AP
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_AP=\"$(WIFI_DRIVER_FW_PATH_AP)\"
endif
ifdef WIFI_DRIVER_FW_PATH_P2P
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_P2P=\"$(WIFI_DRIVER_FW_PATH_P2P)\"
endif
ifdef WIFI_DRIVER_FW_PATH_PARAM
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_PARAM=\"$(WIFI_DRIVER_FW_PATH_PARAM)\"
endif

ifeq ($(BOARD_WIFI_VENDOR), realtek)
LOCAL_CFLAGS += -DWIFI_VENDOR_REALTEK
endif

LOCAL_SRC_FILES += wifi/wifi.c

LOCAL_SHARED_LIBRARIES += libnetutils
