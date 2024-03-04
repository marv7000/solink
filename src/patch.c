#include <stdio.h>
#include <stdlib.h>

#include <patch.h>
#include <string.h>
#include <instr.h>

bool patch_get_dynsym(const elf_file* elf, uint16_t* dynsym, uint16_t* dynstr)
{
    str dynsym_name = str_new_text(".dynsym");
    str dynstr_name = str_new_text(".dynstr");
    if (!elf_find_section(dynsym_name, elf, dynsym) || !elf_find_section(dynstr_name, elf, dynstr))
    {
        fprintf(stderr, "Error: Couldn't find dynamic symbol table!\n");
        return false;
    }
    str_free(dynsym_name);
    str_free(dynstr_name);
    return true;
}

bool patch_get_symbols(const elf_file* elf, str** names, uint64_t* num)
{
    if (!elf || !names || !num)
        return false;

    // Get the dynamic symbol table.
    uint16_t lib_sym, lib_str;
    if (!patch_get_dynsym(elf, &lib_sym, &lib_str))
        return false;

    uint64_t num_sym = elf->section_header[lib_sym].sh_size / elf->section_header[lib_sym].sh_entsize;

    // Write all entries to the buffer.
    str* buf = (str*)calloc(sizeof(str), num_sym);
    for (uint64_t i = 0; i < num_sym; i++)
    {
        elf_symtab* sym = ((elf_symtab*)elf->section_data[lib_sym]) + i;
        // Only match symbols with info == STB_GLOBAL | STB_FUNC
        if (sym->sym_info == 0x12)
        {
            buf[i] = str_new_text((char*)(elf->section_data[lib_str] + sym->sym_name));
        }
        else
        {
            buf[i] = str_new();
        }
    }
    *num = num_sym;
    *names = buf;
    return true;
}

bool patch_find_symbol(const elf_file* elf, str name, elf_symtab** sym)
{
    if (!elf || str_empty(name) || !sym)
        return false;

    uint16_t lib_sym, lib_str;
    if (!patch_get_dynsym(elf, &lib_sym, &lib_str))
        return false;

    str* sym_names;
    uint64_t num_names;
    patch_get_symbols(elf, &sym_names, &num_names);
    for (uint64_t i = 0; i < num_names; i++)
    {
        if (str_empty(sym_names[i]))
            continue;
        if (str_equal(sym_names[i], name))
        {
            *sym = ((elf_symtab*)elf->section_data[lib_sym]) + i;
            return true;
        }
    }
    return false;
}

bool patch_match_symbols(const elf_file* target, const elf_file* libs, uint64_t num_lib, str** names, uint64_t* num_str)
{
    if (!target || !libs || num_lib == 0 || !names || !num_str)
        return false;

    // Get all symbol names from the target.
    str* target_sym_names;
    uint64_t target_sym_num_names;
    if (!patch_get_symbols(target, &target_sym_names, &target_sym_num_names))
        return false;

    // Get all symbol names from libraries.
    str** sym_names = (str**)malloc(num_lib * sizeof(str*));
    uint64_t* sym_num_names = (uint64_t*)malloc(num_lib * sizeof(uint64_t));
    for (uint64_t i = 0; i < num_lib; i++)
        if (!patch_get_symbols(&libs[i], &sym_names[i], &sym_num_names[i]))
            return false;

    // Allocate memory.
    str* buf = (str*)calloc(sizeof(str), target_sym_num_names);
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
                if (str_empty(sym_names[lib][sym]) || str_empty(target_sym_names[exe]))
                    continue;
                if (str_equal(sym_names[lib][sym], target_sym_names[exe]))
                {
                    buf[idx] = target_sym_names[exe];
                    idx++;
                }
            }
        }
    }
    *num_str = idx;
    *names = buf;
    return true;
}

bool patch_link_library(elf_file* target, const elf_file* library)
{
    str* names;
    uint64_t num_names;
    if (!patch_get_symbols(target, &names, &num_names))
        return false;
    for (uint64_t sym = 0; sym < num_names; sym++)
    {
        // Deliberately ignoring result, as not all symbols might be used.
        patch_link_symbol(target, library, names[sym]);
    }
    return true;
}

bool patch_link_symbol(elf_file* target, const elf_file* library, str name)
{
    if (!target || !library || str_empty(name))
        return false;

    // Get bytes from library function.
    elf_symtab* sym;
    if (!patch_find_symbol(library, name, &sym))
        return false;
    uint8_t* fn_bytes = (uint8_t*)malloc(sym->sym_size);
    memcpy(fn_bytes, library->data + sym->sym_value, sym->sym_size);

	str link_str = str_new_text(".link_");
	str_concat(&link_str, name);
    elf_new_section new_section = {0};
    new_section.name = link_str;
    new_section.data = fn_bytes;
    new_section.size = sym->sym_size;

    if (elf_add_section(target, &new_section) != ELF_OK)
    {
        return false;
    }
    target->section_data[target->header.e_shnum - 1] = fn_bytes;

    return true;
}
