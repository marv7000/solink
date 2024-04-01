#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
    {
        libs[i] = elf_read(ARGS.files[i]);
    }

    const u16 last = ARGS.num_files - 1;
    elf_obj* target = libs + last;

    // Print library symbols.
    str* names = NULL;
    size num_names = 0;
    patch_get_symbols(target, &names, &num_names);
    log_msg(LOG_INFO, "linking symbol(s):");
    for (size sym = 0; sym < num_names; sym++)
    {
        log_msg(LOG_INFO, "[%c]\t%s", patch_find_sym(target, names[sym]) ? 'x' : ' ', names[sym]);
    }

    // Patch input executable with all libraries.
    for (u16 i = 0; i < last; i++)
    {
        if (!patch_link_library(target, &libs[i]))
        {
            log_msg(LOG_ERR, "failed to link against \"%s\"!", ARGS.files[i]);
        }
    }

    // Write the result to file.
    elf_write(ARGS.output, target);
    log_msg(LOG_INFO, "wrote the patched binary to \"%s\".", ARGS.output);

    free(libs);
    return 0;
}
