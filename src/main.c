#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    log_msg(LOG_INFO, "linking symbols of \"%s\" like this:\n\n", basename(ARGS.files[num_libs]));

    // Get all symbol names from the target.
    str* symbols = NULL; // List of symbol names.
    size num_sym = patch_get_symbols(target, &symbols); // Amount of symbol names.
    i32* sym_idx = calloc(num_sym, sizeof(i32)); // List of indices into all ELFs, where each symbol is located.
    size strs_len = 0; // Size of the longest symbol string, for nice table formatting.

    // Try to resolve the symbols.
    for (size sym = 0; sym < num_sym; sym++)
    {
        // Find which library provides this symbol.
        sym_idx[sym] = -1;
        for (u16 lib = 0; lib < num_libs; lib++)
        {
            if (patch_find_sym(libs + lib, symbols[sym]))
            {
                sym_idx[sym] = lib;
                break;
            }
        }
        // FIXME: There's probably a nicer way to do this.
        if (strlen(symbols[sym]) > strs_len)
            strs_len = strlen(symbols[sym]);
    }

    // Print table header.
    log_msg(LOG_INFO, "link\tname");
    for (size i = 0; i < strs_len - 4; i++) // Pad len - "name"
        log_msg(LOG_INFO, " ");
    log_msg(LOG_INFO, "\tsource\n");

    for (size sym = 0; sym < num_sym; sym++)
    {
        // Padding string for nice formatting.
        const size pad_len = strs_len - strlen(symbols[sym]);
        char pad_str[256];
        memset(pad_str, ' ', pad_len);
        pad_str[pad_len] = '\0';

        if (sym_idx[sym] >= 0)
            log_msg(LOG_INFO, "[x]\t%s%s\t%s\n", symbols[sym], pad_str, basename(ARGS.files[sym_idx[sym]]));
        else
            log_msg(LOG_INFO, "[ ]\t%s%s\t-\n", symbols[sym], pad_str);
    }

    // TODO: Finish linking code
    /*
    // Patch input executable with all libraries.
    for (u16 i = 0; i < num_libs; i++)
    {
        if (!patch_link_library(target, &libs[i]))
            log_msg(LOG_ERR, "failed to link against \"%s\"!", basename(ARGS.files[i]));
    }
    */

    // Write the result to file.
    elf_write(ARGS.output, target);
    log_msg(LOG_INFO, "wrote the patched binary to \"%s\"\n", ARGS.output);

    free(libs);
    return 0;
}
