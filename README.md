# Pine
Pine is an interpreted language. It's syntax is similar to a high level assembly-like language, where each line is a single instruction.

## Build
Environment:  
* tested on linux
* c++ 14 compiler
* cmake

Libraries:  
* my [parg](https://github.com/octobanana/parg) library, for parsing cli args, included as `./src/parg.hh`
* [fmt](https://github.com/fmtlib/fmt) library, included as `./src/format.h` and `./src/format.cc`

The following shell commands will build the project:  
```bash
git clone <repo_name>
cd <repo_name>
./build.sh -r
```
To build the debug version, run the build script without the -r flag.  

## Install
The following shell commands will install the project:  
```bash
./install.sh -r
```

## Instructions
The following are the currently implemented instructions:  

### mov
### clear
### add
### sub
### mlt
### div
### mod
### lbl
### cmp
### jmp
### jeq
### jne
### jlt
### jgt
### jge
### jle
### pop
### psh
### prt
### ask
### ifl
### ofl
### run
### ret
### dbg
### slp
### ext

## Examples
There are several examples in the `./examples` directory.
