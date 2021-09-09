
Configuration (config.mk)

RISC-V toolchain with support for vector (binutils)
RVVBENCH_RVV_SUPPORT=1

Platform independent -> without vector benchmarks
RVVBENCH_RVV_SUPPORT=0


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

Build for debug and install in /opt/rvvbench/bin
make prefix=/opt/rvvbench debug=1 install

Build without debug
make debug=0
or
make

Build without debug and install in /opt/rvvbench/bin
make prefix=/opt/rvvbench debug=0 install
or
make prefix=/opt/rvvbench install
