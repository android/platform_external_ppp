# Flags common to both pppd and plugins
COMMON_CFLAGS := -DCHAPMS=1 -DMPPE=1 -DINET6=1 -DUSE_OPENSSL=1		\
	-Wno-missing-field-initializers -Wno-unused-parameter -Werror	\
	-Wno-pointer-sign

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	auth.c \
	ccp.c \
	chap-md5.c \
	chap-new.c \
	chap_ms.c \
	demand.c \
	eap.c \
	ecp.c \
	eui64.c \
	fsm.c \
	ipcp.c \
	ipv6cp.c \
	lcp.c \
	magic.c \
	main.c \
	options.c \
	pppcrypt.c \
	pppox.c \
	session.c \
	sys-linux.c \
	tty.c \
	upap.c \
	utils.c

# options.c:623:21: error: passing 'const char *' to parameter of type 'char *' discards qualifiers.
# [-Werror,-Wincompatible-pointer-types-discards-qualifiers]
LOCAL_CLANG_CFLAGS += -Wno-incompatible-pointer-types-discards-qualifiers

LOCAL_SHARED_LIBRARIES := \
	libcutils liblog libcrypto

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := $(COMMON_CFLAGS)
LOCAL_CFLAGS += -Wno-empty-body -Wno-attributes -Wno-sign-compare

# Enable plugin support
LOCAL_CFLAGS += -DPLUGIN
LOCAL_LDFLAGS := -ldl -rdynamic

LOCAL_MODULE:= pppd
LOCAL_REQUIRED_MODULES := pppol2tp-android

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := pppol2tp-android
LOCAL_SRC_FILES := plugins/pppol2tp-android/pppol2tp-android.c
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/plugins/pppol2tp-android
LOCAL_CFLAGS := $(COMMON_CFLAGS)
LOCAL_CFLAGS += -Wno-gnu-designator -Wno-format
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
include $(BUILD_SHARED_LIBRARY)
