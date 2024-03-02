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

    // Print library symbols.
    if (!args.quiet)
    {
        char*** names = (char***)malloc(args.num_files * sizeof(char**));
        uint64_t* num_names = (uint64_t*)malloc(args.num_files * sizeof(uint64_t));
        for (uint64_t i = 0; i < args.num_files; i++)
        {
            printf("Dynamic symbols provided by \"%s\":\n", args.files[i]);
            patch_get_symbols(libs + i, names + i, num_names + i);
            for (uint64_t sym = 0; sym < num_names[i]; sym++)
                printf("\t%s\n", names[i][sym]);
        }
        printf("Matching symbols:\n");
        // For every executable symbol.
        for (uint64_t exe = 0; exe < num_names[args.num_files - 1]; exe++)
        {
            // For each library.
            for (uint64_t lib = 0; lib < args.num_files - 1; lib++)
            {
                // For each library symbol.
                for (uint64_t sym = 0; sym < num_names[lib]; sym++)
                {
                    if (!strcmp(names[lib][sym], names[args.num_files - 1][exe]))
                        printf("\t%s\n", names[lib][sym]);
                }
            }
        }
        free(names);
        free(num_names);
    }

    // Create the output ELF.
    elf_file target;
    elf_new(&target);

    // Patch input executable with all libraries.
    for (uint16_t i = 0; i < args.num_files - 1; i++)
    {
        patch_link_library(libs + args.num_files - 1, libs + i);
    }

    // Write the result to file.
    elf_error written = elf_write(args.output, &target);
    if (written != ELF_OK)
    {
        fprintf(stderr, "Error: Failed to write the ELF to \"%s\"!\n", args.output);
        fprintf(stderr, "       %s\n", elf_error_str(written));
        return 1;
    }

    return 0;
}
