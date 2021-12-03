# RVVRadar

Framework to support the development of optimized algorithm implementations
for the RISC-V vector extension (RVV).
(C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>

RVVRadar is mainly released under the terms of the GNU General Public License
version 3.0. To facilitate reuse of algorithm implementations, these
implementations are licensed under the terms of the MIT permissive license.
See the file COPYING for details.

Supported RVV Drafts (autodetected by configure):
 * v0.7
   * implemented
   * tested on allwinner d1
 * v0.8
   * implemented
   * untested
 * v0.9/v0.10/v1.0
   * implemented
   * untested


## Directory Structure
 * core .. RVVRadar framework
 * algorithms .. algorithms and their implementations



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

Configure, build and install to /opt/rvvradar/bin
> ```
> ./configure --prefix=/opt/rvvradar install
> make install
> ```

Build for debug (unstripped and with debug symbols)
> ```
> make clean
> make debug=1
> ```
