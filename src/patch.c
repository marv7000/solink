#include <stdio.h>
#include <stdlib.h>

#include <patch.h>
#include <string.h>
#include <instr.h>
#include <args.h>
#include <log.h>

bool patch_get_dynsym(const elf_obj* elf, u16* dynsym, u16* dynstr)
{
    if (!elf_find_section(".dynsym", elf, dynsym) || !elf_find_section(".dynstr", elf, dynstr))
    {
        log_msg(LOG_ERR, "couldn't find dynamic symbol table!");
        return false;
    }
    return true;
}

bool patch_get_symbols(const elf_obj* elf, str** names, size* num)
{
    if (!elf || !names || !num)
        return false;

    // Get the dynamic symbol table.
    u16 lib_sym, lib_str;
    if (!patch_get_dynsym(elf, &lib_sym, &lib_str))
        return false;

    size num_sym = elf->sections[lib_sym].header.sh_size / elf->sections[lib_sym].header.sh_entsize;

    // Write all entries to the buffer.
    str* buf = (str*)calloc(num_sym, sizeof(str));
    for (size i = 0; i < num_sym; i++)
    {
        elf_symtab* sym = ((elf_symtab*)elf->sections[lib_sym].data) + i;
        // Only match symbols with info == STB_GLOBAL | STB_FUNC
        if (sym->sym_info == 0x12)
        {
            buf[i] = (char*)(elf->sections[lib_str].data + sym->sym_name);
        }
        else
        {
            buf[i] = NULL;
        }
    }
    *num = num_sym;
    *names = buf;
    return true;
}

elf_symtab* patch_find_sym(const elf_obj* elf, str name)
{
    if (!elf || !name)
        return NULL;

    uint16_t lib_sym, lib_str;
    if (!patch_get_dynsym(elf, &lib_sym, &lib_str))
        return NULL;

    str* sym_names;
    size num_names;
    patch_get_symbols(elf, &sym_names, &num_names);
    for (size i = 0; i < num_names; i++)
    {
        if (!sym_names[i])
            continue;
        if (!strcmp(sym_names[i], name))
        {
            return ((elf_symtab*)elf->sections[lib_sym].data) + i;
        }
    }
    return NULL;
}

bool patch_match_symbols(const elf_obj* target, const elf_obj* libs, size num_lib, str** names, size* num_str)
{
    if (!target || !libs || num_lib == 0 || !names || !num_str)
        return false;

    // Get all symbol names from the target.
    str* target_sym_names;
    size target_sym_num_names;
    if (!patch_get_symbols(target, &target_sym_names, &target_sym_num_names))
        return false;

    // Get all symbol names from libraries.
    str** sym_names = calloc(num_lib, sizeof(str*));
    size* sym_num_names = calloc(num_lib, sizeof(size));
    for (size i = 0; i < num_lib; i++)
        if (!patch_get_symbols(&libs[i], &sym_names[i], &sym_num_names[i]))
            return false;

    // Allocate memory.
    str* buf = (str*)calloc(sizeof(str), target_sym_num_names);
    size idx = 0;

    // For every executable symbol.
    for (size exe = 0; exe < target_sym_num_names; exe++)
    {
        // For each library.
        for (size lib = 0; lib < num_lib; lib++)
        {
            // For each library symbol.
            for (size sym = 0; sym < sym_num_names[lib]; sym++)
            {
                if (!sym_names[lib][sym] || !target_sym_names[exe])
                    continue;
                if (!strcmp(sym_names[lib][sym], target_sym_names[exe]))
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

bool patch_link_library(elf_obj* target, const elf_obj* library)
{
    str* names;
    size num_names;
    if (!patch_get_symbols(target, &names, &num_names))
        return false;

    // TODO: Create new section for the library.
    //elf_add_section(library, );

    for (size sym = 0; sym < num_names; sym++)
    {
        // Deliberately ignoring result, as not all symbols might be used.
        bool linked = patch_link_symbol(target, library, names[sym]);
        // Unless the force flag is set.
        if (ARGS.force && !linked)
            log_msg(LOG_ERR, "failed to link symbol \"%s\"!", names[sym]);
    }
    return true;
}

bool patch_link_symbol(elf_obj* target, const elf_obj* library, str name)
{
    if (!target || !library || !name)
        return false;

    // Get bytes from library function.
    elf_symtab* sym = patch_find_sym(library, name);
    if (!sym)
        return log_msg(LOG_WARN, "couldn't find symbol \"%s\"", name);

    u8* fn_bytes = malloc(sym->sym_size);
    // Copy the bytes behind the symbol.
    //TODO: Do symbol offset resolving during elf_read() because symbol offsets are absolute!
    //memcpy(fn_bytes, library->data + sym->sym_value, sym->sym_size);

    // Append the bytes to the end of the section.
    target->sections[target->header.e_shnum - 1].data = fn_bytes;

    // Let the symbol know where the data lives now.
    elf_symtab* target_sym = patch_find_sym(target, name);
    if (!sym)
        return log_msg(LOG_WARN, "couldn't find symbol \"%s\" in the target object.", name);


    return true;
}
