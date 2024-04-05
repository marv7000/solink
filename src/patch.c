#include "elf.h"
#include <libgen.h>
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
        return log_msg(LOG_ERR, "couldn't get symbols, no name buffer given!\n");
    if (!elf)
        return log_msg(LOG_ERR, "couldn't get symbols, no ELF given!\n");

    // Get the dynamic symbol table.
    elf_section* lib_sym = elf_section_get(elf, ".dynsym");
    elf_section* lib_str = elf_section_get(elf, ".dynstr");
    size num_sym = lib_sym->header.sh_size / lib_sym->header.sh_entsize;

    // Allocate a max size of all dynamic symbols.
    str* buf = calloc(num_sym, sizeof(str));
    // Write all present function symbols to the buffer.
    for (size i = 0; i < num_sym; i++)
    {
        // Reinterpret the data as an array of symbols.
        elf_symtab* sym = ((elf_symtab*)lib_sym->data) + i;
        // Only match symbols with info == STB_GLOBAL | STB_FUNC
        if (sym->sym_info == 0x12)
            buf[i] = (char*)(lib_str->data + sym->sym_name);
        else
            buf[i] = NULL;
    }
    *names = buf;
    return num_sym;
}

elf_symtab* patch_find_sym(const elf_obj* elf, str name)
{
    if (!name)
        log_msg(LOG_ERR, "[%s] couldn't find symbol, no name given!\n", basename(elf->file_name));
    if (!elf)
        log_msg(LOG_ERR, "couldn't find symbol \"%s\", no ELF given!\n", name);

    elf_section* lib_sym = elf_section_get(elf, ".dynsym");

    str* sym_names;
    size num_names = patch_get_symbols(elf, &sym_names);
    for (size i = 0; i < num_names; i++)
    {
        if (sym_names[i] == NULL)
            continue;
        if (!strcmp(sym_names[i], name))
            return ((elf_symtab*)lib_sym->data) + i;
    }
    return NULL;
}

bool patch_link_library(elf_obj* target, const elf_obj* library, u16 num_lib)
{
    // Get all symbols of the target.
    str* names;
    size num_names = patch_get_symbols(target, &names);

    // Find which symbols are available in each library. -1 means nothing provides it.
    i32* provides = calloc(num_names, sizeof(i32));
    for (size sym = 0; sym < num_names; sym++)
    {
        if (names[sym] == NULL)
            continue;
        provides[sym] = -1;
        for (u16 lib = 0; lib < num_lib; lib++)
        {
            if (patch_find_sym(library + lib, names[sym]))
            {
                // If another library has already provided this symbol, we have a conflict!
                if (provides[sym] != -1)
                    return log_msg(LOG_ERR, "conflict detected: %s and %s both provide \"%s\"\n",
                        basename(library[lib].file_name), basename(library[provides[sym]].file_name), names[sym]
                    );
                provides[sym] = (i32)lib;
            }
        }
    }

    // TODO: Create new section for the library.
    //elf_add_section(library, );

    // FIXME: This could probably be written nicer.
    for (size sym = 0; sym < num_names; sym++)
    {
        if (names[sym] == NULL)
            continue;
        // If nothing provides this symbol.
        if (provides[sym] == -1)
        {
            log_msg(LOG_WARN, "[%s <- %s] nothing provides symbol \"%s\"\n",
                basename(target->file_name), basename(library->file_name), names[sym]);
            continue;
        }

        // Deliberately ignoring result, as not all symbols might be used.
        bool linked = patch_link_symbol(target, library + provides[sym], names[sym]);
        // Unless the force flag is set.
        if (ARGS.force && !linked)
            return log_msg(LOG_WARN, "[%s <- %s] failed to link symbol \"%s\"\n",
                basename(target->file_name), basename(library->file_name), names[sym]);
    }
    return true;
}

bool patch_link_symbol(elf_obj* target, const elf_obj* library, str name)
{
    if (!name)
        return log_msg(LOG_WARN, "failed to link a symbol, no name given\n");
    if (!target)
        return log_msg(LOG_WARN, "[? <- ?] failed to link symbol \"%s\", no target given\n", name);
    if (!library)
        return log_msg(LOG_WARN, "[%s <- ?] failed to link symbol \"%s\", no library given\n",
            basename(target->file_name), name);

    // Get bytes from library function.
    elf_symtab* sym = patch_find_sym(library, name);
    elf_symtab* target_sym = patch_find_sym(target, name);
    if (!sym)
        return log_msg(LOG_WARN, "[%s <- %s] couldn't find symbol \"%s\" in the library\n",
            basename(target->file_name), basename(library->file_name), name);
    if (sym->sym_size == 0)
        return log_msg(LOG_WARN, "[%s <- %s] symbol \"%s\" has no data, skipping...\n",
            basename(target->file_name), basename(library->file_name), name);

    u8* fn_bytes = malloc(sym->sym_size);
    // Copy the bytes behind the symbol.
    // TODO: Do symbol offset resolving during elf_read() because symbol offsets are absolute!
    //memcpy(fn_bytes, library->data + sym->sym_value, sym->sym_size);

    // TODO: Append the bytes to the end of the section. Make a new section first!
    //target->sections[target->header.e_shnum - 1].data = fn_bytes;

    // TODO: Let the symbol know where the data lives now.
    if (!target_sym)
        // This should never happen, we've already established that the symbol exists.
        // This means memory got corrupted!
        return log_msg(LOG_WARN, "[%s <- %s] couldn't find symbol \"%s\" in the target\n",
            basename(target->file_name), basename(library->file_name), name);

    log_msg(LOG_INFO, "[%s <- %s] linked \"%s\" <%p>\n",
        basename(target->file_name), basename(library->file_name), name, sym->sym_value);
    return true;
}
