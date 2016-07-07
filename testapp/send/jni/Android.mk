LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:=sendclientapp
LOCAL_SRC_FILES:=\
	utils.c \
	network.c\
	tederror.c\
	sendclientapp.c

LOCAL_CFLAGS:=-Wall

include $(BUILD_EXECUTABLE)
