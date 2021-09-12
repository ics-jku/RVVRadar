
Configuration (config.mk)

RISC-V toolchain
RVVBMARK_RV_SUPPORT=1

Platform independent -> without RISC-V and vector benchmarks
RVVBMARK_RV_SUPPORT=0

Only if RVVBMARK_RV_SUPPORT=1
RISC-V toolchain with support for vector (binutils) (64bit only!)
RVVBMARK_RVV_SUPPORT=1

RISC-V without vector benchmarks
RVVBMARK_RVV_SUPPORT=0


Build & Install

"debug"
debug=1 .. no optimization, debug symbols, unstripped install
debug=0 .. optimization, no debug symbols, stripped install
Note: switching beween debug on/off requires a clean!

"prefix"
prefix for install target

Examples:

Build for debug
make debug=1 

Build for debug and install in /usr/local/bin
make debug=1 install

Build for debug and install in /opt/rvvbmark/bin
make prefix=/opt/rvvbmark debug=1 install

Build without debug
make debug=0
or
make

Build without debug and install in /opt/rvvbmark/bin
make prefix=/opt/rvvbmark debug=0 install
or
make prefix=/opt/rvvbmark install
