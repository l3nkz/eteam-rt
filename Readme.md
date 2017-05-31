# E-Team Energy Measurement Runtime and Library

This repository contains libraries and other runtime binaries designed to ease the usage of the
[E-Team][eteam] Linux scheduler.

## Installation

To install the library with its headers and the runtime binaries to your system use the following commands:

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

This command sequence will make on out-off-tree build of the library and runtime and install them to your system.
By default, the library will be installed to /usr/local/lib, the headers to /usr/local/include and the runtime
binaries to /usr/local/bin. If you don't want to install them to your system, you can just simply skip the
last command. The fully built library will then be available in the 'lib' folder in the source directory and the
runtime binary in the 'bin' folder.

## Usage

At the moment, this repository contains the following items:

+ a library - `libeteam` - to programatically start and stop energy measurements for programs via E-Team as well as to get the
  current energy consumption of a program.
+ a runtime program `energy` that can be used to measure the energy consumption of arbitrary other programs in the same
  fashion as the `time` program can be used to measure the runtime of an application.

### libeteam

The functionality provided by the E-Team library can be accessed by using the following include directive in your program

```c
#include <eteam.h>
```

The library then exports the following data structures

```c
struct energy {
    unsigned long package;      /* Energy consumed by the program for the whole package (uJ) */
    unsigned long core;         /* Energy consumed by the program for all CPU cores (uJ) */
    unsigned long dram;         /* Energy consumed by the program for the DRAM (uJ) */
    unsigned long gpu;          /* Energy consumed by the program for the internal GPU (uJ) */
};
```

Furthermore, the following functions are exported by the library

```c
/* Start energy measuring for the process with the given PID */
int start_energy(pid_t pid);

/* Stop energy measuring for the process with given PID */
int stop_energy(pid_t pid);

/* Get the consumed energy for the process with the given PID */
int consumed_energy(pid_t pid, struct energy *energy);
```

By passing `0` as PID to any of the exported functions, they will always operate on the calling process. Hence, if one wants
to activate energy measurements for the currently running process one can use `start_energy(0)` instead of `start_energy(getpid())`.

### energy

The program `energy` can be used to get the energy consumption of any executable available on the system. Using the `energy` program
is as simple as using the `time` program to get runtime statistics for an executable. The following command shows an example usage
of the energy program to measure the energy consumption of `firefox`:

```bash
energy -- firefox
```

To be able to differentiate between measured applications and own command line options, `energy` requires that a '--' must be before
any definition of an application. This scheme allows the `energy` program to measure the energy consumption of multiple programs
in parallel and also makes passing additional options to the programs very easy. For example, to measure the energy consumption of a
firefox instance in parallel to the [memtier benchmark][memtierbench] one can use the following command:

```bash
energy -- firefox --profile foobar --jsconsole -- memtier_bench --request=10000 --data-size=1024
```

Without any additional options, the program will run the given executable once with E-Team and at the end display the consumed energy
and various other statistics as a csv. This behavior can be changed with via the various options that the program supports. For example
to measure the memtier benchmark for 20 runs with output redirected to the file `memtier_run` one can use the following command:

```bash
energy --repeat=20 --redirect=memtier_run -- memtier_bench --requests=10000 --data-size=1024
```

To see all the possible configuration knobs of the `energy` program use `energy --help`.

#### Measurement Methods

To be able to make comparison measurements or use the `energy` program for various different jobs, the program also supports 3 different
ways to determine the energy consumption of a program:

+ **E-Team** (default): uses the E-Team Linux scheduler to make the energy measurements while still allowing processor time-multiplexing.
  (`energy -- ? firefox` or `energy -- firefox`)
+ **MSR end-to-end** (requires root privileges): uses the RAPL MSRs to make an end-to-end measurement of the program while using CFS
  for scheduling. This method is not capable of accurately measuring the energy consumption of a program, if there are other applications
  running in parallel in the system. (`energy -- - firefox`)
+ **None**: don't make any energy measurements at all. (`energy -- ! firefox`)


[eteam]: https://dummy.com "E-Team scheduler for the Linux kernel"
[memtierbench]: https://github.com/RedisLabs/memtier_benchmark "NoSQL Redis and Memcache traffic generation and benchmarking tool"
