# WebTV box rom subproject Makefile

TOPDIR = ..
SUBDIR = Box

CommonCSources =	\
					BuildDate.c \
					BoxCUtils.c \
					BoxDebugger.c \
					Checksum.c \
					CInterrupts.c  \
					Disassembler.c \
					Fence.c \
					Flash.c \
					HWKeyboard.c  \
					HWDisplay.c  \
					HWModem.c \
					HWExpansion.c \
					IIC.c \
					IR.c 		\
					NextInstrAddr.c \
					SiliconSerialC.c  \
					SmartCard.c  \
#				

CommonASources =	\
					Caches.s  \
					Exceptions.s  \
					AInterrupts.s  \
					SiliconSerial.s  \
					BoxAUtils.s	\
					CrashLog.s \
					Flash.s \
#


APP_CSRC	= AppBoot.c HWAudio.c $(CommonCSources)
APP_ASRC	= $(CommonASources)

BOOT_CSRC	= BoxBoot.c FlashDownload.c Lzss.c bip.c $(CommonCSources)
BOOT_ASRC	= $(CommonASources)

GOOBER_CSRC = BoxBoot.c $(CommonCSources) 
GOOBER_ASRC = $(CommonASources) 

BuildDate.o	: ../Interfaces/SystemVersion.h

include $(TOPDIR)/Makefile.include


.PHONY: LocalEntryPoint
ifdef APP_ROM
LocalEntryPoint : ${OBJ}/app_crt0.s.o 
else
LocalEntryPoint : ${OBJ}/crt0.s.o
endif

