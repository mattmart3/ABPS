LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:=tedproxy
LOCAL_SRC_FILES:=\
	../src/utils.c \
	../src/network.c\
	../src/tederror.c\
	../src/main.c

LOCAL_CFLAGS:=-Wall

include $(BUILD_EXECUTABLE)
