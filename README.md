# solink
A linker for shared object files

### About
`solink` reads ELF exectuables and attempts to satisfy external references by
reading provided shared objects and patching the executable.
A common use case for this tool is for users without access to the 
source code of an application needing a portable executable without external
references.

> **_Warning:_** 
> It's highly discouraged to link against system libraries (such as `glibc`).
> While this works, it can have unwanted side effects or cause programs to 
> crash.

### Usage
```
solink <Flag(s)> [Path to shared object(s)] [Path to executable]
```
All flags are optional, but there has to be at least one shared object.
The last argument is always the executable to be linked to.
If no arguments are provided, `solink` will output a help text (equivalent to
`solink --help`).

Example:
```sh
solink -o my_patched_app libs/my_lib1.so libs/my_lib2.so my_app
```

### Flags
For a list of flags and options, see the [documentation](docs/flags.md).

### Building
```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
