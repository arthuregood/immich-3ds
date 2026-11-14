#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM nao definido. Rode ./build.sh ou exporte DEVKITARM=<path>/devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      :=  immich-3ds
BUILD       :=  build
SOURCES     :=  source
DATA        :=
INCLUDES    :=  source
ICON        :=  meta/icon.png

APP_TITLE       := Immich 3DS
APP_DESCRIPTION := Send 3DS photos to Immich
APP_AUTHOR      := arthuregood

#---------------------------------------------------------------------------------
ARCH    :=  -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS  :=  -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__ -DCURL_STATICLIB
ASFLAGS :=  -g $(ARCH)
LDFLAGS :=  -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

# ordem importa: curl depende de mbedtls/zlib, tudo depende de libctru
LIBS    :=  -lcurl -lmbedtls -lmbedx509 -lmbedcrypto -ljpeg -lz -lctru -lm

LIBDIRS :=  $(CTRULIB) $(PORTLIBS)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT   :=  $(CURDIR)/$(TARGET)
export VPATH    :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR  :=  $(CURDIR)/$(BUILD)

CFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

ifeq ($(strip $(CPPFILES)),)
    export LD := $(CC)
else
    export LD := $(CXX)
endif

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE  :=  $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                    $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                    -I$(CURDIR)/$(BUILD)
export LIBPATHS :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON := $(TOPDIR)/$(ICON)
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh

.PHONY: all clean

all: $(BUILD)
	@sh tools/gen_build_config.sh "$(CURDIR)/.env" "$(CURDIR)/source/config_build.h"
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh

#---------------------------------------------------------------------------------
else

$(OUTPUT).3dsx   :  $(OUTPUT).elf $(OUTPUT).smdh
$(OFILES)        :
$(OUTPUT).elf    :  $(OFILES)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
