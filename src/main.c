#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <args.h>
#include <elf.h>
#include <patch.h>

int32_t main(const int32_t argc, const char** argv)
{
    // Parse arguments.
    arguments args = {0};
    args_parse(&args, argc, argv);

    // Open all libraries.
    // TODO: Potentially unsafe, change to malloc.
    elf_file* libs = alloca(args.num_files * sizeof(elf_file));
    for (uint16_t i = 0; i < args.num_files; i++)
    {
        elf_error read = elf_read(args.files[i], libs + i);
        if (read != ELF_OK)
        {
            fprintf(stderr, "Error: Failed to read the ELF \"%s\" from file!\n", args.files[i]);
            fprintf(stderr, "       %s\n", elf_error_str(read));
            return 1;
        }
    }

    const uint64_t last = args.num_files - 1;

    // Print library symbols.
    if (!args.quiet)
    {
        char** names;
        uint64_t num_names;
        patch_match_symbols(libs + last, libs, last, &names, &num_names);
        printf("Linking %lu symbol%s:\n", num_names, num_names == 1 ? "" : "s");
        for (uint64_t sym = 0; sym < num_names; sym++)
            printf("[%lu]\t%s\n", sym, names[sym]);
    }

    // Create the output ELF.
    elf_file* target = libs + last;

    // Patch input executable with all libraries.
    for (uint16_t i = 0; i < last; i++)
    {
        patch_link_library(target, libs + i);
    }

    // Write the result to file.
    elf_error written = elf_write(args.output, target);
    if (written != ELF_OK)
    {
        fprintf(stderr, "Error: Failed to write the ELF to \"%s\"!\n", args.output);
        fprintf(stderr, "       %s\n", elf_error_str(written));
        return 1;
    }
    if (!args.quiet)
        printf("Wrote the patched binary to \"%s\".\n", args.output);

    return 0;
}
