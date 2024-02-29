#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    ELF_OK,
    ELF_NONE,
    ELF_INVALID_MAGIC,
    ELF_INVALID_IDENT_CLASS,
    ELF_INVALID_IDENT_DATA,
    ELF_INVALID_VERSION
} elf_error;

typedef struct
{
    uint32_t sym_name;
    uint8_t sym_info;
    uint8_t sym_other;
    uint16_t sym_shndx;
    uint64_t sym_value;
    uint64_t sym_size;
} __attribute__((packed)) elf_symtab;

typedef struct
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} elf_section_header;

typedef struct
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf_program_header;

typedef struct
{
    uint32_t e_ident_magic;
    uint8_t e_ident_class;
    uint8_t e_ident_data;
    uint8_t e_ident_version;
    uint8_t e_ident_osabi;
    uint8_t e_ident_abiversion;
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf_header;

typedef struct
{
    elf_header hdr;
    elf_program_header* ph;
    elf_section_header* sh;
    char** sn;
    uint8_t** sb;
} elf_file;

/// \brief              Performs a sanity check on the given ELF.
/// \param  [in] elf    The file to check.
/// \returns            ELF_OK if successful, any other value indicates failure.
elf_error elf_check(const elf_file* elf);

/// \brief                  Opens an ELF file and parses its contents.
/// \param  [in]    path    The file path to the ELF.
/// \param  [out]   elf     The deserialized ELF.
/// \returns                ELF_OK if successful, any other value indicates failure.
elf_error elf_read(const char* path, elf_file* elf);

/// \brief                  Writes an ELF struct to file.
/// \param  [in]    path    The file path to save to.
/// \param  [in]    elf     The deserialized ELF.
/// \returns                ELF_OK if successful, any other value indicates failure.
elf_error elf_write(const char* path, const elf_file* elf);

/// \brief                  Finds a section by name and gets its index.
/// \param  [in]    name    The name of the section.
/// \param  [in]    elf     The deserialized ELF.
/// \param  [out]   idx     The index of the section, if function returned `ELF_OK`.
/// \return                 `true` if successful, otherwise `false`.
bool elf_find_section(const char* name, const elf_file* elf, uint16_t* idx);
