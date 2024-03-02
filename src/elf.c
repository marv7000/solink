#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>

elf_error elf_check(const elf_file* elf)
{
    // No ELF was provided.
    if (!elf)
        return ELF_NONE;
    // Magic has to be exact.
    if (elf->header.e_ident_magic != 0x464c457f)
        return ELF_INVALID_MAGIC;
    // Either 32-bit or 64-bit.
    if (elf->header.e_ident_class != 1 && elf->header.e_ident_class != 2)
        return ELF_INVALID_IDENT_CLASS;
    // Either little endian or big endian.
    if (elf->header.e_ident_data != 1 && elf->header.e_ident_data != 2)
        return ELF_INVALID_IDENT_DATA;
    // Version has to be 1.
    if (elf->header.e_version != 1)
        return ELF_INVALID_VERSION;
    // Machine has to be x86_64.
    if (elf->header.e_machine != 0x3e)
        return ELF_UNSUPPORTED_ARCH;

    return ELF_OK;
}

elf_error elf_new(elf_file* elf)
{
    if (!elf)
        return ELF_NONE;

    // Initialize all values to 0.
    memset(elf, 0, sizeof(elf_file));

    // Set magic.
    elf->header.e_ident_magic = 0x464c457f;
    elf->header.e_ident_class = 2;
    elf->header.e_ident_data = 1;
    elf->header.e_ident_version = 1;
    elf->header.e_ident_osabi = 0;
    elf->header.e_ident_abiversion = 0;
    elf->header.e_type = 3;
    elf->header.e_machine = 0x3e;
    elf->header.e_version = 1;

    return ELF_OK;
}

elf_error elf_free(elf_file* elf)
{
    // Deallocate all arrays.
    if (!elf) return ELF_NONE;
    if (elf->section_header) free(elf->section_header);
    if (elf->program_header) free(elf->program_header);
    if (elf->section_data) free(elf->section_data);

    // Initialize all values to 0.
    memset(elf, 0, sizeof(elf_file));

    return ELF_OK;
}

elf_error elf_read(const char* path, elf_file* elf)
{
    if (!elf || !path)
        return ELF_NONE;

    elf_error err;
    FILE* f = fopen(path, "r");

    // Read header.
    fread(&elf->header.e_ident_magic, sizeof(uint32_t), 1, f);
    fread(&elf->header.e_ident_class, sizeof(uint8_t), 1, f);
    fread(&elf->header.e_ident_data, sizeof(uint8_t), 1, f);
    fread(&elf->header.e_ident_version, sizeof(uint8_t), 1, f);
    fread(&elf->header.e_ident_osabi, sizeof(uint8_t), 1, f);
    fread(&elf->header.e_ident_abiversion, sizeof(uint8_t), 1, f);

    fseek(f, 7, SEEK_CUR);

    fread(&elf->header.e_type, sizeof(uint16_t), 1, f);
    fread(&elf->header.e_machine, sizeof(uint16_t), 1, f);
    fread(&elf->header.e_version, sizeof(uint32_t), 1, f);

    if (elf->header.e_ident_class == 1)
    {
        fread(&elf->header.e_entry, sizeof(uint32_t), 1, f);
        fread(&elf->header.e_phoff, sizeof(uint32_t), 1, f);
        fread(&elf->header.e_shoff, sizeof(uint32_t), 1, f);
    }
    else
    {
        fread(&elf->header.e_entry, sizeof(uint64_t), 1, f);
        fread(&elf->header.e_phoff, sizeof(uint64_t), 1, f);
        fread(&elf->header.e_shoff, sizeof(uint64_t), 1, f);
    }

    fread(&elf->header.e_flags, sizeof(uint32_t), 1, f);
    fread(&elf->header.e_ehsize, sizeof(uint16_t), 1, f);
    fread(&elf->header.e_phentsize, sizeof(uint16_t), 1, f);
    fread(&elf->header.e_phnum, sizeof(uint16_t), 1, f);
    fread(&elf->header.e_shentsize, sizeof(uint16_t), 1, f);
    fread(&elf->header.e_shnum, sizeof(uint16_t), 1, f);
    fread(&elf->header.e_shstrndx, sizeof(uint16_t), 1, f);

    // Check header for validity.
    err = elf_check(elf);
    if (err != ELF_OK)
        return err;
    
    // Read program headers.
    fseek(f, elf->header.e_phoff, SEEK_SET);
    elf->program_header = malloc(elf->header.e_phnum * sizeof(elf_program_header));
    for (uint16_t i = 0; i < elf->header.e_phnum; i++)
    {
        fread(&elf->program_header[i].p_type, sizeof(uint32_t), 1, f);
        if (elf->header.e_ident_class == 1)
        {
            fread(&elf->program_header[i].p_offset, sizeof(uint32_t), 1, f);
            fread(&elf->program_header[i].p_vaddr, sizeof(uint32_t), 1, f);
            fread(&elf->program_header[i].p_paddr, sizeof(uint32_t), 1, f);
            fread(&elf->program_header[i].p_filesz, sizeof(uint32_t), 1, f);
            fread(&elf->program_header[i].p_memsz, sizeof(uint32_t), 1, f);
            fread(&elf->program_header[i].p_flags, sizeof(uint32_t), 1, f);
            fread(&elf->program_header[i].p_align, sizeof(uint32_t), 1, f);
        }
        else
        {
            fread(&elf->program_header[i].p_flags, sizeof(uint32_t), 1, f);
            fread(&elf->program_header[i].p_offset, sizeof(uint64_t), 1, f);
            fread(&elf->program_header[i].p_vaddr, sizeof(uint64_t), 1, f);
            fread(&elf->program_header[i].p_paddr, sizeof(uint64_t), 1, f);
            fread(&elf->program_header[i].p_filesz, sizeof(uint64_t), 1, f);
            fread(&elf->program_header[i].p_memsz, sizeof(uint64_t), 1, f);
            fread(&elf->program_header[i].p_align, sizeof(uint64_t), 1, f);
        }
    }
    
    // Read section headers.
    fseek(f, elf->header.e_shoff, SEEK_SET);
    elf->section_header = malloc(elf->header.e_shnum * sizeof(elf_section_header));
    for (uint16_t i = 0; i < elf->header.e_shnum; i++)
    {
        fread(&elf->section_header[i].sh_name, sizeof(uint32_t), 1, f);
        fread(&elf->section_header[i].sh_type, sizeof(uint32_t), 1, f);
        if (elf->header.e_ident_class == 1)
        {
            fread(&elf->section_header[i].sh_flags, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_addr, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_offset, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_size, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_link, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_info, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_addralign, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_entsize, sizeof(uint32_t), 1, f);
        }
        else
        {
            fread(&elf->section_header[i].sh_flags, sizeof(uint64_t), 1, f);
            fread(&elf->section_header[i].sh_addr, sizeof(uint64_t), 1, f);
            fread(&elf->section_header[i].sh_offset, sizeof(uint64_t), 1, f);
            fread(&elf->section_header[i].sh_size, sizeof(uint64_t), 1, f);
            fread(&elf->section_header[i].sh_link, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_info, sizeof(uint32_t), 1, f);
            fread(&elf->section_header[i].sh_addralign, sizeof(uint64_t), 1, f);
            fread(&elf->section_header[i].sh_entsize, sizeof(uint64_t), 1, f);
        }
    }

    // Read section bodies.
    elf->section_data = calloc(elf->header.e_shnum, sizeof(uint8_t*));
    for (uint16_t i = 0; i < elf->header.e_shnum; i++)
    {
        fseek(f, elf->section_header[i].sh_offset, SEEK_SET);
        const uint64_t section_size = elf->section_header[i].sh_size;
        elf->section_data[i] = malloc(section_size);
        fread(elf->section_data[i], sizeof(uint8_t), section_size, f);
    }

    // Clean up.
    fclose(f);
    return ELF_OK;
}

elf_error elf_write(const char* path, const elf_file* elf)
{
    if (!path || !elf)
        return ELF_NONE;

    elf_error err = elf_check(elf);
    if (err != ELF_OK)
        return err;
    
    FILE* f = fopen(path, "w");

    // Write header.
    fwrite(&elf->header.e_ident_magic, sizeof(uint32_t), 1, f);
    fwrite(&elf->header.e_ident_class, sizeof(uint8_t), 1, f);
    fwrite(&elf->header.e_ident_data, sizeof(uint8_t), 1, f);
    fwrite(&elf->header.e_ident_version, sizeof(uint8_t), 1, f);
    fwrite(&elf->header.e_ident_osabi, sizeof(uint8_t), 1, f);
    fwrite(&elf->header.e_ident_abiversion, sizeof(uint8_t), 1, f);

    fseek(f, 7, SEEK_CUR);

    fwrite(&elf->header.e_type, sizeof(uint16_t), 1, f);
    fwrite(&elf->header.e_machine, sizeof(uint16_t), 1, f);
    fwrite(&elf->header.e_version, sizeof(uint32_t), 1, f);

    if (elf->header.e_ident_class == 1)
    {
        fwrite(&elf->header.e_entry, sizeof(uint32_t), 1, f);
        fwrite(&elf->header.e_phoff, sizeof(uint32_t), 1, f);
        fwrite(&elf->header.e_shoff, sizeof(uint32_t), 1, f);
    }
    else
    {
        fwrite(&elf->header.e_entry, sizeof(uint64_t), 1, f);
        fwrite(&elf->header.e_phoff, sizeof(uint64_t), 1, f);
        fwrite(&elf->header.e_shoff, sizeof(uint64_t), 1, f);
    }

    fwrite(&elf->header.e_flags, sizeof(uint32_t), 1, f);
    fwrite(&elf->header.e_ehsize, sizeof(uint16_t), 1, f);
    fwrite(&elf->header.e_phentsize, sizeof(uint16_t), 1, f);
    fwrite(&elf->header.e_phnum, sizeof(uint16_t), 1, f);
    fwrite(&elf->header.e_shentsize, sizeof(uint16_t), 1, f);
    fwrite(&elf->header.e_shnum, sizeof(uint16_t), 1, f);
    fwrite(&elf->header.e_shstrndx, sizeof(uint16_t), 1, f);

    // Clean up.
    fclose(f);
    return ELF_OK;
}

bool elf_find_section(const char* name, const elf_file* elf, uint16_t* idx)
{
    if (!name || !elf || !idx)
        return false;

    // For every section header.
    for (uint16_t sect = 0; sect < elf->header.e_shnum; sect++)
    {
        // Seek to the string table + name offset.
        char* string_off;
        elf_get_section_name(elf, sect, &string_off);
        if (strcmp((char*)string_off, name) == 0)
        {
            *idx = sect;
            return true;
        }
    }

    return false;
}

bool elf_get_section_name(const elf_file* elf, uint16_t idx, char** name)
{
    if (!elf || !name)
        return false;
    if (idx > elf->header.e_shnum)
        return false;
    *name = (char*)(elf->section_data[elf->header.e_shstrndx] + elf->section_header[idx].sh_name);
    return true;
}

char* elf_error_str(elf_error err)
{
    switch (err)
    {
        case ELF_NONE:
            return "No file given. (ELF_NONE)";
        case ELF_UNSUPPORTED_ARCH:
            return "Unsupported machine architecture. (ELF_UNSUPPORTED_ARCH)";
        case ELF_INVALID_MAGIC:
            return "Invalid file magic. (ELF_INVALID_MAGIC)";
        case ELF_INVALID_IDENT_CLASS:
            return "Invalid file format, expected 32-bit or 64-bit. (ELF_INVALID_IDENT_CLASS)";
        case ELF_INVALID_IDENT_DATA:
            return "Invalid file endianness, expect little or big endian. (ELF_INVALID_IDENT_DATA)";
        case ELF_INVALID_VERSION:
            return "Invalid file version, expected version 1. (ELF_INVALID_VERSION)";
        default:
            return NULL;
    }
}
