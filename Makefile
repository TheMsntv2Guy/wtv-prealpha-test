#
# Makefile for WebTV box rom build(s)
#

####
#     How to add a new file
#----------------------------
#		1. go the directory where the new file is located, add the file
#			to the list of sources for each ROM (if applicable).
#
#     How to add a directory
#----------------------------
#		1. Edit the directory list in Makefile.include
#		2. Create a makefile (copy existing one)
#		3. Add the list of sources files in the new local Makefile.
#
#####


TOPDIR = .

.DEFAULT:		default
default			: unirom

include Makefile.include


APP_BASE		= ${CONFIG}/result/approm
BOOT_BASE		= ${CONFIG}/result/bootrom
GOOBER_BASE		= ${CONFIG}/result/gooberrom

APP_DUMP		= $(APP_BASE:%=%.dump)
BOOT_DUMP		= $(BOOT_BASE:%=%.dump)
GOOBER_DUMP		= $(GOOBER_BASE:%=%.dump)

APP_SYM			= $(APP_BASE:%=%.sym)
BOOT_SYM		= $(BOOT_BASE:%=%.sym)
GOOBER_SYM		= $(GOOBER_BASE:%=%.sym)

APP_LINKOUT		= $(APP_BASE).ld
BOOT_LINKOUT	= $(BOOT_BASE).ld
GOOBER_LINKOUT	= $(GOOBER_BASE).ld

# these need to be .o so cap doesn't translate them when dragging them accross the net.
UNI_IMAGE		= ${CONFIG}/result/unirom.o
APP_IMAGE		= $(APP_BASE).o
BOOT_IMAGE		= $(BOOT_BASE).o
GOOBER_IMAGE	= $(GOOBER_BASE).o

APP_MAIN		= ${CONFIG}/obj/app_crt0.s.o
BOOT_MAIN		= ${CONFIG}/obj/crt0.s.o
GOOBER_MAIN		= ${CONFIG}/obj/crt0.s.o


.PHONY			: allapp allboot allgoober default
allapp			: ${SUBDIRS} $(APP_IMAGE)
allboot			: ${SUBDIRS} $(BOOT_IMAGE)
allgoober		: ${SUBDIRS} $(GOOBER_IMAGE)
all				: unirom



ifndef VERBOSE
  MAKE_OPTIONS = --silent
else
  MAKE_OPTIONS = 
endif

#---- actual targets used (on command line) ----------------------------------------
#

unirom						: approm bootrom blankline unimessage blankline2
	@gmake ${MAKE_OPTIONS} UNI_ROM=1  startuni

unirom-nodebug				: approm-nodebug bootrom-nodebug blankline unimessage blankline2
	@gmake ${MAKE_OPTIONS} UNI_ROM=1 NON_DEBUG=1 startuni

unirom-nodebug-external		: approm-nodebug-external bootrom-nodebug-external blankline unimessage blankline2
	@gmake ${MAKE_OPTIONS} UNI_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1  startuni

unirom-nodebug-external-production: approm-nodebug-external-production bootrom-nodebug-external-production blankline unimessage blankline2
	@gmake ${MAKE_OPTIONS} UNI_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1 PRODUCTION_BUILD=1  startuni

unirom-post						: approm-post bootrom-post blankline unimessage-post blankline2
	@gmake ${MAKE_OPTIONS} UNI_ROM=1  POST=1 startuni

approm						: blankline appmessage blankline2 builddate
	@gmake ${MAKE_OPTIONS} APP_ROM=1  startapp

approm-nodebug				: blankline appmessage-nodebug blankline2 builddate
	@gmake ${MAKE_OPTIONS} APP_ROM=1 NON_DEBUG=1  startapp

approm-nodebug-external		: blankline appmessage-nodebug-external blankline2 builddate
	@gmake ${MAKE_OPTIONS} APP_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1  startapp

approm-nodebug-external-production		: blankline appmessage-nodebug-external-production blankline2 builddate
	@gmake ${MAKE_OPTIONS} APP_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1 PRODUCTION_BUILD=1 startapp

approm-post					: blankline appmessage-post blankline2 builddate
	@gmake ${MAKE_OPTIONS} APP_ROM=1 POST=1 startapp

bootrom						: blankline bootmessage blankline2 builddate
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 startboot

bootrom-nodebug				: blankline bootmessage-nodebug blankline2 builddate
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 NON_DEBUG=1 startboot

bootrom-nodebug-external	: blankline bootmessage-nodebug-external blankline2 builddate
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1 startboot

bootrom-nodebug-external-production	: blankline bootmessage-nodebug-external-production blankline2 builddate
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1 PRODUCTION_BUILD=1 startboot

bootrom-post				: blankline bootmessage-post blankline2 builddate
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 POST=1 startboot

gooberrom					: blankline goobermessage blankline2 builddate
	@gmake ${MAKE_OPTIONS} GOOBER_ROM=1 startgoober


nondebug					: unirom-nodebug

external					: unirom-nodebug-external

production					: unirom-nodebug-external-production


#---- actual target that trigger build -------------------------------------------
#
.PHONY			: startapp startgoober startboot

startuni		:  $(TARGET_DIRS) $(UNI_IMAGE)

startapp		:  $(TARGET_DIRS) tools allapp

startboot		:  $(TARGET_DIRS) tools allboot

startgoober		:  $(TARGET_DIRS) tools allgoober


#---- build tools first -------------------------------------------
#
.PHONY			: tools
tools			:
	@(cd ./Tools/Common; gmake ${MAKE_OPTIONS} all;)

PATH := ${CWD}/objects/tools:${CWD}/Tools/Common:${PATH}
export PATH


#---- user friendly messages ------------------------------------------------------
#
.PHONY			: blankline appmessage appmessage-nodebug appmessage-nodebug-external bootmessage bootmessage-nodebug bootmessage-nodebug-external goobermessage

blankline:
	@echo   "#"

blankline2:
	@echo   "#"

unimessage:
	@echo   "# Building unirom ROM...."

unimessage-post:
	@echo   "# Building unirom POST ROM...."

appmessage:
	@echo   "# Building debug App ROM...."

appmessage-nodebug:
	@echo   "# Building non-debug App ROM...."

appmessage-nodebug-external:
	@echo   "# Building non-debug, external App ROM...."

appmessage-nodebug-external-production:
	@echo   "# Building non-debug, external, Production App ROM...."

appmessage-post:
	@echo   "# Building POST debug App ROM...."

bootmessage:
	@echo   "# Building debug Boot ROM...."

bootmessage-nodebug:
	@echo   "# Building non-debug Boot ROM...."

bootmessage-nodebug-external:
	@echo   "# Building non-debug, external Boot ROM...."

bootmessage-post:
	@echo   "# Building POST debug Boot ROM...."

bootmessage-nodebug-external-production:
	@echo   "# Building non-debug, external, Production Boot ROM...."

goobermessage:
	@echo   "# Building debug Goober ROM...."


#---- builddate stuff -----------------------------------------------------------
#
builddate:
ifndef OFFICIAL_BUILD
	@echo   "#define kSoftwarePhase \"$(PHASE_STRING)\" " > Interfaces/SystemVersion.h 
	@echo   "#define kSoftwareBuild \"$(BUILD_STRING)\" " >> Interfaces/SystemVersion.h
	@echo   "#define kSoftwareBuildNumber 0" >> Interfaces/SystemVersion.h
endif

BUILD_NUMBER = 0  # can be overridden on the command line

#-------- Clean up rules -----------------------------------------------------
#
cleanall : cleanapp cleanboot cleangoober

cleanall-nodebug : cleanapp-nodebug cleanboot-nodebug

cleanall-nodebug-external : cleanapp-nodebug-external cleanboot-nodebug-external

cleanall-nodebug-external-production : cleanapp-nodebug-external-production cleanboot-nodebug-external-production

cleanapp:
	@gmake ${MAKE_OPTIONS} APP_ROM=1 realcleanapp

cleanapp-nodebug:
	@gmake ${MAKE_OPTIONS} APP_ROM=1 NON_DEBUG=1 realcleanapp

cleanapp-nodebug-external:
	@gmake ${MAKE_OPTIONS} APP_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1 realcleanapp

cleanapp-nodebug-external-production:
	@gmake ${MAKE_OPTIONS} APP_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1 PRODUCTION_BUILD=1 realcleanapp

clean-message:
	echo ""
	echo "# Cleaning out ${CONFIG}..."
	echo ""

realcleanapp: clean-message
	$(RM) "${CONFIG}"

cleanboot:
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 realcleanboot

cleanboot-nodebug:
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 NON_DEBUG=1 realcleanboot

cleanboot-nodebug-external:
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1 realcleanboot

cleanboot-nodebug-external-production:
	@gmake ${MAKE_OPTIONS} BOOT_ROM=1 NON_DEBUG=1 EXTERNAL_BUILD=1 PRODUCTION_BUILD=1 realcleanboot

realcleanboot: clean-message
	$(RM) "${CONFIG}"

cleangoober:
	@gmake ${MAKE_OPTIONS} GOOBER_ROM=1 realcleangoober


realcleangoober: clean-message
	$(RM) "${CONFIG}"

clean : clean-error clean-help

clean-error :
	@echo ""
	@echo "    Nothing cleaned.... you need to be more specific:"
	@echo ""
	

BUILDBOXROM_OPTIONS = -build=$(BUILD_NUMBER) 
ifdef NON_DEBUG
  BUILDBOXROM_OPTIONS += 
else
  BUILDBOXROM_OPTIONS += -debug
endif

#---- Link commands -----------------------------------------------------------
#
.PHONY: app_link boot_link goober_link $(UNI_IMAGE)

$(UNI_IMAGE)	:
	echo "     Creating UNIROM ROM image: "${notdir ${UNI_IMAGE}}
	${CWD}/Tools/Common/BuildBoxRom.pl ${BUILDBOXROM_OPTIONS} -unirom $(UNI_IMAGE)
	echo "    -> Result in: $(CONFIG)/result"
	echo ""
	echo ""

$(APP_LINKOUT)	: ${LINK_LIBRARIES} EntryPoint
	echo "     Linking "${notdir ${APP_LINKOUT}}
	$(CC) -nostdlib $(APP_LD_FLAGS) $(APP_MAIN) \
		${LINK_LIBRARIES} -B/tools/gnu-mips/mips-ecoff/bin/ \
		-L/tools/gnu-mips/lib/gcc-lib/mips-ecoff/2.7.2  -o $(APP_LINKOUT)
	echo "     Creating "${notdir ${APP_SYM}}
	$(OBJDUMP) --syms $(APP_LINKOUT) > $(APP_SYM)
	echo "XXXXXX" >> $(APP_SYM)
	echo "     Stripping "${notdir ${APP_LINKOUT}}
	$(STRIP) $(APP_LINKOUT)


$(APP_IMAGE)	: ${APP_LINKOUT}
	echo "     Creating APP ROM image: "${notdir ${APP_IMAGE}}
	${CWD}/Tools/Common/BuildBoxRom.pl ${BUILDBOXROM_OPTIONS} -approm $(APP_LINKOUT)
	echo "    -> Result in: $(CONFIG)/result"
	echo ""
	echo ""


$(BOOT_LINKOUT)	: ${LINK_LIBRARIES} EntryPoint
	echo "     Linking "${notdir ${BOOT_LINKOUT}}
	$(CC) -nostdlib $(BOOT_LD_FLAGS) $(BOOT_MAIN) \
		${LINK_LIBRARIES} -B/tools/gnu-mips/mips-ecoff/bin/ \
		-L/tools/gnu-mips/lib/gcc-lib/mips-ecoff/2.7.2  -o $(BOOT_LINKOUT)
	echo "     Creating "${notdir ${BOOT_LINKOUT}}
	$(STRIP) $(BOOT_LINKOUT)


$(BOOT_IMAGE)	: ${BOOT_LINKOUT}
	${CWD}/Tools/Common/BuildBoxRom.pl ${BUILDBOXROM_OPTIONS} -bootrom $(BOOT_LINKOUT)
	echo "    -> Result in: $(CONFIG)/result"
	echo ""
	echo ""


$(GOOBER_LINKOUT) : ${LINK_LIBRARIES} EntryPoint
	echo "     Linking "${notdir ${GOOBER_LINKOUT}}
	$(CC) -nostdlib $(GOOBER_LD_FLAGS) $(GOOBER_MAIN) \
		${LINK_LIBRARIES} -B/tools/gnu-mips/mips-ecoff/bin/ \
		-L/tools/gnu-mips/lib/gcc-lib/mips-ecoff/2.7.2  -o $(GOOBER_LINKOUT)
	echo "     Creating "${notdir ${GOOBER_LINKOUT}}
	$(STRIP) $(GOOBER_LINKOUT)
	echo "    -> Result in: $(CONFIG)/result"
	echo ""
	echo ""

# just to be consistent
$(GOOBER_IMAGE)	: ${GOOBER_LINKOUT}
	cp -p  ${GOOBER_LINKOUT} ${GOOBER_IMAGE}



#---- help messages -----------------------------------------------------------
#
help :
	@echo ""
	@echo "    Legitimate targets are:"
	@echo "        approm        -  builds a debug version of the APP ROM (default target)"
	@echo "        bootrom       -  builds a debug version of the BOOT ROM"
	@echo "        gooberrom     -  builds a debug version of the GOOBER ROM"
	@echo ""
	@echo "        approm-nodebug     -  builds a non-debug version of the APP ROM"
	@echo "        bootrom-nodebug    -  builds a non-debug version of the BOOT ROM"
	@echo ""
	@echo "        approm-nodebug-external   -  builds a non-debug, external version of the APP ROM"
	@echo "        bootrom-nodebug-external  -  builds a non-debug, external version of the BOOT ROM"
	@echo ""
	@echo "        See help-clean for information on clean targets"
	@echo ""
	@echo ""
	
help-clean :
	@echo ""
	@echo "    Legitimate clean targets are:"
	@echo "        cleanall    -  cleanup all debug roms "
	@echo "        cleanapp    -  cleanup the APP ROM only"
	@echo "        cleanboot   -  cleanup the Boot ROM only"
	@echo "        cleangoober -  cleanup the Goober ROM only"
	@echo ""
	@echo "        cleanall-nodebug    -  cleanup all non-debug roms"
	@echo "        cleanapp-nodebug    -  cleanup the non-debug APP ROM only"
	@echo "        cleanboot-nodebug   -  cleanup the non-debug Boot ROM only"
	@echo ""
	@echo "        cleanall-nodebug-external    -  cleanup all non-debug, external roms"
	@echo "        cleanapp-nodebug-external    -  cleanup the non-debug, external APP ROM only"
	@echo "        cleanboot-nodebug-external   -  cleanup the non-debug, external Boot ROM only"
	@echo ""
	@echo ""
