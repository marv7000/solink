#include <stdio.h>
#include <stdlib.h>

#include <patch.h>
#include <string.h>

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

bool patch_match_symbols(const elf_file* target, const elf_file* libs, uint64_t num_lib, char*** str, uint64_t* num_str)
{
    if (!target || !libs || num_lib == 0 || !str)
        return false;

    // Get all symbol names from the target.
    char** target_sym_names;
    uint64_t target_sym_num_names;
    patch_get_symbols(target, &target_sym_names, &target_sym_num_names);

    // Get all symbol names from libraries.
    char*** sym_names = (char***)malloc(target_sym_num_names * sizeof(char**));
    uint64_t* sym_num_names = (uint64_t*)malloc(target_sym_num_names * sizeof(uint64_t));;
    for (uint64_t i = 0; i < num_lib; i++)
        patch_get_symbols(libs + i, (&sym_names)[i], (&sym_num_names)[i]);

    // Allocate memory.
    *str = (char**)malloc(target_sym_num_names * sizeof(char*));
    uint64_t idx = 0;

    // For every executable symbol.
    for (uint64_t exe = 0; exe < target_sym_num_names; exe++)
    {
        // For each library.
        for (uint64_t lib = 0; lib < num_lib; lib++)
        {
            // For each library symbol.
            for (uint64_t sym = 0; sym < sym_num_names[lib]; sym++)
            {
                if (!strcmp(sym_names[lib][sym], target_sym_names[exe]))
                {
                    (*str)[idx] = target_sym_names[exe];
                    idx++;
                }
            }
        }
    }
    *num_str = idx;
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
