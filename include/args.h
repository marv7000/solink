#pragma once
#include <types.h>

typedef struct
{
    u16 num_files;
    str* files;
    str output;
    u32 num_symbols;
    str* symbols;
    bool force;
    bool version;
    bool help;
} arguments;

extern arguments ARGS;

/// \brief                  Parses command line arguments into an argument struct.
/// \param          argc    The amount of arguments in `argv`.
/// \param  [in]    argv    The argument vector.
void args_parse(i32 argc, str* argv);
