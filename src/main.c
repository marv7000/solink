#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <args.h>
#include <elf.h>
#include <patch.h>

int32_t main(const int32_t argc, const char** argv)
{
    // Parse arguments.
    arguments args;
    args_parse(&args, argc, argv);

    elf_file file;
    if (elf_read(argv[argc - 1], &file) != ELF_OK)
        return 1;

    // Get 
    uint16_t idx;
    if (!elf_find_section(".dynstr", &file, &idx))
    {
        fprintf(stderr, "Error: Couldn't find dynamic symbol table!\n");
        return 1;
    }

    if (elf_write(args.output, &file) != ELF_OK)
        return 1;

    return 0;
}
