
# MegaOS

My hobby OS that is very ironic

## Authors

[rocketLover11](https://github.com/rocketLover11)

## Build Instructions

### Requirements

 - [Git](https://git-scm.com)
 - [x86_64-elf-gcc](https://formulae.brew.sh/formula/x86_64-elf-gcc)
 - [x86_64-elf-binutils](https://formulae.brew.sh/formula/x86_64-elf-binutils)
 - [NASM](https://www.nasm.us)
 - [CMake](https://cmake.org)
 - [GNU Make](https://ftp.gnu.org/old-gnu/Manuals/make-3.80/html_node/make.html)

### Windows

```batch
git clone https://github.com/rocketLover11/megaos.git
cd megaos
cd limine
make
cd ..
build.bat iso
```

### Unix-like (Linux/MacOS **NOT TESTED**)

```bash
git clone https://github.com/rocketLover11/megaos.git
cd megaos
cd limine
make
cd ..
./build.sh iso
```

## Roadmap

 - v0.2: Finish microkernel
 - v0.3: Setup user space

## About

License: MIT  
Version: v0.1.3
