#pragma once
#include <stddef.h>
#include <stdbool.h>

#include <types.h>

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
	u32 sym_name;
	u8 sym_info;
	u8 sym_other;
	u16 sym_shndx;
	u64 sym_value;
	u64 sym_size;
} __attribute__((packed)) elf_symtab;

typedef struct
{
	u32 p_type;
	u32 p_flags;
	u64 p_offset;
	u64 p_vaddr;
	u64 p_paddr;
	u64 p_filesz;
	u64 p_memsz;
	u64 p_align;
} elf_program_header;

typedef struct
{
	u32 sh_name;
	u32 sh_type;
	u64 sh_flags;
	u64 sh_addr;
	u64 sh_offset;
	u64 sh_size;
	u32 sh_link;
	u32 sh_info;
	u64 sh_addralign;
	u64 sh_entsize;
} elf_section_header;

#define ELF_MAGIC 0x464c457f

typedef struct
{
	u32 e_ident_magic;
	u8 e_ident_class;
	u8 e_ident_data;
	u8 e_ident_version;
	u8 e_ident_osabi;
	u8 e_ident_abiversion;
	u16 e_type;
	elf_machine e_machine;
	u32 e_version;
	u64 e_entry;
	u64 e_phoff;
	u64 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;
} elf_header;

typedef struct
{
	elf_program_header header;
} elf_segment;

typedef struct
{
	elf_section_header header;
	u8* data;
} elf_section;

typedef struct
{
	str file_name;
	/// ELF Header
	elf_header header;
	elf_header old_header;
	/// Programs/Segments
	elf_segment* segments;
	/// Sections
	elf_section* sections;
} elf_obj;

/// \brief                  Creates and initializes a new ELF.
elf_obj elf_new(void);

/// \brief                  Frees all resources associated with the given ELF.
/// \param  [in] elf        The file to free.
void elf_free(elf_obj* elf);

/// \brief                  Performs a sanity check on the given ELF.
/// \param  [in]    elf     The file to check.
/// \returns                `true` if successful, otherwise `false`.
bool elf_check(const elf_obj* elf);

/// \brief                  Opens an ELF file and parses its contents.
/// \param  [in]    file    The file path to the ELF.
/// \param  [out]   elf     The deserialized ELF.
elf_obj elf_read(const str file);

/// \brief                  Writes an ELF struct to file.
/// \param  [in]    file    The file to save to.
/// \param  [in]    elf     The ELF to write.
void elf_write(const str file, const elf_obj* elf);

/// \brief                  Finds a section by name.
/// \param  [in]    elf     The deserialized ELF.
/// \param  [in]    name    The name of the section.
/// \returns                A pointer to the section in memory.
elf_section* elf_section_get(const elf_obj* elf, const str name);

/// \brief                  Adds a new named section.
/// \param  [in]    elf     The deserialized ELF.
/// \param  [in]    name    The name of the section.
/// \param  [in]    off     The virtual address where this section and segment lives.
/// \returns                A pointer to the new section in memory.
elf_section* elf_section_add(elf_obj* elf, const str name, u64 off);

/// \brief                  Gets the name of a section at the given index.
/// \param  [in]    elf     The file where the section is stored.
/// \param  [in]    idx     The index of the section to get the name of.
/// \returns                The string if successful, otherwise `NULL`.
str elf_section_get_name(const elf_obj* elf, u16 idx);

u16 elf_section_get_idx(const elf_obj* elf, const elf_section* name);

size elf_gnu_hash(const str name);
