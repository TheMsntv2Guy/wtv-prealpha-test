# WebTV box rom subproject Makefile

TOPDIR = ..
SUBDIR = Storage

COMMON_CSRC =	\
			MemoryManager.c  \
			ObjectStore.c \
#

APP_CSRC	= $(COMMON_CSRC) StoreStream.c FlashStorage.c 

BOOT_CSRC	= $(COMMON_CSRC)  FlashStorage.c

GOOBER_CSRC	= $(COMMON_CSRC) 

include $(TOPDIR)/Makefile.include
