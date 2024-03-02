#include <stdio.h>
#include <stdlib.h>

#include <patch.h>

bool patch_get_dynsym(const elf_file* elf, uint16_t* dynsym, uint16_t* dynstr)
{
    if (!elf_find_section(".dynsym", elf, dynsym) || !elf_find_section(".dynstr", elf, dynstr))
    {
        fprintf(stderr, "Error: Couldn't find dynamic symbol table!\n");
        return false;
    }
    return true;
}

bool patch_get_symbols(const elf_file* elf, char*** str, uint64_t* num)
{
    if (!elf || !str || !num)
        exit(1);

    // Get the dynamic symbol table.
    uint16_t lib_sym, lib_str;
    if (!patch_get_dynsym(elf, &lib_sym, &lib_str))
        return false;

    uint64_t num_sym = elf->section_header[lib_sym].sh_size / elf->section_header[lib_sym].sh_entsize;

    // Write all entries to the buffer.
    *str = (char**)malloc(num_sym * sizeof(char*));
    uint64_t written = 0;
    for (uint64_t i = 1; i < num_sym; i++)
    {
        elf_symtab* sym = ((elf_symtab*)elf->section_data[lib_sym]) + i;
        // Only match symbols with info == STB_GLOBAL | STB_FUNC
        if (sym->sym_info == 0x12)
        {
            (*str)[written] = (char*)(elf->section_data[lib_str] + sym->sym_name);
            written++;
        }
    }
    *num = written;
    return true;
}

bool patch_link_library(elf_file* target, const elf_file* library)
{
    char** names;
    uint64_t num_names;
    patch_get_symbols(target, &names, &num_names);
    for (uint64_t sym = 0; sym < num_names; sym++)
    {
        // Deliberately ignoring result, as not all symbols might be used.
        patch_link_symbol(target, library, names[sym]);
    }
    return true;
}

bool patch_link_symbol(elf_file* target, const elf_file* library, const char* name)
{
    if (!target || !library || !name)
        return false;

    // TODO

    return true;
}
