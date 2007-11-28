# Installer for MinGW.
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

# $Id: mingw_install.make,v 1.10 2007-11-28 16:55:29 chicares Exp $

# Configurable settings ########################################################

# Use the 2005-08-27 version by default.

version  := MinGW-20050827

file_list = $($(version))

# Prefer to set $(prefix) to anything but '/mingw', in order to avoid
# the problem described here:
#   http://sourceforge.net/mailarchive/message.php?msg_id=10579421
#   http://sourceforge.net/mailarchive/message.php?msg_id=10581810
# when multiple versions of MinGW gcc are installed.

prefix   := /trial/$(version)

# In the past, it seemed necessary to specify a mirror, e.g.:
#  mirror := http://easynews.dl.sourceforge.net/sourceforge/mingw
# but as of about 2006-12 sf.net seems to select one automatically
# when this is passed to wget:
mirror   := http://downloads.sourceforge.net/mingw

# File lists ###################################################################

# $(MinGW-20061119): 'Candidate' versions from
#   Chris Sutcliffe's 2006-11-19T02:27Z email to MinGW-dvlpr,
# but see
#   Keith MARSHALL's 2006-12-02T20:30Z email to Mingw-users.

MinGW-20061119 = \
  binutils-2.16.91-20060119-1.tar.gz \
  gcc-core-3.4.5-20060117-1.tar.gz \
  gcc-g++-3.4.5-20060117-1.tar.gz \
  mingw-runtime-3.11.tar.gz \
  w32api-3.8.tar.gz \

MinGW-20060119 = \
  binutils-2.16.91-20060119-1.tar.gz \
  gcc-core-3.4.5-20060117-1.tar.gz \
  gcc-g++-3.4.5-20060117-1.tar.gz \
  mingw-runtime-3.9.tar.gz \
  w32api-3.6.tar.gz \

# This, I think, is equivalent to the C++ portion of 'MinGW-5.0.0.exe'.

MinGW-20050827 = \
  binutils-2.16.91-20050827-1.tar.gz \
  gcc-core-3.4.4-20050522-1.tar.gz \
  gcc-g++-3.4.4-20050522-1.tar.gz \
  mingw-runtime-3.8.tar.gz \
  w32api-3.3.tar.gz \

# TODO ?? Add file lists for earlier releases.

# Archive md5sums ##############################################################

binutils-2.16.91-20050827-1.tar.gz-md5 := 9d2de9e3cd5fede3d12150b8a7d0bbf7
binutils-2.16.91-20060119-1.tar.gz-md5 := a54f33ca9d6cf56dc99c0c5367f58ea3
gcc-core-3.4.4-20050522-1.tar.gz-md5   := 46f17998ab61be9dbede233b44c7b0e6
gcc-core-3.4.5-20060117-1.tar.gz-md5   := 1a4afae471ea93b975e3f8b3ac529eac
gcc-g++-3.4.4-20050522-1.tar.gz-md5    := db44ac5b06d7f262c59422ae21511659
gcc-g++-3.4.5-20060117-1.tar.gz-md5    := d11a9d63a0f862650f755fdb4e947dc4
mingw-runtime-3.8.tar.gz-md5           := 5852e9b2c369aff1d4ba47d3dd20728f
mingw-runtime-3.9.tar.gz-md5           := 0cb66b1071da224ea2174f960c593e2e
mingw-runtime-3.11.tar.gz-md5          := e1c21f8c4ece49d8bd9fef9e1b0e44a7
w32api-3.3.tar.gz-md5                  := 2da21c26013711ae90d3b2416c20856e
w32api-3.6.tar.gz-md5                  := 2f86ec42cafd774ec82162fbc6e6808d
w32api-3.8.tar.gz-md5                  := b53fdf670f33d2e901749f4792e659f2

# Utilities ####################################################################

ECHO   := echo
GREP   := grep
MD5SUM := md5sum
MKDIR  := mkdir
MV     := mv
RM     := rm
TAR    := tar
WGET   := wget

# Error messages ###############################################################

wget_missing = \
  "\nError: Unable to find '$(WGET)', which is required for" \
  "\nautomated downloads. Download it manually from the MinGW" \
  "\nsite and install it on your PATH." \
  "\n"

prefix_exists = \
  "\nError: Prefix directory '$(prefix)' already exists." \
  "\nIt is generally unsafe to install one version of a program" \
  "\non top of another. Probably you ought to rename the old" \
  "\nversion in order to preserve it; if not, then remove it." \
  "\n"

scratch_exists = \
  "\nError: Scratch directory 'scratch' already exists." \
  "\nProbably it is left over from a previous failure." \
  "\nJust remove it unless you're sure you want whatever files" \
  "\nit might contain." \
  "\n"

# Targets ######################################################################

.PHONY: all
all: $(file_list)
	@$(MV) scratch $(prefix)

$(file_list): initial_setup

.PHONY: initial_setup
initial_setup:
	@type "$(WGET)" >/dev/null || { $(ECHO) -e $(wget_missing)   && false; }
	@[ ! -e $(prefix) ]        || { $(ECHO) -e $(prefix_exists)  && false; }
	@[ ! -e scratch   ]        || { $(ECHO) -e $(scratch_exists) && false; }
	@$(MKDIR) --parents $(prefix)
	@$(RM) --force --recursive $(prefix)
	@$(MKDIR) --parents scratch

# Some files are duplicated in MinGW archives, so the order of
# extraction is important. It follows these instructions:
#   http://groups.yahoo.com/group/mingw32/message/1145
# It is apparently fortuitous that the order is alphabetical.

# Some gcc archives distributed by MinGW contain a version of
# 'libiberty.a' that's incompatible with the version provided with
# binutils. See:
#   http://sourceforge.net/mailarchive/message.php?msg_id=4456861
# to learn why this may matter a great deal.
#
# Other conflicts like that have been known to occur. For instance,
# these archives
#   binutils-2.16.91-20050827-1.tar.gz
#   gcc-core-3.4.4-20050522-1.tar.gz
# contain 'info/dir' files that differ.
#
# For the moment, gcc's 'libiberty.a' is explicitly excluded because
# it has sometimes been significantly wrong, and it seems reasonable
# to expect binutils to provide a 'libiberty.a' that works with the
# 'libbfd.a' it also provides. However, other conflicts are managed by
# specifying '--keep-old-files' and permitting 'tar' to fail with an
# error message identifying any conflict.

TARFLAGS := --keep-old-files
%.tar.bz2: TARFLAGS += --bzip2
%.tar.gz:  TARFLAGS += --gzip
gcc%:      TARFLAGS += --exclude 'libiberty.a' --exclude 'info/dir'

# New spelling '--no-verbose' has replaced original '--non-verbose':
#   http://sourceware.org/ml/cygwin-apps/2005-10/msg00140.html

#WGETFLAGS := --no-verbose --timestamping
WGETFLAGS := --timestamping

.PHONY: %.tar.bz2 %.tar.gz
%.tar.bz2 %.tar.gz:
	[ -e $@ ] || $(WGET) $(WGETFLAGS) $(mirror)/$@
	$(ECHO) "$($@-md5) *$@" | $(MD5SUM) --check
	-$(TAR) --extract $(TARFLAGS) --directory=scratch --file=$@

# Test #########################################################################

ifeq (3.81,$(firstword $(sort $(MAKE_VERSION) 3.81)))
  this_makefile := $(lastword $(MAKEFILE_LIST))
else
  $(error Upgrade to make-3.81)
endif

test_prefix = /eraseme

test_file_list = \
  gcc-response-file-2.tar.gz \
  zlib-1.2.3-mingwPORT.tar.bz2 \

# Archive md5sums for $(test_file_list).
gcc-response-file-2.tar.gz-md5         := aa9825791000af0e4d4d0345bd4325ce
zlib-1.2.3-mingwPORT.tar.bz2-md5       := e131ea48214af34bd6adee6b7bdadfd5

test_overrides = \
  --no-print-directory \
     --file=$(this_makefile) \
     prefix='$(test_prefix)' \
  file_list='$(test_file_list)' \
  WGETFLAGS='--timestamping' \

#  WGETFLAGS='--no-verbose --timestamping' \

.PHONY: test
test:
	@$(ECHO) "Expect some ignored errors, then a success message"
	@$(RM) --force $(test_file_list)
	@$(RM) --force --recursive $(test_prefix)
	@$(RM) --force --recursive scratch
	@$(MKDIR) scratch
	@$(ECHO) "  test 0: expect an error in 'initial_setup' "
	-@$(MAKE) $(test_overrides) >test_0
	@$(GREP) --silent \
	  "Error: Scratch directory 'scratch' already exists." test_0
	@$(RM) test_0
	@$(RM) --force --recursive scratch
	@$(MKDIR) $(test_prefix)
	@$(ECHO) "  test 1: expect another error in 'initial_setup' "
	-@$(MAKE) $(test_overrides) >test_1
	@$(GREP) --silent \
	  "Error: Prefix directory '$(test_prefix)' already exists." test_1
	@$(RM) test_1
	@$(RM) --force --recursive $(test_prefix)
	@$(ECHO) "  test 2: expect to get two files and install all files"
	@$(MAKE) $(test_overrides)
	@$(MD5SUM) --check --status test.md5
	@$(RM) --force $(firstword $(test_file_list))
	@$(RM) --force --recursive $(test_prefix)
	@$(ECHO) "  test 3: expect to get one file and install all files"
	@$(MAKE) $(test_overrides)
	@$(MD5SUM) --check --status test.md5
	@$(RM) --force --recursive $(test_prefix)
	@$(ECHO) "  test 4: expect to get no file and install all files"
	@$(MAKE) $(test_overrides)
	@$(MD5SUM) --check --status test.md5
	@$(RM) --force --recursive $(test_prefix)
	@$(ECHO) "All tests succeeded"

