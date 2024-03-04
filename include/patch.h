#pragma once
#include <stdbool.h>

#include <elf.h>
#include <str.h>

/// \brief                  Checks an ELF for presence of the dynamic symbol table (aka ".dynsym", ".dynstr").
/// \param  [in]    elf     The file to check.
/// \param  [ref]   dynsym  The index to the dynamic symbol table.
/// \param  [ref]   dynstr  The index to the dynamic symbol string table.
/// \returns                `true` if present, otherwise `false`.
bool patch_get_dynsym(const elf_file* elf, uint16_t* dynsym, uint16_t* dynstr);

/// \brief                  Extracts the symbol names from the dynamic symbol table.
/// \param  [in]    elf     The file to extract from.
/// \param  [ref]   names   A reference to an array of all symbol names.
/// \param  [ref]   num     The size of the name array.
/// \returns                `true` if symbol table is present, otherwise `false`.
bool patch_get_symbols(const elf_file* elf, str** names, uint64_t* num);

/// \brief                  Gets a pointer to the specified symbol.
/// \param  [in]    elf     The file to extract from.
/// \param  [in]    idx     The index of the symbol in the symbol table.
/// \param  [ref]   sym     Address to write the result to.
/// \returns                `true` if successful, otherwise `false`.
bool patch_get_symbol(const elf_file* elf, uint64_t idx, elf_symtab** sym);

/// \brief                  Gets a pointer to the specified symbol.
/// \param  [in]    elf     The file to extract from.
/// \param  [in]    name    The name of the symbol in the symbol table.
/// \param  [ref]   sym     Address to write the result to.
/// \returns                `true` if successful, otherwise `false`.
bool patch_find_symbol(const elf_file* elf, str name, elf_symtab** sym);

/// \brief                  Matches all library names against the ones from the target.
/// \param  [in]    target  The file to match against.
/// \param  [in]    libs    An array of all libraries to match against.
/// \param  [out]   num_lib Number of elements in the array `libs`.
/// \param  [out]   names   A reference to an array of matched symbols.
/// \param  [out]   num_str Number of elements in the array `str`.
/// \returns                `true` if successful, otherwise `false`.
bool patch_match_symbols(const elf_file* target, const elf_file* libs, uint64_t num_lib, str** names, uint64_t* num_str);

/// \brief                  Matches all symbols against each other and links the library to the target.
/// \param  [in]    target  The ELF to link to.
/// \param  [in]    library The ELF to link against.
/// \returns                `true` if successful, otherwise `false`.
bool patch_link_library(elf_file* target, const elf_file* library);

/// \brief                  Links a given symbol from the library to the target.
/// \param  [in]    target  The ELF to link to.
/// \param  [in]    library The ELF to link against.
/// \param  [in]    name    The name of the symbol to link.
/// \returns                `true` if successful, otherwise `false`.
bool patch_link_symbol(elf_file* target, const elf_file* library, str name);
