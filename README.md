# RVVRadar

(C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>

RVVRadar is a framework to support the programmer over the four major steps of
development, verification, measurement and evaluation during the vectorization
process of algorithms for the RISC-V Vector extension (RVV).

Development was started in the context of a bachelor theses at the Institute
for Complex Systems (ICS), JKU Linz. Special thanks to Dr. Daniel Große and
Lucas Klemmer, MSc for advise and mentoring.

RVVRadar is mainly released under the terms of the GNU General Public License
version 3.0. To facilitate reuse of included algorithm implementations, these
implementations are licensed under the terms of the MIT permissive license.
See the file COPYING for details.

Supported RVV Drafts (autodetected by configure):
 * v0.7
   * implemented
   * tested on Allwinner D1
 * v0.8
   * implemented
   * tested on Allwinner D1 (with binary compatible v0.7.1)
 * v0.9/v0.10/v1.0
   * implemented
   * v0.9 tested on Allwinner D1 (with binary compatible v0.7.1)
   * v0.10 and v1.0 tested on qemu_sifive (rvv-1.0-upstream-v10)
     using linux-5.15.6 + vector v9


## Motivation
Leveraging the vector potential in the software is a very challenging
task. Multiple development iterations when implementing/optimizing
algorithms for RVV including verification, measurements, and evaluation
are necessary. Moreover, algorithms usually serve a practical purpose in
a bigger context and are therefore integrated into larger software systems.
This leads to five practical challenges:
 1. run-time results of algorithms that are deeply integrated into large
    software systems are often difficult to access and therefore hard to
    verify,
 1. instrumentation for run-time measurements is hard to integrate and
    maintain in existing software systems,
 1. instrumentation must be specifically implemented for every software
    system,
 1. run-time instrumentation and verification is specific for every software
    system which makes it hard to get comparable results, and finally
 1. the performance of specific implementations may vary between different
    hardware platforms.

RVVRadar tackles these challenges. It provides a standardized framework
to support programmers in development, verification, measurement
and evaluation of vectorized algorithms.

RVVRadar follows a bottom-up design approach. When a new algorithm has to be
implemented for some target software system, it is added to the framework
including a baseline implementation. In an iterative manner, programmers can
add new optimized implementations to RVVRadar. When the framework is executed
on a target hardware platform it automatically verifies and measures the
performance of all implementations and generates standardized statistics.
Based on these statistics the implementations can be optimized or a completely
new implementation can be added.


## Directory Structure
 * core .. RVVRadar framework
   * algset.c/h .. Main framework and API
   * chrono.c/h .. Timing measurement and statistics
   * rvv_helpers.h .. rvv helper macros to support different RVV drafts
 * algorithms .. Included Algorithms and their implementations
   * memcpy .. "simple" copy of elements from one memory location to another.
   * mac_8_16_32 .. muliple-accumulate which multiplies two fields with 16bit
                    values and adds them in-place to a given 32bit value field
   * mac_16_32_32 .. multiply-accumulate which multiplies two fields with 8bit
                    values, then adds a field with 16bit values and saves the
                    result in a dedicated 32bit result field
   * png_filters .. png filter types: up, sub, avg, path for RGB and RGBA


## Configuration, Build & Install

### Configuration
```
./configure
```
Determines if the used toolchain supports RISC-V, RVV, which version/draft of
RVV and sets install prefix.

Build is also possible, if RISC-V or RVV is not supported by the used
toolchain, but implementations depending on them are excluded in this case.


### Build
```
make
```

Variable: "debug" (Examples below)
 * debug=1 .. no optimization (except algorithm implementations, debug
              symbols, unstripped install
 * debug=0 .. optimization, no debug symbols, stripped install


### Install
Install to configured prefix (default="/usr/local/bin")
```
make install
```


### Clean
Clean all build artefacts
```
make clean
```

Clean all build artefacts and configuration
```
make distclean
```


### Examples

Configure build and install to /usr/local/bin
```
./configure
make
make install
```

Configure, build and install to /opt/rvvradar/bin
```
./configure --prefix=/opt/rvvradar install
make install
```

Build for debug (unstripped and with debug symbols)
```
make clean
make debug=1
```


## Usage

### Online Help

```
RVVRadar -a 0x1 -s 256 -i 1 -v
```

```
RVVRadar-0.8 (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
RISC-V support is disabled

Usage: ./RVVRadar options

Options:
  --algs_enabled|-a <algs_mask>
     Bitmask of algorithms to run (hexadecimal).
       bit             algorithm
         0             memcpy
         1             mac_16_32_32 (32bit += 16bit * 16bit)
         2             mac_8_16_32  (32bit = 16bit + 8bit * 8bit)
         3             png_filter_up3
         4             png_filter_up4
         5             png_filter_sub3
         6             png_filter_sub4
         7             png_filter_avg3
         8             png_filter_avg4
         9             png_filter_paeth3
        10             png_filter_paeth4

  --iterations|-i <#iterations>
     Number of iterations to run each algorithm implementation.

  --len_start|-s <#elements>
     Initial number of elements to run algorithm implementations
     with.
     (Will be doubled until len_end is reached).

  [--len_end|-e <#elements>]
     Final number of elements to run algorithm implementations
     with.
     (len_start will be doubled until len_end is reached).
     (Default: value given with --len_start)

  [--randseed|-r <seed>]
     Set random seed for test data.
     (Default: 0)

  [--verify|-v]
     Enable verification of algorithm implementation results.
     Enable this, if you want to make sure, that calculations
     are correct (e.g. while development, functional tests, ...).
     Leave it disabled if you want to execute with as little
     interference (caches, ...) as possible.
     (Default: false)

  [--quiet|-q]
     Prevent human readable output (progress and statistics).
     (Default: false)

  [--help|-h]
     Print this help.


Output:
  stdout: results as comma separated values (csv).
          (including column headers)
  stderr: human readable output (supressed if quiet was set)
          and errors (independent of quiet).
```

### Examples

#### Run and Verify all Implementations of ''memcpy'' with a single run on 256 bytes
```
RVVRadar -a 0x1 -s 256 -i 1 -v
```

Human readable output on stderr:

```
RVVRadar-0.8 (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
RISC-V support is enabled
RISC-V RVV support is enabled (v0.7)

 + parameters:
   + randseed:       0
   + verify:         true
   + iterations:     1
   + len_start:      256
   + len_end:        256
   + algs_enabled: 0x1
 + set: RVVRadar
   + algorithm: memcpy(len=256)
     + implementation: c byte noavect
       + runs:  1
       + fails: 0
       + timing:
         + nmeasure:    1
         + min [ns]:    4792
         + max [ns]:    4792
         + mean [ns]:   4792
         + var [ns]:    0
         + stdev [ns]:  0
         + median [ns]: 4792
         + hist[000]:       1 [4792, 4792]
         + hist[001]:       0 [4793, 4793]
         + hist[002]:       0 [4794, 4794]
         + hist[003]:       0 [4795, 4795]
         + hist[004]:       0 [4796, 4796]
         + hist[005]:       0 [4797, 4797]
         + hist[006]:       0 [4798, 4798]
         + hist[007]:       0 [4799, 4799]
         + hist[008]:       0 [4800, 4800]
         + hist[009]:       0 [4801, 4801]
         + hist[010]:       0 [4802, 4802]
         + hist[011]:       0 [4803, 4803]
         + hist[012]:       0 [4804, 4804]
         + hist[013]:       0 [4805, 4805]
         + hist[014]:       0 [4806, 4806]
         + hist[015]:       0 [4807, 4807]
         + hist[016]:       0 [4808, 4808]
         + hist[017]:       0 [4809, 4809]
         + hist[018]:       0 [4810, 4810]
         + hist[019]:       0 [4811, 4811]
...
```

Machine interpretable output on stdout:
```
set;algorithm(parameters);implementation;runs;fails;nmeasure;tdmin [ns];tdmax [ns];tdmean [ns];tdvar [ns];tdstdev [ns];tdmedian [ns];nbuckets;hist_bucket[0];hist_bucket[1];hist_bucket[2];hist_bucket[3];hist_bucket[4];hist_bucket[5];hist_bucket[6];hist_bucket[7];hist_bucket[8];hist_bucket[9];hist_bucket[10];hist_bucket[11];hist_bucket[12];hist_bucket[13];hist_bucket[14];hist_bucket[15];hist_bucket[16];hist_bucket[17];hist_bucket[18];hist_bucket[19]
RVVRadar;memcpy(len=256);c byte noavect;1;0;1;5459;5459;5459;0;0;5459;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
RVVRadar;memcpy(len=256);4 int regs;1;0;1;4625;4625;4625;0;0;4625;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
RVVRadar;memcpy(len=256);c byte avect;1;0;1;4541;4541;4541;0;0;4541;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
RVVRadar;memcpy(len=256);system;1;0;1;5208;5208;5208;0;0;5208;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
RVVRadar;memcpy(len=256);rvv 32bit elements (no grouping);1;0;1;4292;4292;4292;0;0;4292;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
RVVRadar;memcpy(len=256);rvv 8bit elements (no grouping);1;0;1;4625;4625;4625;0;0;4625;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
RVVRadar;memcpy(len=256);rvv 8bit elements (group two);1;0;1;4375;4375;4375;0;0;4375;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
RVVRadar;memcpy(len=256);rvv 8bit elements (group four);1;0;1;3125;3125;3125;0;0;3125;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
RVVRadar;memcpy(len=256);rvv 8bit elements (group eight);1;0;1;4334;4334;4334;0;0;4334;20;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0
```

Check for fails in human readable or machine interpretable output.


#### Measure 100 Iterations of all PNG Filter Algorithms (RGB and RGBA) on 50KPixels per Run
```
RVVRadar -a 0x7f8 -s 50000 -i 100 -q > result.csv
```

Machine interpretable output on stdout (result.csv):
```
set;algorithm(parameters);implementation;runs;fails;nmeasure;tdmin [ns];tdmax [ns];tdmean [ns];tdvar [ns];tdstdev [ns];tdmedian [ns];nbuckets;hist_bucket[0];hist_bucket[1];hist_bucket[2];hist_bucket[3];hist_bucket[4];hist_bucket[5];hist_bucket[6];hist_bucket[7];hist_bucket[8];hist_bucket[9];hist_bucket[10];hist_bucket[11];hist_bucket[12];hist_bucket[13];hist_bucket[14];hist_bucket[15];hist_bucket[16];hist_bucket[17];hist_bucket[18];hist_bucket[19]
RVVRadar;png_filters_up3(len=50000,rowbytes=150000);c byte noavect;100;0;100;1089050;1175635;1101334;471664707;21717;1091675;20;68;14;1;0;0;0;0;0;0;0;2;3;4;1;3;1;1;1;0;1
RVVRadar;png_filters_up3(len=50000,rowbytes=150000);c byte avect;100;0;100;277586;347252;284240;103589376;10177;281606;20;47;26;13;9;1;0;0;0;0;0;0;0;1;1;1;0;0;0;0;1
RVVRadar;png_filters_up3(len=50000,rowbytes=150000);rvv_m1;100;0;100;220252;893757;231778;4520455751;67234;223439;20;96;2;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_up3(len=50000,rowbytes=150000);rvv_m2;100;0;100;182293;265835;193941;70971040;8424;193022;20;1;8;82;7;0;0;0;0;0;0;0;1;0;0;0;0;0;0;0;1
RVVRadar;png_filters_up3(len=50000,rowbytes=150000);rvv_m4;100;0;100;182377;255502;190084;86432435;9296;188793;20;4;68;23;3;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;2
RVVRadar;png_filters_up3(len=50000,rowbytes=150000);rvv_m8;100;0;100;176085;225169;188658;19912799;4462;187918;20;2;0;0;0;60;29;3;4;1;0;0;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_up4(len=50000,rowbytes=200000);c byte noavect;100;0;100;1456637;1575096;1472604;778241810;27896;1460011;20;77;3;0;0;0;0;0;2;8;1;1;0;3;0;1;0;1;0;2;1
RVVRadar;png_filters_up4(len=50000,rowbytes=200000);c byte avect;100;0;100;367294;429837;380315;111419035;10555;379211;20;9;11;19;15;18;18;6;0;0;0;0;0;0;0;0;0;0;1;2;1
RVVRadar;png_filters_up4(len=50000,rowbytes=200000);rvv_m1;100;0;100;290002;981716;303213;4927144327;70193;293689;20;96;2;0;0;1;0;0;0;0;0;0;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_up4(len=50000,rowbytes=200000);rvv_m2;100;0;100;248293;335086;261009;119138749;10915;258668;20;1;2;83;10;0;0;0;0;0;1;1;0;0;0;0;0;1;0;0;1
RVVRadar;png_filters_up4(len=50000,rowbytes=200000);rvv_m4;100;0;100;251418;443171;259975;597905752;24452;254565;20;94;0;0;2;0;0;0;1;1;0;0;0;1;0;0;0;0;0;0;1
RVVRadar;png_filters_up4(len=50000,rowbytes=200000);rvv_m8;100;0;100;244919;442087;261045;647786856;25451;254794;20;50;43;0;0;1;0;1;2;0;1;0;0;1;0;0;0;0;0;0;1
RVVRadar;png_filters_sub3(len=50000,rowbytes=150000);c byte noavect;100;0;100;1217634;1314885;1231545;606366049;24624;1219239;20;78;0;0;0;0;0;0;0;3;6;3;0;1;1;4;3;0;0;0;1
RVVRadar;png_filters_sub3(len=50000,rowbytes=150000);c byte avect;100;0;100;1218052;1507262;1234214;1389175082;37271;1219260;20;78;0;3;11;4;1;0;2;0;0;0;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_sub3(len=50000,rowbytes=150000);rvv_dload;100;0;100;860049;1165968;875447;1272039392;35665;865964;20;83;0;2;9;3;2;0;0;0;0;0;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_sub3(len=50000,rowbytes=150000);rvv_reuse;100;0;100;560380;752297;567579;574538679;23969;562046;20;93;0;0;0;1;3;0;1;0;0;1;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_sub4(len=50000,rowbytes=200000);c byte noavect;100;0;100;1622929;1786889;1634840;682920292;26132;1624033;20;82;0;0;0;0;11;1;1;2;1;0;1;0;0;0;0;0;0;0;1
RVVRadar;png_filters_sub4(len=50000,rowbytes=200000);c byte avect;100;0;100;1622805;2051142;1645079;2501601924;50016;1624325;20;71;2;17;6;1;2;0;0;0;0;0;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_sub4(len=50000,rowbytes=200000);rvv_dload;100;0;100;829298;1015258;843744;964657408;31058;830923;20;82;0;0;0;1;7;0;3;3;1;1;0;1;0;0;0;0;0;0;1
RVVRadar;png_filters_sub4(len=50000,rowbytes=200000);rvv_reuse;100;0;100;533296;609338;537800;179208032;13386;534504;20;92;2;0;0;0;0;0;0;0;0;0;1;3;0;0;0;0;0;0;2
RVVRadar;png_filters_avg3(len=50000,rowbytes=150000);c byte noavect;100;0;100;1808264;1908765;1825920;630210135;25103;1812160;20;66;7;0;0;0;0;0;0;2;10;6;0;3;3;1;0;0;1;0;1
RVVRadar;png_filters_avg3(len=50000,rowbytes=150000);c byte avect;100;0;100;1808931;2062308;1834548;1949746259;44155;1812785;20;71;0;0;11;4;3;4;2;0;3;0;0;0;0;0;0;0;0;1;1
RVVRadar;png_filters_avg3(len=50000,rowbytes=150000);rvv;100;0;100;1212092;1372844;1229197;911416000;30189;1214384;20;76;0;0;0;0;7;7;3;1;2;1;0;0;1;1;0;0;0;0;1
RVVRadar;png_filters_avg4(len=50000,rowbytes=200000);c byte noavect;100;0;100;2239601;2354810;2261564;985377515;31390;2243018;20;64;5;0;0;0;0;2;0;12;2;2;2;4;0;1;1;2;0;1;2
RVVRadar;png_filters_avg4(len=50000,rowbytes=200000);c byte avect;100;0;100;2239392;13866527;3329522;6926813505311;2631884;2244851;20;82;2;0;2;0;1;1;1;0;1;2;0;1;2;2;0;1;1;0;1
RVVRadar;png_filters_avg4(len=50000,rowbytes=200000);rvv;100;0;100;1073633;7577351;1243136;790025566654;888833;1076863;20;95;2;0;0;0;0;1;0;0;0;0;0;0;0;0;0;0;0;1;1
RVVRadar;png_filters_paeth3(len=50000,rowbytes=150000);c byte noavect;100;0;100;4459618;4769287;4516438;2285605899;47808;4515432;20;36;0;1;23;8;13;9;4;3;1;0;1;0;0;0;0;0;0;0;1
RVVRadar;png_filters_paeth3(len=50000,rowbytes=150000);c byte avect;100;0;100;4460119;4716203;4511993;2407153689;49062;4515056;20;39;2;0;5;17;8;11;6;4;3;3;0;0;0;0;0;0;0;0;2
RVVRadar;png_filters_paeth3(len=50000,rowbytes=150000);rvv bulk load;100;0;100;4283242;5329917;4344089;13546723327;116390;4331638;20;64;25;7;1;0;1;0;0;1;0;0;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_paeth3(len=50000,rowbytes=150000);rvv;100;0;100;3901489;4315450;3943560;3679225729;60656;3943989;20;48;1;26;9;6;8;0;0;0;0;0;0;0;0;0;0;1;0;0;1
RVVRadar;png_filters_paeth4(len=50000,rowbytes=200000);c byte noavect;100;0;100;5772504;6056757;5830811;3070262953;55409;5829588;20;40;0;3;7;6;9;9;9;15;0;0;0;0;0;0;0;0;0;1;1
RVVRadar;png_filters_paeth4(len=50000,rowbytes=200000);c byte avect;100;0;100;5776379;6071882;5835689;3376725106;58109;5846879;20;41;0;0;6;8;15;12;10;5;0;0;0;0;0;0;0;1;0;1;1
RVVRadar;png_filters_paeth4(len=50000,rowbytes=200000);rvv bulk load;100;0;100;4222908;4599786;4266418;2397696746;48966;4271909;20;42;0;28;12;9;5;3;0;0;0;0;0;0;0;0;0;0;0;0;1
RVVRadar;png_filters_paeth4(len=50000,rowbytes=200000);rvv;100;0;100;3808863;4194867;3850047;2611940602;51107;3849925;20;49;0;23;10;10;3;4;0;0;0;0;0;0;0;0;0;0;0;0;1
```


### Interpreting the Results
RVVRadar provides extensive statistics for each algorithm implementation,
which allows for detailed analysis:
 * the minimum/maximum run-time
 * the arithmetic mean run-time, including variance and standard deviation
 * the median run-time
 * and a run-time histogram with 20 buckets between min and max run-time

For a simple performance evaluation the average run-times are the most
interesting values. RVVRadar provides two averaging methods for its run-time
measurements, namely the arithmetic mean and the median. Since GNU/Linux is
not a hard real-time system and thus not deterministic there are run-time
spikes. For this reason, it is recommended to use the median, as it is more
robust against such outliers.

The csv can easy be processed with tools like *csvtool* or using python.


#### Example: Show Median Runtimes from Example above using *csvtool*

```
$ csvtool -t ';' col 1,2,3,12 result.csv
set,algorithm(parameters),implementation,tdmedian [ns]
RVVRadar,"png_filters_up3(len=50000,rowbytes=150000)",c byte noavect,1091675
RVVRadar,"png_filters_up3(len=50000,rowbytes=150000)",c byte avect,281606
RVVRadar,"png_filters_up3(len=50000,rowbytes=150000)",rvv_m1,223439
RVVRadar,"png_filters_up3(len=50000,rowbytes=150000)",rvv_m2,193022
RVVRadar,"png_filters_up3(len=50000,rowbytes=150000)",rvv_m4,188793
RVVRadar,"png_filters_up3(len=50000,rowbytes=150000)",rvv_m8,187918
RVVRadar,"png_filters_up4(len=50000,rowbytes=200000)",c byte noavect,1460011
RVVRadar,"png_filters_up4(len=50000,rowbytes=200000)",c byte avect,379211
RVVRadar,"png_filters_up4(len=50000,rowbytes=200000)",rvv_m1,293689
RVVRadar,"png_filters_up4(len=50000,rowbytes=200000)",rvv_m2,258668
RVVRadar,"png_filters_up4(len=50000,rowbytes=200000)",rvv_m4,254565
RVVRadar,"png_filters_up4(len=50000,rowbytes=200000)",rvv_m8,254794
RVVRadar,"png_filters_sub3(len=50000,rowbytes=150000)",c byte noavect,1219239
RVVRadar,"png_filters_sub3(len=50000,rowbytes=150000)",c byte avect,1219260
RVVRadar,"png_filters_sub3(len=50000,rowbytes=150000)",rvv_dload,865964
RVVRadar,"png_filters_sub3(len=50000,rowbytes=150000)",rvv_reuse,562046
RVVRadar,"png_filters_sub4(len=50000,rowbytes=200000)",c byte noavect,1624033
RVVRadar,"png_filters_sub4(len=50000,rowbytes=200000)",c byte avect,1624325
RVVRadar,"png_filters_sub4(len=50000,rowbytes=200000)",rvv_dload,830923
RVVRadar,"png_filters_sub4(len=50000,rowbytes=200000)",rvv_reuse,534504
RVVRadar,"png_filters_avg3(len=50000,rowbytes=150000)",c byte noavect,1812160
RVVRadar,"png_filters_avg3(len=50000,rowbytes=150000)",c byte avect,1812785
RVVRadar,"png_filters_avg3(len=50000,rowbytes=150000)",rvv,1214384
RVVRadar,"png_filters_avg4(len=50000,rowbytes=200000)",c byte noavect,2243018
RVVRadar,"png_filters_avg4(len=50000,rowbytes=200000)",c byte avect,2244851
RVVRadar,"png_filters_avg4(len=50000,rowbytes=200000)",rvv,1076863
RVVRadar,"png_filters_paeth3(len=50000,rowbytes=150000)",c byte noavect,4515432
RVVRadar,"png_filters_paeth3(len=50000,rowbytes=150000)",c byte avect,4515056
RVVRadar,"png_filters_paeth3(len=50000,rowbytes=150000)",rvv bulk load,4331638
RVVRadar,"png_filters_paeth3(len=50000,rowbytes=150000)",rvv,3943989
RVVRadar,"png_filters_paeth4(len=50000,rowbytes=200000)",c byte noavect,5829588
RVVRadar,"png_filters_paeth4(len=50000,rowbytes=200000)",c byte avect,5846879
RVVRadar,"png_filters_paeth4(len=50000,rowbytes=200000)",rvv bulk load,4271909
RVVRadar,"png_filters_paeth4(len=50000,rowbytes=200000)",rvv,3849925
```

### Adding a New Algorithm

To add a new algorithm following steps must be performed:

 1. Using *memcpy* as template for *newalg*
    1. Change to directory *algorithms*
    1. Copy *memcpy* to *newalg* (name of the new algorithm to add)
 1. Adapt *newalg*
    1. Change to directory *algorithms/newalg*
    1. *alg.c* / *alg.h*
       1. Replace occurrences of *memcpy* with *newalg* (check case)
       1. Adapt input/output data buffers according to input/output parameter
          needs of the algorithm
       1. Disable all implementations except the two starting "c" in
          *impls_add*
    1. *impl_c.in.c* (baseline implementation)
       1. Replace occurrences of *memcpy* with *newalg*
       1. Adapt implementation to function of algorithm
         * **Important Notes**:
           * This implementation will be used for verification and as baseline
             for further comparisons
           * The buildsystem creates two functions from this code. Both are
             compiled with GCCs *-O3*, but one with and one without using GCCs
             auto-vectorizer
    1. *impl_rv.c*, *impl_rvv.c*
       1. Comment-out or delete content
 1. Add *newalg* to *RVVRadar.c*
    1. Include *algorithms/newalg/alg.h*
    1. Add and handle the new algorithm id *ALG_ID_PNG_FILTER_NEWALG*

After these steps *newalg* is integrated in RVVRadar and it can be built
by *make* (The build system handles *newalg* automatically).

The execution of RVVRadar already provides two results for *newalg*. Namely
for the baseline implementation compiled without and with using GCC's
auto-vectorizer.

After integration of *newalg* the iterative development process can begin.
Just add new implementations (e.g. in *impl_rvv.c*) and enable them in *alg.c*
(*impls_add*). The evaluation can then be performed using the automatically
generated results from RVVRadar.
