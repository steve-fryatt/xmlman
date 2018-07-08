# Copyright 2018, Stephen Fryatt (info@stevefryatt.org.uk)
#
# This file is part of XmlMan:
#
#   http://www.stevefryatt.org.uk/software/
#
# Licensed under the EUPL, Version 1.1 only (the "Licence");
# You may not use this work except in compliance with the
# Licence.
#
# You may obtain a copy of the Licence at:
#
#   http://joinup.ec.europa.eu/software/page/eupl
#
# Unless required by applicable law or agreed to in
# writing, software distributed under the Licence is
# distributed on an "AS IS" basis, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the Licence for the specific language governing
# permissions and limitations under the Licence.

# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

.PHONY: all clean documentation release install

# The build date.

BUILD_DATE := $(shell date "+%d %b %Y")
HELP_DATE := $(shell date "+%-d %B %Y")

# Construct version or revision information.

ifeq ($(VERSION),)
  RELEASE := $(shell svnversion --no-newline)
  VERSION := r$(RELEASE)
  RELEASE := $(subst :,-,$(RELEASE))
  HELP_VERSION := ----
else
  RELEASE := $(subst .,,$(VERSION))
  HELP_VERSION := $(VERSION)
endif

$(info Building for $(TARGET) with version $(VERSION) ($(RELEASE)) on date $(BUILD_DATE))

# The archive to assemble the release files in.  If $(RELEASE) is set, then the file can be given
# a standard version number suffix.

ifeq ($(TARGET),riscos)
  ZIPFILE := xmlman$(RELEASE)ro.zip
else
  ZIPFILE := xmlman$(RELEASE)linux.zip
endif
SRCZIPFILE := xmlman$(RELEASE)src.zip
BUZIPFILE := xmlman$(shell date "+%Y%m%d").zip


# Build Tools

ifeq ($(TARGET),riscos)
  CC := $(wildcard $(GCCSDK_INSTALL_CROSSBIN)/*gcc)
else
  CC := gcc
endif

MKDIR := mkdir -p
RM := rm -rf
CP := cp

ZIP := $(GCCSDK_INSTALL_ENV)/bin/zip

MANTOOLS := $(SFTOOLS_BIN)/mantools
BINDHELP := $(SFTOOLS_BIN)/bindhelp
TEXTMERGE := $(SFTOOLS_BIN)/textmerge
MENUGEN := $(SFTOOLS_BIN)/menugen


# Build Flags

ifeq ($(TARGET),riscos)
  CCFLAGS := -mlibscl -mhard-float -static -mthrowback -Wall -O2 -D'RISCOS' -D'BUILD_VERSION="$(VERSION)"' -D'BUILD_DATE="$(BUILD_DATE)"' -fno-strict-aliasing -mpoke-function-name
  ZIPFLAGS := -x "*/.svn/*" -r -, -9
else
  CCFLAGS := -Wall -g -O2 -fno-strict-aliasing -rdynamic -D'LINUX' -D'BUILD_VERSION="$(VERSION)"' -D'BUILD_DATE="$(BUILD_DATE)"'
  ZIPFLAGS := -x "*/.svn/*" -r -9
endif
SRCZIPFLAGS := -x "*/.svn/*" -r -9
BUZIPFLAGS := -x "*/.svn/*" -r -9
BINDHELPFLAGS := -f -r -v
MENUGENFLAGS := -d


# Includes and libraries.

#INCLUDES := -I$(GCCSDK_INSTALL_ENV)/include
#LINKS := -L$(GCCSDK_INSTALL_ENV)/lib


# Set up the various build directories.

SRCDIR := src
MANUAL := manual
OBJROOT := obj
OBJLINUX := linux
OBJRO := ro
OUTDIRLINUX := buildlinux
OUTDIRRO:= buildro
ifeq ($(TARGET),riscos)
  OBJDIR := $(OBJROOT)/$(OBJRO)
  OUTDIR := $(OUTDIRRO)
else
  OBJDIR := $(OBJROOT)/$(OBJLINUX)
  OUTDIR := $(OUTDIRLINUX)
endif



# Set up the named target files.

ifeq ($(TARGET),riscos)
  RUNIMAGE := xmlman,ff8
  README := ReadMe,fff
  LICENSE := Licence,fff
else
  RUNIMAGE := xmlman
  README := ReadMe.txt
  LICENSE := Licence.txt
endif


# Set up the source files.

ifeq ($(TARGET),riscos)
  INCLUDES := -I$(GCCSDK_INSTALL_ENV)/include
  LINKS := -L$(GCCSDK_INSTALL_ENV)/lib -lOSLibH32
else
  INCLUDES := -I/usr/include/libxml2
  LINKS := -lxml2
endif

MANSRC := Source
MANSPR := ManSprite

OBJS := args.o		\
	parse.o		\
	string.o	\
	xmlman.o


# Build everything, but don't package it for release.

all: documentation $(OUTDIR)/$(RUNIMAGE)


# Build the complete !RunImage from the object files.

OBJS := $(addprefix $(OBJDIR)/, $(OBJS))

$(OUTDIR)/$(RUNIMAGE): $(OBJDIR) $(OBJS)
	$(CC) $(CCFLAGS) -o $(OUTDIR)/$(RUNIMAGE) $(OBJS) $(LINKS)

# Build the object files, and identify their dependencies.

-include $(OBJS:.o=.d)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $(CCFLAGS) $(INCLUDES) $< -o $@
	@$(CC) -MM $(CCFLAGS) $(INCLUDES) $< > $(@:.o=.d)
	@mv -f $(@:.o=.d) $(@:.o=.d).tmp
	@sed -e 's|.*:|$@:|' < $(@:.o=.d).tmp > $(@:.o=.d)
	@sed -e 's/.*://' -e 's/\\$$//' < $(@:.o=.d).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(@:.o=.d)
	@rm -f $(@:.o=.d).tmp

# Create a folder to hold the object files.

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

# Build the documentation

documentation: $(OUTDIR)/$(README)

$(OUTDIR)/$(README): $(MANUAL)/$(MANSRC)
	$(MANTOOLS) -MTEXT -I$(MANUAL)/$(MANSRC) -O$(OUTDIR)/$(README) -D'version=$(HELP_VERSION)' -D'date=$(HELP_DATE)'


# Build the release Zip file.

release: clean all
	$(RM) ../$(ZIPFILE)
	(cd $(OUTDIR) ; $(ZIP) $(ZIPFLAGS) ../../$(ZIPFILE) $(RUNIMAGE) $(README) $(LICENSE))
	$(RM) ../$(SRCZIPFILE)
	$(ZIP) $(SRCZIPFLAGS) ../$(SRCZIPFILE) $(OUTDIRLINUX) $(OUTDIRRO) $(SRCDIR) $(MANUAL) Makefile


# Build a backup Zip file

backup:
	$(RM) ../$(BUZIPFILE)
	$(ZIP) $(BUZIPFLAGS) ../$(BUZIPFILE) *


# Install the finished version in the GCCSDK, ready for use.

install: clean all
	$(CP) -r $(OUTDIR)/$(RUNIMAGE) $(SFTOOLS_BIN)


# Clean targets

clean:
	$(RM) $(OBJDIR)/*
	$(RM) $(OUTDIR)/$(RUNIMAGE)
	$(RM) $(OUTDIR)/$(README)

