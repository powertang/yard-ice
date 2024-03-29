#
# prog.mk 
#
# Copyright(C) 2012 Robinson Mittmann. All Rights Reserved.
# 
# This file is part of the YARD-ICE.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3.0 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You can receive a copy of the GNU Lesser General Public License from 
# http://www.gnu.org/

ifndef CONFIG_MK
 $(error Please include "config.mk" in your Makefile)
endif

#------------------------------------------------------------------------------ 
# cross compiling 
#------------------------------------------------------------------------------ 

ifndef CROSS_COMPILE
  # default to the host compiler by tricking the make to assign a 
  # empty string to the CROSS_COMPILE variable
  empty =
  CROSS_COMPILE = $(empty)
endif	

ifndef CFLAGS
  CFLAGS := -g -O1
endif

include $(SCRPTDIR)/cross.mk

#------------------------------------------------------------------------------ 
# generated files
#------------------------------------------------------------------------------ 
ifdef VERSION_MAJOR
  VERSION_H = $(OUTDIR)/version.h
else
  VERSION_H =
endif

#------------------------------------------------------------------------------ 
# generated source files
#------------------------------------------------------------------------------ 
HFILES_OUT = $(VERSION_H) $(addprefix $(OUTDIR)/, $(HFILES_GEN))
CFILES_OUT = $(addprefix $(OUTDIR)/, $(CFILES_GEN))
SFILES_OUT = $(addprefix $(OUTDIR)/, $(SFILES_GEN))

#------------------------------------------------------------------------------ 
# object files
#------------------------------------------------------------------------------ 
OFILES = $(addprefix $(OUTDIR)/, $(notdir $(CFILES_OUT:.c=.o) \
         $(SFILES_OUT:.S=.o)) $(CFILES:.c=.o) $(SFILES:.S=.o))
#ODIRS = $(abspath $(sort $(dir $(OFILES))))
ODIRS = $(sort $(dir $(OFILES)))

#------------------------------------------------------------------------------ 
# dependency files
#------------------------------------------------------------------------------ 
#DFILES = $(abspath $(addprefix $(DEPDIR)/, $(notdir $(CFILES_OUT:.c=.d) \
#         $(SFILES_OUT:.S=.d)) $(CFILES:.c=.d) $(SFILES:.S=.d)))
DFILES = $(addprefix $(DEPDIR)/, $(notdir $(CFILES_OUT:.c=.d) \
			$(SFILES_OUT:.S=.d)) $(CFILES:.c=.d) $(SFILES:.S=.d))
#DDIRS = $(abspath $(sort $(dir $(DFILES))))
DDIRS = $(sort $(dir $(DFILES)))

#------------------------------------------------------------------------------ 
# library dircetories 
#------------------------------------------------------------------------------ 
LIBDIRS := $(abspath $(LIBDIRS))

#------------------------------------------------------------------------------ 
# path variables
#------------------------------------------------------------------------------ 
LIBPATH := $(addprefix $(OUTDIR)/, $(notdir $(LIBDIRS))) $(LDDIR) $(abspath $(LIBPATH))
INCPATH	:= $(abspath $(INCPATH)) $(abspath .) $(abspath $(OUTDIR))

#$(info --------------------------)
#$(info OS = '$(OS)')
#$(info OSTYPE = '$(OSTYPE)')
#$(info HOST = '$(HOST)')
#$(info OFILES = '$(OFILES)')
#$(info CC = '$(CC)')
#$(info SRCDIR = '$(SRCDIR)')
#$(info DIRMODE = '$(DIRMODE)')
#$(info INCPATH = '$(INCPATH)')
#$(info LIBDIRS = '$(LIBDIRS)')
#$(info DDIRS = '$(DDIRS)')
#$(info INCPATH = '$(INCPATH)')
#$(info LIBPATH = '$(LIBPATH)')
#$(info abspath = '$(abspath .)')
#$(info realpath = '$(realpath .)')
#$(info CFLAGS = '$(CFLAGS)')
#$(info --------------------------)

#------------------------------------------------------------------------------ 
# program output files
#------------------------------------------------------------------------------ 
ifdef PROG
  ifeq ($(strip $(CROSS_COMPILE)),)
    ifneq ($(HOST),Linux)
    	PROG_BIN := $(OUTDIR)/$(PROG).exe
    else
    	PROG_BIN := $(OUTDIR)/$(PROG)
    endif
  else
    PROG_BIN := $(OUTDIR)/$(PROG).bin
  endif
  PROG_MAP := $(OUTDIR)/$(PROG).map
  PROG_ELF := $(OUTDIR)/$(PROG).elf
  PROG_SYM := $(OUTDIR)/$(PROG).sym
  PROG_LST := $(OUTDIR)/$(PROG).lst
  PROG_TAG := $(OUTDIR)/$(PROG).tag
endif

ifeq ($(HOST),Cygwin)
  INCPATH_WIN := $(subst \,\\,$(foreach h,$(INCPATH),$(shell cygpath -w $h)))
  OFILES_WIN := $(subst \,\\,$(foreach o,$(OFILES),$(shell cygpath -w $o)))
  LIBPATH_WIN := $(subst \,\\,$(foreach l,$(LIBPATH),$(shell cygpath -w $l)))
  PROG_BIN_WIN := $(subst \,\\,$(shell cygpath -w $(PROG_BIN)))
  PROG_ELF_WIN := $(subst \,\\,$(shell cygpath -w $(PROG_ELF)))
  PROG_LST_WIN := $(subst \,\\,$(shell cygpath -w $(PROG_LST)))
  PROG_SYM_WIN := $(subst \,\\,$(shell cygpath -w $(PROG_SYM)))
endif


#export LDFLAGS INCPATH LIBPATH

FLAGS_TO_PASS := $(FLAGS_TO_PASS) 'D=$(dbg_level)' 'V=$(verbose)' \
				 'MACH=$(MACH)'\
				 'CPU=$(CPU)'\
				 'CC=$(CC)'\
				 'LD=$(LD)'\
				 'AS=$(AS)'\
				 'AR=$(AR)'\
				 'OBJCOPY=$(OBJCOPY)'\
				 'OBJDUMP=$(OBJDUMP)'\
				 'STRIP=$(STRIP)'\
				 'CFLAGS=$(CFLAGS)'\
				 'SFLAGS=$(SFLAGS)'\
				 'LDFLAGS=$(LDFLAGS)'\
				 'INCPATH=$(INCPATH)'\
				 'LIBPATH=$(LIBPATH)'

LIBDIRS_ALL := $(LIBDIRS:%=%-all)

LIBDIRS_CLEAN := $(LIBDIRS:%=%-clean)

CLEAN_FILES := $(HFILES_OUT) $(CFILES_OUT) $(SFILES_OUT) $(OFILES) $(DFILES) $(PROG_BIN) $(PROG_ELF) $(PROG_LST) $(PROG_SYM) $(PROG_MAP)

ifeq (Windows,$(HOST))
  CLEAN_FILES := $(subst /,\,$(CLEAN_FILES))
endif

all: $(PROG_BIN) $(PROG_SYM) $(PROG_LST)

clean: libs-clean
	$(Q)$(RMALL) $(CLEAN_FILES)

prog: $(PROG_BIN)

elf: $(PROG_ELF)

map: $(PROG_MAP)

bin: $(PROG_BIN)

sym: $(PROG_SYM)

lst: $(PROG_LST)

libs-all: $(LIBDIRS_ALL)

libs-clean: $(LIBDIRS_CLEAN)

#------------------------------------------------------------------------------ 
# Helpers to print the binary full path
#------------------------------------------------------------------------------ 

bin_path:
	@$(ECHO) $(PROG_BIN)

elf_path:
	@$(ECHO) $(PROG_ELF)

#------------------------------------------------------------------------------ 
# Code::Blocks targets
#------------------------------------------------------------------------------ 

Debug: 
	$(Q)$(MAKE) D=1 all

Release: 
	$(Q)$(MAKE) D=0 all

cleanDebug: 
	$(Q)$(MAKE) D=1 clean

cleanRelease: 
	$(Q)$(MAKE) D=0 clean

.PHONY: all clean prog elf map bin lst libs-all libs-clean bin_path elf_path 
.PHONY: Debug Release cleanDebug cleanRelease
.PHONY: $(LIBDIRS_ALL) $(LIBDIRS_CLEAN)

#------------------------------------------------------------------------------ 
# Library dependencies targets
#------------------------------------------------------------------------------ 

$(LIBDIRS_ALL):
	$(ACTION) "Building : $@"
	$(Q)$(MAKE) -C $(@:%-all=%) O=$(OUTDIR)/$(notdir $(@:%-all=%)) $(FLAGS_TO_PASS) all

$(LIBDIRS_CLEAN):
	$(ACTION) "Cleaning : $@"
	$(Q)$(MAKE) -C $(@:%-clean=%) O=$(OUTDIR)/$(notdir $(@:%-clean=%)) $(FLAGS_TO_PASS) clean

#------------------------------------------------------------------------------ 
# Program targets
#------------------------------------------------------------------------------ 

$(PROG_ELF) $(PROG_MAP): $(LIBDIRS_ALL) $(OFILES) $(OBJ_EXTRA)
	$(ACTION) "LD: $(PROG_ELF)"
ifeq ($(HOST),Cygwin)
	$(Q)$(LD) $(LDFLAGS) $(OFILES_WIN) $(OBJ_EXTRA) -Wl,--print-map \
	-Wl,--cref -Wl,--sort-common \
	-Wl,--start-group $(addprefix -l,$(LIBS)) -Wl,--end-group \
	$(addprefix -L,$(LIBPATH_WIN)) -o $(PROG_ELF_WIN) > $(PROG_MAP)
else
	$(Q)$(LD) $(LDFLAGS) $(OFILES) $(OBJ_EXTRA) -Wl,--print-map \
	-Wl,--cref -Wl,--sort-common \
	-Wl,--start-group $(addprefix -l,$(LIBS)) -Wl,--end-group \
	$(addprefix -L,$(LIBPATH)) -o $(PROG_ELF) > $(PROG_MAP)
endif

%.sym: %.elf
	$(ACTION) "SYM: $@"
ifeq ($(HOST),Cygwin)
	$(Q)$(OBJDUMP) -t $(PROG_ELF_WIN) | sort > $@
else
	$(Q)$(OBJDUMP) -t $< | sort > $@
endif

%.lst: %.elf
	$(ACTION) "LST: $@"
ifeq ($(HOST),Cygwin)
	$(Q)$(OBJDUMP) -w -d -t -S -r -z $(PROG_ELF_WIN) > $@
else
	$(Q)$(OBJDUMP) -w -d -t -S -r -z $< > $@
endif

ifeq ($(strip $(CROSS_COMPILE)),)
$(PROG_BIN): $(PROG_ELF)
	$(ACTION) "Strip: $(PROG_ELF)"
  ifeq ($(HOST),Cygwin)
	$(Q)$(STRIP) -o $(PROG_BIN_WIN) $(PROG_ELF_WIN)
  else
	$(Q)$(STRIP) -o $@ $<
  endif
else
%.bin: %.elf
	$(ACTION) "BIN: $@"
	$(Q)$(OBJCOPY) -j .init -j .text -j .ARM.extab -j .ARM.exidx -j .data \
					  --output-target binary $< $@
endif

#------------------------------------------------------------------------------ 
# Build tree
#------------------------------------------------------------------------------ 

$(ODIRS):
	$(ACTION) "Creating outdir: $@"
ifeq ($(HOST),Windows)
	$(Q)$(MKDIR) $(subst /,\,$@)
else
	$(Q)$(MKDIR) $@
endif

$(DDIRS):
	$(ACTION) "Creating depdir: $@"
ifeq ($(HOST),Windows)
	$(Q)$(MKDIR) $(subst /,\,$@)
else
	$(Q)$(MKDIR) $@ 
endif

$(LIBDIRS_ALL): | $(ODIRS)

$(HFILES_OUT) $(CFILES_OUT) $(SFILES_OUT): | $(ODIRS)

$(DDIRS): | $(ODIRS) $(CFILES_OUT) $(HFILES_OUT)

$(DFILES): | $(DDIRS) 

ifdef VERSION_MAJOR
  include $(SCRPTDIR)/version.mk
endif

include $(SCRPTDIR)/cc.mk

include $(SCRPTDIR)/jtag.mk

#
# FIXME: automatic dependencies are NOT included in Cygwin.
# The dependencie files must have the paths converted
# to cygwin (unix) style to be of any use!
#
ifneq ($(HOST),Cygwin)
-include $(DFILES)
endif



