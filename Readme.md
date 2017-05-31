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


[eteam]: https://dummy.com "E-Team scheduler for the Linux kernel"
