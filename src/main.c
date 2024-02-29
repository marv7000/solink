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
    arguments args = {0};
    args_parse(&args, argc, argv);

    // Open the input ELF.
    elf_file elf_input;
    if (elf_read(args.input, &elf_input) != ELF_OK)
        return 1;

    // Get dynamic symbol table.
    uint16_t dynsym;
    uint16_t dynstr;
    if (!elf_find_section(".dynsym", &elf_input, &dynsym) || !elf_find_section(".dynstr", &elf_input, &dynstr))
    {
        fprintf(stderr, "Error: Couldn't find dynamic symbol table!\n");
        return 1;
    }

    // Print all dynamic symbol names.
    if (!args.quiet)
    {
        printf("Dynamic symbols required by \"%s\":\n", args.input);
        uint64_t num_entries = elf_input.sh[dynsym].sh_size / elf_input.sh[dynsym].sh_entsize;
        for (uint64_t i = 1; i < num_entries; i++)
        {
            elf_symtab* sym = ((elf_symtab*)elf_input.sb[dynsym]) + i;
            printf("%s\n", elf_input.sb[dynstr] + sym->sym_name);
        }
    }

    // TODO: Get library exports.

    // TODO: Patch input executable.

    // Write the result to file.
    if (elf_write(args.output, &elf_input) != ELF_OK)
    {
        fprintf(stderr, "Error: Failed to write the ELF to file!\n");
        return 1;
    }

    return 0;
}
