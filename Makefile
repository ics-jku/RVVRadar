# Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
#
# SPDX-License-Identifier: GPL-3.0-only

ifeq (,$(wildcard config.mk))
    $(error Not configured yet -> call configure / see README.md)
endif
include config.mk

debug?=0

BIN_NAME=rvvbmark
COREDIR=core
ALGDIR=algorithms
ASTYLE_ARGS=--options=none --suffix=none --quiet \
	    --style=linux --indent=force-tab=8 --pad-header --pad-oper --indent-preprocessor
VERSION_STR=$(BIN_NAME)-${RVVBMARK_VERSION}.${RVVBMARK_SUBVERSION}
INSTALL_BIN_DIR="$(RVVBMARK_INSTALL_PREFIX)/bin"
PKG_CONFIG ?= "pkg-config"
INSTALL ?= install
STRIP ?= strip

# debug handling
ifeq ($(debug),1)
	# make debug=1
	# no optimization, with debug symbols install unstripped
	CFLAGS+=	-Og -g
	INSTALLFLAGS=
	OBJDIR=		.obj/debug
else
	# Algorithm implementations are built explicitly with/without
	# autovectorization independent of the settings here (see below)
	CFLAGS+=	-O2
	INSTALLFLAGS=	-s --strip-program=$(STRIP)
	OBJDIR=		.obj/release
endif

CFLAGS+=	$(RVVBMARK_EXTRA_CFLAGS) \
		-Wall -D_GNU_SOURCE \
		-I. \
		-DRVVBMARK_VERSION_STR="\"$(VERSION_STR)\"" \
		-DRVVBMARK_RV_SUPPORT=$(RVVBMARK_RV_SUPPORT) \
		-DRVVBMARK_RVV_SUPPORT=$(RVVBMARK_RVV_SUPPORT)
LIBS+=		-lm
LDFLAGS+=

# Generally all C code is platform independent. Special cases are handled
# by ifdefs (RVVBMARK_RVV_SUPPORT) in code.

# All *.h files
HEADERS := $(wildcard $(COREDIR)/*.h) $(wildcard $(ALGDIR)/*/*.h)

# All *.c files
C_SOURCES := $(wildcard *.c) $(wildcard $(COREDIR)/*.c) $(wildcard $(ALGDIR)/*/*.c)

# All *.c.in files
# Will be compiled to multiple object files with different optimizations
C_SOURCES_OPT_IN := $(wildcard *_c.c.in) $(wildcard $(ALGDIR)/*/*_c.c.in)

# build %.o from %.c and %.s
OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(C_SOURCES))
# build multiple %.o with different optimizations from %.c.in
OBJS += $(patsubst %.c.in,$(OBJDIR)/%_avect.o,$(C_SOURCES_OPT_IN))
OBJS += $(patsubst %.c.in,$(OBJDIR)/%_noavect.o,$(C_SOURCES_OPT_IN))



.PHONY: all check style clean distclean install create_obj_dir


all: $(BIN_NAME)

create_obj_dir:
		@for o in $(OBJS) ; do mkdir -p `dirname $${o}` ; done

# generic rule
$(OBJDIR)/%.o: %.c $(HEADERS) config.mk | create_obj_dir
		$(CC) $(CFLAGS) -c $< -o $@

# generic rule for disabled autovectorization
# verbose info of vectorization is print on compilation (there should be no output!)
$(OBJDIR)/%_c_noavect.o: %_c.c.in $(HEADERS) config.mk | create_obj_dir
		@echo "BUILD $< WITHOUT VECTORIZATION"
		sed $< -e s/@OPTIMIZATION@/noavect/g | $(CC) $(CFLAGS) -O3 -fno-tree-vectorize -fopt-info-vec-all -c -o $@ -xc -

# generic rule for enabled autovectorization
# -O3 enables autovectorization (-ftree-vectorize) on x86(debian 10) and risc-v (risc-v foundation toolchain)
# verbose info of vectorization is print on compilation
$(OBJDIR)/%_c_avect.o: %_c.c.in $(HEADERS) config.mk | create_obj_dir
		@echo "BUILD $< WITH VECTORIZER"
		sed $< -e s/@OPTIMIZATION@/avect/g | $(CC) $(CFLAGS) -O3 -ftree-vectorize -fopt-info-vec-all -c -o $@ -xc -



$(BIN_NAME): $(OBJS) $(HEADERS) config.mk
		$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

check:
		cppcheck -q -f . ${C_SOURCES} ${C_SOURCES_OPT_IN} ${HEADERS}

style:
		(PWD=`pwd`; astyle $(ASTYLE_ARGS) $(C_SOURCES) ${C_SOURCES_OPT_IN} $(HEADERS);)

clean:
		- rm -rf .obj
		- rm -f $(BIN_NAME)

distclean: clean
		- rm config.mk

install: all
		-@mkdir -p $(INSTALL_BIN_DIR)
		$(INSTALL) -m 755 $(INSTALLFLAGS) $(BIN_NAME) $(INSTALL_BIN_DIR)
