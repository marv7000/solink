#include <stdio.h>
#include <stdlib.h>
//! This header is Unix only, swap this for a portable function later!
#include <libgen.h>

#include <args.h>
#include <elf.h>
#include <patch.h>
#include <log.h>

i32 main(i32 argc, str* argv)
{
    // Parse arguments.
    args_parse(argc, argv);

    // Open all libraries.
    elf_obj* libs = calloc(ARGS.num_files, sizeof(elf_obj));
    for (u16 i = 0; i < ARGS.num_files; i++)
        libs[i] = elf_read(ARGS.files[i]);

    const u16 num_libs = ARGS.num_files - 1;
    elf_obj* target = &libs[num_libs];

    // Print matching library symbols.
    log_msg(LOG_INFO, "linking dynamic symbols to \"%s\"", basename(ARGS.files[num_libs]));

    // Get all symbol names from the target.
    str* symbols = NULL;
    size num_sym = patch_get_symbols(target, &symbols);

    // Try to resolve the symbols.
    for (size sym = 0; sym < num_sym; sym++)
    {
        // Find which library provides this symbol.
        i32 found_in = -1;
        for (u16 lib = 0; lib < num_libs; lib++)
        {
            if (patch_find_sym(libs + lib, symbols[sym]))
            {
                found_in = lib;
                break;
            }
        }

        if (found_in >= 0)
            log_msg(LOG_INFO, "[x] %s\t-> %s", symbols[sym], basename(ARGS.files[found_in]));
        else
            log_msg(LOG_INFO, "[ ] %s", symbols[sym]);
    }

    // Patch input executable with all libraries.
    for (u16 i = 0; i < num_libs; i++)
    {
        if (!patch_link_library(target, &libs[i]))
            log_msg(LOG_ERR, "failed to link against \"%s\"!", basename(ARGS.files[i]));
    }

    // Write the result to file.
    elf_write(ARGS.output, target);
    log_msg(LOG_INFO, "wrote the patched binary to \"%s\".", ARGS.output);

    free(libs);
    return 0;
}
