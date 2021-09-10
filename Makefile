include config.mk
debug?=0
prefix?=/usr/local

BIN_NAME=rvvbench
ASTYLE_ARGS=--options=none --suffix=none --quiet \
	    --style=linux --indent=force-tab=8 --pad-header --pad-oper --indent-preprocessor
VERSION_STR=$(BIN_NAME)-${RVVBENCH_VERSION}.${RVVBENCH_SUBVERSION}
INSTALL_DIR="$(prefix)/bin"
PKG_CONFIG ?= "pkg-config"
INSTALL ?= install

# debug handling
ifeq ($(debug),1)
	# make debug=1
	# no optimization, with debug symbols install unstripped
	CFLAGS+=	-O0 -g
	INSTALLFLAGS=
else
	# TODO: measure different settings
	CFLAGS+=	-O2
	INSTALLFLAGS=	-s
endif

CFLAGS+=	-Wall -std=c11 -D_GNU_SOURCE \
		-I. \
		-DRVVBENCH_VERSION_STR="\"$(VERSION_STR)\"" \
		-DRVVBENCH_RV_SUPPORT=$(RVVBENCH_RV_SUPPORT) \
		-DRVVBENCH_RVV_SUPPORT=$(RVVBENCH_RVV_SUPPORT)
LIBS+=
LDFLAGS+=

HEADERS := $(wildcard *.h)

# Generally all C code is platform independent. Special cases are handled
# by ifdefs (RVVBENCH_RVV_SUPPORT) in code.
C_SOURCES := $(wildcard *.c)

ifeq ($(RVVBENCH_RV_SUPPORT),1)
  # All files ending with *_rv.s is for RISC-V
  ASM_SOURCES := $(wildcard *_rv.s)
  ifeq ($(RVVBENCH_RVV_SUPPORT),1)
    ASFLAGS += -march=rv64imafdcv
    # All files ending with *_rvv.s is for RISC-V Vector extension
    ASM_SOURCES += $(wildcard *_rvv.s)
  endif
else
ASM_SOURCES :=
endif

OBJS := $(patsubst %.c,%.o,$(C_SOURCES)) $(patsubst %.s,%.o,$(ASM_SOURCES))


all: $(BIN_NAME)

# generic rule
%.o: %.c %.h $(HEADERS)
		$(CC) $(CFLAGS) -c $<

$(BIN_NAME): $(OBJS) $(HEADERS)
		$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

check:
		cppcheck -q -f . ${C_SOURCES} ${HEADERS}

style:
		(PWD=`pwd`; astyle $(ASTYLE_ARGS) $(C_SOURCES) $(HEADERS);)

clean:
		- rm $(BIN_NAME)
		- rm *.o

install: all
		-@mkdir -p $(INSTALL_DIR)
		$(INSTALL) -m 755 $(INSTALLFLAGS) $(BIN_NAME) $(INSTALL_DIR)

install_target: install

uninstall:
		-rm -f $(INSTALL_DIR)/$(BIN_NAME)
		
uninstall_target: uninstall
