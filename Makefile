# Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
# All rights reserved.
#
# SPDX-License-Identifier: GPL-3.0-only

ifeq (,$(wildcard config.mk))
    $(error Not configured yet -> call configure / see README.md)
endif
include config.mk

debug?=0

BIN_NAME=rvvbmark
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
else
	# Benchmarks are built explicitly with/without autovectorization
	# independent of the settings here (see below)
	CFLAGS+=	-O2
	INSTALLFLAGS=	-s --strip-program=$(STRIP)
endif

ASFLAGS+= 	$(RVVBMARK_EXTRA_ASFLAGS)
CFLAGS+=	$(RVVBMARK_EXTRA_CFLAGS) \
		-Wall -std=c11 -D_GNU_SOURCE \
		-I. \
		-DRVVBMARK_VERSION_STR="\"$(VERSION_STR)\"" \
		-DRVVBMARK_RV_SUPPORT=$(RVVBMARK_RV_SUPPORT) \
		-DRVVBMARK_RVV_SUPPORT=$(RVVBMARK_RVV_SUPPORT)
LIBS+=		-lm
LDFLAGS+=

# Generally all C code is platform independent. Special cases are handled
# by ifdefs (RVVBMARK_RVV_SUPPORT) in code.

# All *.h files
HEADERS := $(wildcard *.h)

# All *.c files
C_SOURCES := $(wildcard *.c)

# All *.c.in files
# Will be compiled to multiple object files with different optimizations
C_SOURCES_OPT_IN := $(wildcard *_c.c.in)

# All .s files
ifeq ($(RVVBMARK_RV_SUPPORT),1)
  # All files ending with *_rv.s is for RISC-V
  ASM_SOURCES := $(wildcard *_rv.s)
  ifeq ($(RVVBMARK_RVV_SUPPORT),1)
    # All files ending with *_rvv.s is for RISC-V Vector extension
    ASM_SOURCES += $(wildcard *_rvv.s)
  endif
else
ASM_SOURCES :=
endif

# build %.o from %.c and %.s
OBJS := $(patsubst %.c,%.o,$(C_SOURCES))
OBJS += $(patsubst %.s,%.o,$(ASM_SOURCES))
# build multiple %.o with different optimizations from %.c.in
OBJS += $(patsubst %.c.in,%_avect.o,$(C_SOURCES_OPT_IN))
OBJS += $(patsubst %.c.in,%_noavect.o,$(C_SOURCES_OPT_IN))



.PHONY: all check style clean distclean install


all: $(BIN_NAME)

# generic rule
%.o: %.c $(HEADERS) config.mk
		$(CC) $(CFLAGS) -c $<

# generic rule for disabled autovectorization
# verbose info of vectorization is print on compilation (there should be no output!)
%_c_noavect.o: %_c.c.in $(HEADERS) config.mk
		@echo "BUILD $< WITHOUT VECTORIZATION"
		sed $< -e s/@OPTIMIZATION@/noavect/g | $(CC) $(CFLAGS) -O3 -fno-tree-vectorize -fopt-info-vec-all -c -o $@ -xc -

# generic rule for enabled autovectorization
# -O3 enables autovectorization (-ftree-vectorize) on x86(debian 10) and risc-v (risc-v foundation toolchain)
# verbose info of vectorization is print on compilation
%_c_avect.o: %_c.c.in $(HEADERS) config.mk
		@echo "BUILD $< WITH VECTORIZER"
		sed $< -e s/@OPTIMIZATION@/avect/g | $(CC) $(CFLAGS) -O3 -ftree-vectorize -fopt-info-vec-all -c -o $@ -xc -



$(BIN_NAME): $(OBJS) $(HEADERS) config.mk
		$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

check:
		cppcheck -q -f . ${C_SOURCES} ${C_SOURCES_OPT_IN} ${HEADERS}

style:
		(PWD=`pwd`; astyle $(ASTYLE_ARGS) $(C_SOURCES) ${C_SOURCES_OPT_IN} $(HEADERS);)

clean:
		- rm $(BIN_NAME)
		- rm *.o

distclean: clean
		- rm config.mk

install: all
		-@mkdir -p $(INSTALL_BIN_DIR)
		$(INSTALL) -m 755 $(INSTALLFLAGS) $(BIN_NAME) $(INSTALL_BIN_DIR)
