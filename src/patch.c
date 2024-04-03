#include "elf.h"
#include <stdio.h>
#include <stdlib.h>

#include <patch.h>
#include <string.h>
#include <instr.h>
#include <args.h>
#include <log.h>

size patch_get_symbols(const elf_obj* elf, str** names)
{
    if (!names)
        return log_msg(LOG_ERR, "couldn't get symbols, no name buffer given!");
    if (!elf)
        return log_msg(LOG_ERR, "couldn't get symbols, no ELF given!");

    // Get the dynamic symbol table.
    u16 lib_sym = elf_find_section(elf, ".dynsym");
    u16 lib_str = elf_find_section(elf, ".dynstr");
    size num_sym = elf->sections[lib_sym].header.sh_size / elf->sections[lib_sym].header.sh_entsize;

    // Allocate a max size of all dynamic symbols.
    str* buf = calloc(num_sym, sizeof(str));
    size i = 0, written = 0;
    // Write all present function symbols to the buffer.
    while (i < num_sym)
    {
        elf_symtab* sym = ((elf_symtab*)elf->sections[lib_sym].data) + i;
        // Only match symbols with info == STB_GLOBAL | STB_FUNC
        if (sym->sym_info == 0x12)
        {
            buf[written] = (char*)(elf->sections[lib_str].data + sym->sym_name);
            written++;
        }
        i++;
    }
    *names = buf;
    return written;
}

elf_symtab* patch_find_sym(const elf_obj* elf, str name)
{
    if (!name)
        log_msg(LOG_ERR, "couldn't find symbol, no name given!", name);
    if (!elf)
        log_msg(LOG_ERR, "couldn't find symbol \"%s\", no ELF given!", name);

    u16 lib_sym = elf_find_section(elf, ".dynsym");

    str* sym_names;
    size num_names = patch_get_symbols(elf, &sym_names);
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
    size target_sym_num_names = patch_get_symbols(target, &target_sym_names);

    // Get all symbol names from libraries.
    str** sym_names = calloc(num_lib, sizeof(str*));
    size* sym_num_names = calloc(num_lib, sizeof(size));
    for (size i = 0; i < num_lib; i++)
        sym_num_names[i] = patch_get_symbols(&libs[i], &sym_names[i]);

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
    size num_names = patch_get_symbols(target, &names);

    // TODO: Create new section for the library.
    //elf_add_section(library, );

    for (size sym = 0; sym < num_names; sym++)
    {
        // Deliberately ignoring result, as not all symbols might be used.
        bool linked = patch_link_symbol(target, library, names[sym]);
        // Unless the force flag is set.
        if (ARGS.force && !linked)
            return log_msg(LOG_ERR, "failed to link symbol \"%s\"!", names[sym]);
    }
    return true;
}

bool patch_link_symbol(elf_obj* target, const elf_obj* library, str name)
{
    if (!name)
        return log_msg(LOG_ERR, "failed to link a symbol, no name given!");
    if (!target)
        return log_msg(LOG_ERR, "failed to link symbol \"%s\", no target given!", name);
    if (!library)
        return log_msg(LOG_ERR, "failed to link symbol \"%s\", no library given!", name);

    // Get bytes from library function.
    elf_symtab* sym = patch_find_sym(library, name);
    if (!sym)
        return log_msg(LOG_WARN, "couldn't find symbol \"%s\"! (sym = %p)", name, sym);

    u8* fn_bytes = malloc(sym->sym_size);
    // Copy the bytes behind the symbol.
    // TODO: Do symbol offset resolving during elf_read() because symbol offsets are absolute!
    //memcpy(fn_bytes, library->data + sym->sym_value, sym->sym_size);

    // Append the bytes to the end of the section.
    target->sections[target->header.e_shnum - 1].data = fn_bytes;

    // TODO: Let the symbol know where the data lives now.
    elf_symtab* target_sym = patch_find_sym(target, name);
    if (!target_sym)
        return log_msg(LOG_WARN, "couldn't find symbol \"%s\" in the target object.", name);

    return true;
}
