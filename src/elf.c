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
    if (elf->header.e_machine != EM_X86_64)
        return ELF_UNSUPPORTED_ARCH;
    // ELF cannot be patched already.
    if (elf->data && *(uint64_t*)(&elf->data[elf->size - 8]) == *(const uint64_t*)".patched")
        return ELF_ALREADY_PATCHED;

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

    // Clear the struct.
    memset(elf, 0, sizeof(elf_file));

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

    // Read only the body, not the header.
    fseek(f, 0, SEEK_END);
    elf->size = ftell(f);
    fseek(f, 0, SEEK_SET);
    elf->data = (uint8_t*)malloc(elf->size);
    fread(elf->data, sizeof(uint8_t), elf->size, f);


    // Check ELF again.
    err = elf_check(elf);
    if (err != ELF_OK)
        return err;

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

    // Write the body, then seek over it again, so it can be overwritten.
    fwrite(elf->data, sizeof(uint8_t), elf->size, f);
    fseek(f, 0, SEEK_SET);

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

    uint16_t sh_fix = elf->header.e_shnum + elf->new_data_size;
    fwrite(&sh_fix, sizeof(uint16_t), 1, f);
    fwrite(&elf->header.e_shstrndx, sizeof(uint16_t), 1, f);

    fseek(f, (long)elf->header.e_phoff, SEEK_SET);
    for (uint16_t i = 0; i < elf->header.e_phnum; i++)
    {
        fwrite(&elf->program_header[i].p_type, sizeof(uint32_t), 1, f);
        if (elf->header.e_ident_class == 1)
        {
            fwrite(&elf->program_header[i].p_offset, sizeof(uint32_t), 1, f);
            fwrite(&elf->program_header[i].p_vaddr, sizeof(uint32_t), 1, f);
            fwrite(&elf->program_header[i].p_paddr, sizeof(uint32_t), 1, f);
            fwrite(&elf->program_header[i].p_filesz, sizeof(uint32_t), 1, f);
            fwrite(&elf->program_header[i].p_memsz, sizeof(uint32_t), 1, f);
            fwrite(&elf->program_header[i].p_flags, sizeof(uint32_t), 1, f);
            fwrite(&elf->program_header[i].p_align, sizeof(uint32_t), 1, f);
        }
        else
        {
            fwrite(&elf->program_header[i].p_flags, sizeof(uint32_t), 1, f);
            fwrite(&elf->program_header[i].p_offset, sizeof(uint64_t), 1, f);
            fwrite(&elf->program_header[i].p_vaddr, sizeof(uint64_t), 1, f);
            fwrite(&elf->program_header[i].p_paddr, sizeof(uint64_t), 1, f);
            fwrite(&elf->program_header[i].p_filesz, sizeof(uint64_t), 1, f);
            fwrite(&elf->program_header[i].p_memsz, sizeof(uint64_t), 1, f);
            fwrite(&elf->program_header[i].p_align, sizeof(uint64_t), 1, f);
        }
    }

    fseek(f, (long)elf->header.e_shoff, SEEK_SET);
    for (uint16_t i = 0; i < elf->header.e_shnum; i++)
    {
        fwrite(&elf->section_header[i].sh_name, sizeof(uint32_t), 1, f);
        fwrite(&elf->section_header[i].sh_type, sizeof(uint32_t), 1, f);
        if (elf->header.e_ident_class == 1)
        {
            fwrite(&elf->section_header[i].sh_flags, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_addr, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_offset, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_size, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_link, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_info, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_addralign, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_entsize, sizeof(uint32_t), 1, f);
        }
        else
        {
            fwrite(&elf->section_header[i].sh_flags, sizeof(uint64_t), 1, f);
            fwrite(&elf->section_header[i].sh_addr, sizeof(uint64_t), 1, f);
            fwrite(&elf->section_header[i].sh_offset, sizeof(uint64_t), 1, f);
            fwrite(&elf->section_header[i].sh_size, sizeof(uint64_t), 1, f);
            fwrite(&elf->section_header[i].sh_link, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_info, sizeof(uint32_t), 1, f);
            fwrite(&elf->section_header[i].sh_addralign, sizeof(uint64_t), 1, f);
            fwrite(&elf->section_header[i].sh_entsize, sizeof(uint64_t), 1, f);
        }
    }

    // Total size of new bytes written in the section header.
    const uint64_t section_header_size = elf->header.e_ident_class == 1 ? 0x28 : 0x40;
    const uint64_t new_header_size = section_header_size * elf->new_data_size;
    const uint64_t pos = ftello(f);
    const uint64_t align = 32;
    uint64_t new_data_offset = pos + new_header_size;
    if (new_data_offset % align != 0)
        new_data_offset += align - pos % align;

    uint64_t data_body_counter = 0;

    // Write newly added sections.
    for (uint64_t i = 0; i < elf->new_data_size; i++)
    {
        // Calculate name offset.
        // TODO: This is broken in objdump.
        //uint64_t base = elf->section_header[elf->header.e_shstrndx].sh_offset;
        //uint64_t name = new_data_offset + total_body_size - base;
        uint64_t name = 0;
        fwrite(&name, sizeof(uint32_t), 1, f);

        uint32_t type = 1;
        fwrite(&type, sizeof(uint32_t), 1, f);

        uint64_t flags = 6;
        uint64_t actual_offset = new_data_offset + data_body_counter;
        data_body_counter += elf->new_data[i].size;
        if (data_body_counter % align != 0)
            data_body_counter += align - data_body_counter % align;
        uint64_t addr = 0x400000 | actual_offset;
        uint64_t link = 0;
        uint64_t info = 0;
        uint64_t ent_size = 0;

        if (elf->header.e_ident_class == 1)
        {
            fwrite(&flags, sizeof(uint32_t), 1, f);
            fwrite(&addr, sizeof(uint32_t), 1, f);
            fwrite(&actual_offset, sizeof(uint32_t), 1, f);
            fwrite(&elf->new_data[i].size, sizeof(uint32_t), 1, f);
            fwrite(&link, sizeof(uint32_t), 1, f);
            fwrite(&info, sizeof(uint32_t), 1, f);
            fwrite(&align, sizeof(uint32_t), 1, f);
            fwrite(&ent_size, sizeof(uint32_t), 1, f);
        }
        else
        {
            fwrite(&flags, sizeof(uint64_t), 1, f);
            fwrite(&addr, sizeof(uint64_t), 1, f);
            fwrite(&actual_offset, sizeof(uint64_t), 1, f);
            fwrite(&elf->new_data[i].size, sizeof(uint64_t), 1, f);
            fwrite(&link, sizeof(uint32_t), 1, f);
            fwrite(&info, sizeof(uint32_t), 1, f);
            fwrite(&align, sizeof(uint64_t), 1, f);
            fwrite(&ent_size, sizeof(uint64_t), 1, f);
        }
    }
    // Write the bodies.
    for (uint64_t i = 0; i < elf->new_data_size; i++)
    {
        // Align.
        uint64_t cur = ftello(f);
        if (cur % align != 0)
            cur += (align - cur % align);
        fseek(f, cur, SEEK_SET);

        // Write.
        fwrite(elf->new_data[i].data, sizeof(char), elf->new_data[i].size, f);
    }
    // Write the string table.
    // for (uint64_t i = 0; i < elf->new_data_size; i++)
    // {
    //     fwrite(elf->new_data[i].name, sizeof(char), strlen(elf->new_data[i].name) + 1, f);
    // }

    // Write a marker at the end to let solink know that this ELF has been patched.
    fwrite(".patched", sizeof(uint64_t), 1, f);

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
    uint8_t* data = elf->data + elf->section_header[elf->header.e_shstrndx].sh_offset;
    *name = (char*)(data + elf->section_header[idx].sh_name);
    return true;
}

char* elf_error_str(elf_error err)
{
    switch (err)
    {
        case ELF_NONE:
            return "No data given. (ELF_NONE)";
        case ELF_ALREADY_PATCHED:
            return "ELF has already been patched. (ELF_ALREADY_PATCHED)";
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

elf_error elf_add_section(elf_file* elf, const elf_new_section* section)
{
    if (!elf || !section)
        return ELF_NONE;

    elf->new_data_size++;
    if (!elf->new_data)
        elf->new_data = (elf_new_section*)malloc(sizeof(elf_new_section));
    else
    {
        elf_new_section* new_mem = (elf_new_section*)malloc(elf->new_data_size * sizeof(elf_new_section));
        memcpy(new_mem, elf->new_data, elf->new_data_size * sizeof(elf_new_section));
        free(elf->new_data);
        elf->new_data = new_mem;
    }

    elf->new_data[elf->new_data_size - 1].name = section->name;

    elf->new_data[elf->new_data_size - 1].data = section->data;
    elf->new_data[elf->new_data_size - 1].size = section->size;

    return ELF_OK;
}
