#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <str.h>

typedef enum
{
    ELF_OK,
    ELF_NONE,
    ELF_ALREADY_PATCHED,
    ELF_UNSUPPORTED_ARCH,
    ELF_INVALID_MAGIC,
    ELF_INVALID_IDENT_CLASS,
    ELF_INVALID_IDENT_DATA,
    ELF_INVALID_VERSION
} elf_error;

typedef enum {
    EM_NONE         = 0,
    EM_386          = 3,
    EM_ARM          = 40,
    EM_X86_64       = 62,
    EM_AARCH64      = 183,
    EM_RISCV        = 243
} elf_machine;

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
	elf_machine e_machine;
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
    str name;
    uint8_t* data;
    size_t size;
} elf_new_section;

typedef struct
{
    /// ELF Header
    elf_header header;
    /// Program Header
    elf_program_header* program_header;
    /// Section Header
    elf_section_header* section_header;
    /// Section Data
    uint8_t** section_data;
    /// Whole file if read from somewhere.
    uint8_t* data;
    /// Whole file size.
    size_t size;
    /// Manually inserted data.
    elf_new_section* new_data;
    /// Amount of new data.
    size_t new_data_size;
} elf_file;


/// \brief              Creates and initializes a new ELF.
/// \param  [in] elf    The file to create.
/// \return             ELF_OK if successful, any other value indicates failure.
elf_error elf_new(elf_file* elf);

/// \brief              Frees all resources associated with the given ELF.
/// \param  [in] elf    The file to free.
/// \return             ELF_OK if successful, any other value indicates failure.
elf_error elf_free(elf_file* elf);

/// \brief              Performs a sanity check on the given ELF.
/// \param  [in] elf    The file to check.
/// \returns            ELF_OK if successful, any other value indicates failure.
elf_error elf_check(const elf_file* elf);

/// \brief              Returns a string representation of an ELF error.
/// \param  [in] err    The error to get.
/// \returns            A string to the description of the given error, or NULL if ELF_OK.
char* elf_error_str(elf_error err);

/// \brief                  Opens an ELF file and parses its contents.
/// \param  [in]    path    The file path to the ELF.
/// \param  [out]   elf     The deserialized ELF.
/// \returns                ELF_OK if successful, any other value indicates failure.
elf_error elf_read(str path, elf_file* elf);

/// \brief                  Writes an ELF struct to file.
/// \param  [in]    path    The file path to save to.
/// \param  [in]    elf     The deserialized ELF.
/// \returns                ELF_OK if successful, any other value indicates failure.
elf_error elf_write(str path, const elf_file* elf);

/// \brief                  Finds a section by name and gets its index.
/// \param  [in]    name    The name of the section.
/// \param  [in]    elf     The deserialized ELF.
/// \param  [out]   idx     The index of the section, if function returned `ELF_OK`.
/// \returns                `true` if successful, otherwise `false`.
bool elf_find_section(str name, const elf_file* elf, uint16_t* idx);

/// \brief                  Gets the name of a section at the given index.
/// \param  [in]    elf     The file where the section is stored.
/// \param  [in]    idx     The index of the section to get the name of.
/// \param  [out]   name    A reference to a string.
/// \returns                `true` if successful, otherwise `false`.
bool elf_get_section_name(const elf_file* elf, uint16_t idx, str* name);

/// \brief                  Adds a new section to the ELF.
/// \param  [ref]   elf     The file to add a new section to.
/// \param  [in]    section The section data.
/// \returns                ELF_OK if successful, any other value indicates failure.
elf_error elf_add_section(elf_file* elf, const elf_new_section* section);
