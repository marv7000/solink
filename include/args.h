#pragma once
#include <stdint.h>
#include <stdbool.h>

#include <str.h>

typedef struct
{
    uint16_t num_files;
    str* files;
    str output;
    uint32_t num_symbols;
    str* symbols;
    bool force;
    bool quiet;
    bool version;
    bool help;
} arguments;


/// \brief                  Parses command line arguments into an argument struct.
/// \param  [out]   args    The parsed arguments.
/// \param          argc    The amount of arguments in `argv`.
/// \param  [in]    argv    The argument vector.
void args_parse(arguments* args, int32_t argc, str* argv);

/// \brief                  Checks if the path is a valid file.
/// \param  [in]    path    The path to the file.
/// \returns                True if valid, false otherwise.
bool args_check_file(str path);
