#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
SOURCES		:=	source source/fs source/fs/udf source/fs/iso9660 source/fs/audiocdfs/ source/os/switch
DATA		:=	data
INCLUDES	:=  include source source/fs source/fs/udf source/fs/iso9660 source/fs/audiocdfs/ source/os/switch

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec

CFLAGS	:=	-g -Wall -Werror \
			-ffunction-sections \
			-fdata-sections \
			$(ARCH) \
			$(BUILD_CFLAGS)

CFLAGS	+=	$(INCLUDE)

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS := $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)
			
$(eval LIB_VERSION_MAJOR = $(shell grep 'define LIBUSBDVD_VERSION_MAJOR\b' include/usbdvd.h | tr -s [:blank:] | cut -d' ' -f3))
$(eval LIB_VERSION_MINOR = $(shell grep 'define LIBUSBDVD_VERSION_MINOR\b' include/usbdvd.h | tr -s [:blank:] | cut -d' ' -f3))
$(eval LIB_VERSION_MICRO = $(shell grep 'define LIBUSBDVD_VERSION_MICRO\b' include/usbdvd.h | tr -s [:blank:] | cut -d' ' -f3))
$(eval LIB_VERSION = $(LIB_VERSION_MAJOR).$(LIB_VERSION_MINOR).$(LIB_VERSION_MICRO))

.PHONY: clean all

#---------------------------------------------------------------------------------
all: libs examples

libs: lib/$(TARGET).a lib/$(TARGET)d.a

lib:
	@[ -d $@ ] || mkdir -p $@

release:
	@[ -d $@ ] || mkdir -p $@

debug:
	@[ -d $@ ] || mkdir -p $@

lib/$(TARGET).a : lib release $(SOURCES) $(INCLUDES)
	@$(MAKE) BUILD=release OUTPUT=$(CURDIR)/$@ \
	BUILD_CFLAGS="-DNDEBUG=1 -O2" \
	DEPSDIR=$(CURDIR)/release \
	--no-print-directory -C release \
	-f $(CURDIR)/Makefile

lib/$(TARGET)d.a : lib debug $(SOURCES) $(INCLUDES)
	@$(MAKE) BUILD=debug OUTPUT=$(CURDIR)/$@ \
	BUILD_CFLAGS="-DDEBUG=1 -Og" \
	DEPSDIR=$(CURDIR)/debug \
	--no-print-directory -C debug \
	-f $(CURDIR)/Makefile

examples: libs
	@$(MAKE) -C examples/example_c -f Makefile clean
	@$(MAKE) --no-print-directory -C examples/example_c -f Makefile
	cp $(CURDIR)/examples/example_c/libusbdvd-example-c.nro $(CURDIR)/
	

dist-bin: all
	@tar --exclude=*~ -cjf $(TARGET)-$(LIB_VERSION).tar.bz2 include lib

dist-src:
	@tar --exclude=*~ -cjf $(TARGET)-$(LIB_VERSION)-src.tar.bz2 include source Makefile

dist: dist-src dist-bin

install: dist-bin
	@bzip2 -cd $(TARGET)-$(LIB_VERSION).tar.bz2 | tar -xf - -C $(PORTLIBS) --exclude='*.md' --exclude='*.nro'

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr release debug lib *.bz2

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT)	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES)

#---------------------------------------------------------------------------------
%_bin.h %.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------

