# RVVBMARK

Benchmark tool for RISC-V vector extension (RVV)
(C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>


## Configuration, Build & Install

### Configuration
> ```
> ./configure
> ```
determines if toolchain supports RISC-V and vector extension (64bit)
sets install prefix


### Build
> ```
> make
> ```

Variables:
"debug"
debug=1 .. no optimization, debug symbols, unstripped install
debug=0 .. optimization, no debug symbols, stripped install
Note: switching beween debug on/off requires a clean!


### Install
Install to configured prefix (default="/usr/local/bin")
> ```
> make install
> ```


### Clean
Clean all build artefacts
> ```
> make clean
> ```

Clean all build artefacts and configuration
> ```
> make distclean
> ```


### Examples

Configure build and install to /usr/local/bin
> ```
> ./configure
> make
> make install
> ```

Configure, build and install to /opt/rvvbmark/bin
> ```
> ./configure --prefix=opt/rvvbmark install
> make install
> ```

Build for debug (unstripped and with debug symbols)
> ```
> make clean
> make debug=1
> ```
