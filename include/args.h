#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    const char* output;
    uint32_t num_symbols;
    const char** symbols;
    bool force;
    bool verbose;
    bool help;
} arguments;


/// \brief                  Parses command line arguments into an argument struct.
/// \param  [out]   args    The parsed arguments.
/// \param          argc    The amount of arguments in `argv`.
/// \param  [in]    argv    The argument vector.
void args_parse(arguments* args, int32_t argc, const char** argv);

/// \brief      Checks if the path is a valid file.
/// \param path The path to the file.
/// \return     True if valid, false otherwise.
bool args_check_file(const char* path);
