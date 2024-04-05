#pragma once

#include <elf.h>
#include <types.h>

/// \brief                  Extracts the symbol names from the dynamic symbol table.
/// \param  [in]    elf     The file to extract from.
/// \param  [out]   names   A reference to an array to store all symbol names in.
/// \returns                The size of the name array.
size patch_get_symbols(const elf_obj* elf, str** names);

/// \brief                  Gets a pointer to the specified symbol.
/// \param  [in]    elf     The file to extract from.
/// \param  [in]    name    The name of the symbol in the symbol table.
/// \returns                A reference to a symbol if successful, otherwise `NULL`.
elf_symtab* patch_find_sym(const elf_obj* elf, str name);

/// \brief                  Matches all symbols against each other and links the library to the target.
/// \param  [in]    target  The ELF to link to.
/// \param  [in]    library The ELF to link against.
/// \param          num_lib The amount of ELFs in `library`.
/// \returns                `true` if successful, otherwise `false`.
bool patch_link_library(elf_obj* target, const elf_obj* library, u16 num_lib);

/// \brief                  Links a given symbol from the library to the target.
/// \param  [in]    target  The ELF to link to.
/// \param  [in]    library The ELF to link against.
/// \param  [in]    name    The name of the symbol to link.
/// \returns                `true` if successful, otherwise `false`.
bool patch_link_symbol(elf_obj* target, const elf_obj* library, str name);
