# Install msw-native libxml2 and libxslt with options suitable for lmi.
#
# Copyright (C) 2006, 2007 Gregory W. Chicares.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#
# http://savannah.nongnu.org/projects/lmi
# email: <chicares@cox.net>
# snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

# $Id: install_libxml2_libxslt.make,v 1.6 2007-11-12 04:44:49 chicares Exp $

# Configurable settings ########################################################

# Libraries are ordered by dependency, rather than alphabetically.
source_archives := \
  libxml2/2.6/libxml2-2.6.26.tar.bz2 \
  libxslt/1.1/libxslt-1.1.17.tar.bz2 \

host          := ftp://ftp.gnome.org
host_path     := pub/GNOME/sources

mingw_root    := /cygdrive/c
mingw_dir     := MinGW-20050827

prefix        := /opt/lmi/local

xml_dir       := /opt/lmi/xml-scratch

# Variables that normally should be left alone #################################

date           = $(shell date -u +'%Y%m%dT%H%MZ')

mingw_bin_dir := $(mingw_root)/$(mingw_dir)/bin

ifeq (3.81,$(firstword $(sort $(MAKE_VERSION) 3.81)))
  this_makefile := $(abspath $(lastword $(MAKEFILE_LIST)))
else
  $(error Upgrade to make-3.81 .)
endif

# '--disable-dependency-tracking' is required with the MinGW toolchain
# in a Cygwin shell, to prevent a catastrophic dependency-tracking
# failure. Apparently the problem is colons in header paths, e.g.:
#   c:/MinGW-20050827/bin/../lib/gcc/mingw32/3.4.4/include/stddef.h:
# which elicit fatal errors such as this:
#   .deps/DOCBparser.Plo:1: *** multiple target patterns.  Stop.

common_options := \
  --disable-dependency-tracking \
  --disable-static \
  --enable-shared \
  --with-debug \
  --without-python \
  CC='$(mingw_bin_dir)/gcc' \
  LD='$(mingw_bin_dir)/ld' \
  LDFLAGS='-lws2_32' \

libxml2-2.6.26_options := \
  $(common_options) \
  --with-schemas \
  --without-iconv \
  --without-modules \
  --without-schematron \

libxslt-1.1.17_options := \
  $(common_options) \
  --with-libxml-prefix=$(prefix) \
  --without-crypto \

# Utilities ####################################################################

ECHO   := echo
GREP   := grep
MD5SUM := md5sum
MKDIR  := mkdir
PATCH  := patch
RM     := rm
TAR    := tar
WGET   := wget

# Error messages ###############################################################

wget_missing = \
  "\nError: Unable to find '$(WGET)', which is required for" \
  "\nautomated downloads. Install it on your PATH." \
  "\n"

# Targets ######################################################################

libraries := $(source_archives:.tar.bz2=)

.PHONY: all
all: clobber $(source_archives) $(libraries)

# Simulated order-only prerequisites.
$(libraries): $(source_archives)
$(source_archives): initial_setup
initial_setup: clobber

.PHONY: initial_setup
initial_setup:
	@type "$(WGET)" >/dev/null || { $(ECHO) -e $(wget_missing) && false; }
	@$(MKDIR) --parents $(prefix)
	@$(MKDIR) --parents $(xml_dir)

WGETFLAGS := \
  --cut-dirs=$(words $(subst /, ,$(host_path))) \
  --force-directories \
  --no-host-directories \
  --timestamping \

wget_whence := $(host)/$(host_path)

%.tar.bz2: decompress = --bzip2
%.tar.gz:  decompress = --gzip

.PHONY: %.tar.bz2 %.tar.gz
%.tar.bz2 %.tar.gz:
	@[ -e $*.md5sum ] || $(WGET) $(WGETFLAGS) $(wget_whence)/$*.md5sum
	@[ -e $@        ] || $(WGET) $(WGETFLAGS) $(wget_whence)/$@
	cd $(dir $@) && \
	  $(GREP) $(notdir $@) $(notdir $*).md5sum | $(MD5SUM) --check --status -
	$(TAR) --extract $(decompress) --directory=$(xml_dir) --file=$@

# TODO ?? Get rid of the 'LOG' stuff once this makefile is stabilized.

.PHONY: $(libraries)
$(libraries):
	export LOG=log-$(date); \
	$($(notdir $@)_exports) \
	$(RM) --force $$LOG; \
	cd $(xml_dir)/$(notdir $@) && \
	  ./configure --prefix=$(prefix) $($(notdir $@)_options) >>$$LOG 2>&1; \
	  $(MAKE)                                                >>$$LOG 2>&1; \
	  $(MAKE) install                                        >>$$LOG 2>&1; \

.PHONY: libxslt/1.1/libxslt-1.1.17
libxslt/1.1/libxslt-1.1.17: patch_libxslt
libxslt/1.1/libxslt-1.1.17: libxml2/2.6/libxml2-2.6.26

# Patches ######################################################################

# libxslt-1.1.17.patch

.PHONY: patch_libxslt
patch_libxslt: libxslt/1.1/libxslt-1.1.17.tar.bz2
	$(PATCH) --directory=$(xml_dir) --strip=1 < libxslt-1.1.17-lmi.patch

# TODO ?? This seems too elaborate and expensive for what it really
# does. As for the install directory, it removes libxml2 fairly well,
# but not libxslt or libexslt...but see update below...
#
# Update: placing $(xml_dir) on a specially-mounted directory
#   C:\opt\lmi on /opt/lmi type system (binmode)
# "fixes" that problem.

.PHONY: clobber
clobber:
	-for z in $(notdir $(libraries)); \
	  do cd $(xml_dir)/$$z && make uninstall maintainer-clean; \
	  done;
	-for z in $(notdir $(libraries)); \
	  do \
	    shopt -s extglob; \
	    echo "Removing non-log files in $$z"; \
	    { for f in $(xml_dir)/$$z/'!(log-*)'; \
	      do \
	        rm --recursive $$f; \
	      done; \
	    }; \
	  done;

