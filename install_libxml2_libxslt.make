# Install libxml2 and libxslt with options suitable for lmi.
#
# Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 Gregory W. Chicares.
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
# https://savannah.nongnu.org/projects/lmi
# email: <gchicares@sbcglobal.net>
# snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

# Configurable settings ########################################################

# Archives and their md5sums

libxml2_version := libxml2-2.9.4
libxslt_version := libxslt-1.1.29

libxml2-2.9.4.tar.gz-md5  := ae249165c173b1ff386ee8ad676815f5
libxslt-1.1.29.tar.gz-md5 := a129d3c44c022de3b9dcf6d6f288d72e

# Libraries are ordered by dependency, rather than alphabetically.
libraries := $(libxml2_version) $(libxslt_version)

source_archives := $(addsuffix .tar.gz, $(libraries))

# 'ftp://xmlsoft.org' has 'libxml2/' and 'libxslt/' subdirectories,
# but they seem to be identical: the latter is apparently a link to
# the former, so only the former is used here.

host          := ftp://xmlsoft.org
host_path     := libxml2

mingw_dir     := /opt/lmi/${LMI_COMPILER}_${LMI_TRIPLET}/gcc_msw

prefix        := /opt/lmi/local
exec_prefix   := $(prefix)/$(LMI_COMPILER)_$(LMI_TRIPLET)

cache_dir     := /srv/cache_for_lmi/downloads

build_dir     := $(exec_prefix)/xml-ad_hoc

# Variables that normally should be left alone #################################

mingw_bin_dir :=

lmi_build_type := $(shell /usr/share/libtool/build-aux/config.guess)

ifeq (cygwin,$(findstring cygwin,$(lmi_build_type)))
  mingw_bin_dir := $(mingw_dir)/bin/
endif

# For 'host' and 'build' configure options, see:
#   https://cygwin.com/ml/cygwin/2002-01/msg00837.html
# Do not use '-fno-omit-frame-pointer' because of an ICE:
#   https://lists.nongnu.org/archive/html/lmi/2019-06/msg00018.html

# '--disable-dependency-tracking' is required with the MinGW toolchain
# in a Cygwin shell, to prevent a catastrophic dependency-tracking
# failure. Apparently the problem is colons in header paths, e.g.:
#   c:/MinGW-20050827/bin/../lib/gcc/mingw32/3.4.4/include/stddef.h:
# which elicit fatal errors such as this:
#   .deps/DOCBparser.Plo:1: *** multiple target patterns.  Stop.

xmlsoft_common_cflags := \
  -fno-ms-extensions \
  -frounding-math \
  -Wno-cpp \
  -Wno-discarded-qualifiers \
  -Wno-format \
  -Wno-format-extra-args \
  -Wno-implicit-fallthrough \
  -Wno-implicit-function-declaration \
  -Wno-maybe-uninitialized \
  -Wno-misleading-indentation \
  -Wno-missing-format-attribute \
  -Wno-missing-prototypes \
  -Wno-nested-externs \
  -Wno-pedantic \
  -Wno-pointer-sign \
  -Wno-sign-compare \
  -Wno-strict-prototypes \
  -Wno-suggest-attribute=format \
  -Wno-unused-but-set-variable \
  -Wno-unused-function \
  -Wno-unused-parameter \
  -Wno-unused-variable \

xmlsoft_common_options := \
  --disable-dependency-tracking \
  --disable-static \
  --enable-shared \
  --with-debug \
  --without-python \
  LDFLAGS='-lws2_32' \
  CPPFLAGS="-w" \
  CFLAGS="-g -O2 $(xmlsoft_common_cflags)" \

$(libxml2_version)_options := \
  --prefix=$(prefix) \
  --exec-prefix=$(exec_prefix) \
  --build=`$(build_dir)/$(libxml2_version)/config.guess` \
  --host=$(LMI_TRIPLET) \
  $(xmlsoft_common_options) \
  --without-lzma \
  --with-schemas \
  --without-iconv \
  --without-modules \
  --without-schematron \
  --without-zlib \

# The '--with-libxml-prefix' option is documented thus:
#   "Specify location of libxml config"
# and libxml2 installs 'xml2-config' under its own $exec_prefix if
# that differs from its own $prefix. It would seem clearer if this
# libxslt option were named '--with-libxml-exec-prefix'.

$(libxslt_version)_options := \
  --prefix=$(prefix) \
  --exec-prefix=$(exec_prefix) \
  --build=`$(build_dir)/$(libxslt_version)/config.guess` \
  --host=$(LMI_TRIPLET) \
  $(xmlsoft_common_options) \
  --with-libxml-prefix=$(exec_prefix) \
  --without-crypto \

# Utilities ####################################################################

CHMOD  := chmod
ECHO   := echo
GREP   := grep
MD5SUM := md5sum
MKDIR  := mkdir
PATCH  := patch
RM     := rm
TAR    := tar
WGET   := wget

# Targets ######################################################################

.PHONY: all
all: clobber_exec_prefix_only $(source_archives) $(libraries)

# Order-only prerequisites.

$(libxslt_version):| $(libxml2_version)
$(libraries)      :| $(source_archives)
$(source_archives):| initial_setup
initial_setup     :| clobber_exec_prefix_only

.PHONY: initial_setup
initial_setup:
	[ -n "$$LMI_COMPILER" ] || { printf '%s\n' "no LMI_COMPILER" && false; }
	[ -n "$$LMI_TRIPLET"  ] || { printf '%s\n' "no LMI_TRIPLET"  && false; }
	$(MKDIR) --parents $(prefix)
	$(MKDIR) --parents $(exec_prefix)
	$(MKDIR) --parents $(cache_dir)
	$(MKDIR) --parents $(build_dir)

# $(WGETFLAGS) and $(wget_whence) must be recursively expanded because
# $(host) and $(host_path) have target-specific values.

WGETFLAGS = \
  --cut-dirs=$(words $(subst /, ,$(host_path))) \
  --force-directories \
  --no-host-directories \
  --no-verbose \

wget_whence = $(host)/$(host_path)

TARFLAGS := --keep-old-files
%.tar.bz2: TARFLAGS += --bzip2
%.tar.gz:  TARFLAGS += --gzip

.PHONY: %.tar.bz2 %.tar.gz
%.tar.bz2 %.tar.gz:
	cd $(cache_dir) && [ -e $@ ] || $(WGET) $(WGETFLAGS) $(wget_whence)/$@
	cd $(cache_dir)/$(dir $@) && \
	  $(ECHO) "$($@-md5) *$@" | $(MD5SUM) --check
	$(TAR) --extract $(TARFLAGS) --directory=$(build_dir) --file=$(cache_dir)/$@

# autotools: 'make install' doesn't respect group permissions--see:
#   https://lists.gnu.org/archive/html/automake/2019-01/msg00000.html
# After the 'chmod' calls, the 'find' command should find nothing.
#
# Someday it may be necessary to add a line like this to the recipe:
#   export lt_cv_to_tool_file_cmd=func_convert_file_cygwin_to_w32
# but that's not necessary for now. See:
#   https://lists.nongnu.org/archive/html/lmi/2011-06/msg00025.html

.PHONY: $(libraries)
$(libraries):
	-[ -e $@-lmi.patch ] && $(PATCH) --directory=$(build_dir) --strip=1 <$@-lmi.patch
	cd $(build_dir)/$@ \
	  && export PATH="$(mingw_bin_dir):${PATH}" \
	  && PKG_CONFIG_PATH="$(exec_prefix)/lib/pkgconfig" \
	    $($@_overrides) ./configure $($@_options) \
	  && $(MAKE) && $(MAKE) install
	-$(CHMOD) -R g=u $(build_dir)
	-$(CHMOD) -R g=u $(prefix)/include/libexslt
	-$(CHMOD) -R g=u $(prefix)/include/libxml2
	-$(CHMOD) -R g=u $(prefix)/include/libxslt
	-$(CHMOD) -R g=u $(prefix)/share/doc/libxml2-*
	-$(CHMOD) -R g=u $(prefix)/share/doc/libxslt-*
	-$(CHMOD) -R g=u $(prefix)/share/gtk-doc/html/libxml2
	-$(CHMOD) -R g=u $(exec_prefix)/bin
	-$(CHMOD) -R g=u $(exec_prefix)/lib
	-$(CHMOD) -R g=u $(prefix)/share/aclocal
	-$(CHMOD) -R g=u $(prefix)/share/man
	find $(prefix) -perm -200 -not -perm -020

# Nonchalantly remove pkgconfig and cmake subdirectories, even though
# other libraries might someday write files in them, because lmi never
# uses them outside of this makefile.
#
# It would probably be much better to use an 'uninstall' target in
# each package's makefile.

.PHONY: clobber_exec_prefix_only
clobber_exec_prefix_only:
	[ -n "$$LMI_COMPILER" ] || { printf '%s\n' "no LMI_COMPILER" && false; }
	[ -n "$$LMI_TRIPLET"  ] || { printf '%s\n' "no LMI_TRIPLET"  && false; }
	-$(RM) --force --recursive $(exec_prefix)/bin/*xml2*
	-$(RM) --force --recursive $(exec_prefix)/bin/*xslt*
	-$(RM) --force --recursive $(exec_prefix)/bin/xmllint*
	-$(RM) --force --recursive $(exec_prefix)/bin/xmlcatalog*
	-$(RM) --force --recursive $(exec_prefix)/include/libxml2
	-$(RM) --force --recursive $(exec_prefix)/include/libxslt
	-$(RM) --force --recursive $(exec_prefix)/include/libexslt
	-$(RM) --force --recursive $(exec_prefix)/lib/*xml2*
	-$(RM) --force --recursive $(exec_prefix)/lib/*xslt*
	-$(RM) --force --recursive $(exec_prefix)/lib/cmake
	-$(RM) --force --recursive $(exec_prefix)/lib/pkgconfig
	-$(RM) --force --recursive $(build_dir)
