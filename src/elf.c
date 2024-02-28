#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <elf.h>

elf_error elf_check(const elf_file* elf)
{
    // No ELF was provided.
    if (!elf)
        return ELF_NONE;
    // Magic has to be exact.
    if (elf->hdr.e_ident_magic != 0x464c457f)
        return ELF_INVALID_MAGIC;
    // Either 32-bit or 64-bit.
    if (elf->hdr.e_ident_class != 1 && elf->hdr.e_ident_class != 2)
        return ELF_INVALID_IDENT_CLASS;
    // Either little endian or big endian.
    if (elf->hdr.e_ident_data != 1 && elf->hdr.e_ident_data != 2)
        return ELF_INVALID_IDENT_DATA;
    // Either little endian or big endian.
    if (elf->hdr.e_version != 1)
        return ELF_INVALID_VERSION;

    return ELF_OK;
}

elf_error elf_read(const char* path, elf_file* elf)
{
    if (!elf || !path)
        return ELF_NONE;

    elf_error err;
    FILE* f = fopen(path, "r");

    // Read header.
    fread(&elf->hdr.e_ident_magic,        sizeof(uint32_t), 1, f);
    fread(&elf->hdr.e_ident_class,        sizeof(uint8_t), 1, f);
    fread(&elf->hdr.e_ident_data,         sizeof(uint8_t), 1, f);
    fread(&elf->hdr.e_ident_version,      sizeof(uint8_t), 1, f);
    fread(&elf->hdr.e_ident_osabi,        sizeof(uint8_t), 1, f);
    fread(&elf->hdr.e_ident_abiversion,   sizeof(uint8_t), 1, f);

    fseek(f, 7, SEEK_CUR);

    fread(&elf->hdr.e_type,       sizeof(uint16_t), 1, f);
    fread(&elf->hdr.e_machine,    sizeof(uint16_t), 1, f);
    fread(&elf->hdr.e_version,    sizeof(uint32_t), 1, f);

    if (elf->hdr.e_ident_class == 1)
    {
        fread(&elf->hdr.e_entry,  sizeof(uint32_t), 1, f);
        fread(&elf->hdr.e_phoff,  sizeof(uint32_t), 1, f);
        fread(&elf->hdr.e_shoff,  sizeof(uint32_t), 1, f);
    }
    else
    {
        fread(&elf->hdr.e_entry,  sizeof(uint64_t), 1, f);
        fread(&elf->hdr.e_phoff,  sizeof(uint64_t), 1, f);
        fread(&elf->hdr.e_shoff,  sizeof(uint64_t), 1, f);
    }

    fread(&elf->hdr.e_flags,      sizeof(uint32_t), 1, f);
    fread(&elf->hdr.e_ehsize,     sizeof(uint16_t), 1, f);
    fread(&elf->hdr.e_phentsize,  sizeof(uint16_t), 1, f);
    fread(&elf->hdr.e_phnum,      sizeof(uint16_t), 1, f);
    fread(&elf->hdr.e_shentsize,  sizeof(uint16_t), 1, f);
    fread(&elf->hdr.e_shnum,      sizeof(uint16_t), 1, f);
    fread(&elf->hdr.e_shstrndx,   sizeof(uint16_t), 1, f);

    // Check header for validity.
    err = elf_check(elf);
    if (err != ELF_OK)
        return err;
    
    // Read program headers.
    fseek(f, elf->hdr.e_phoff, SEEK_SET);
    elf->ph = malloc(elf->hdr.e_phnum * sizeof(elf_program_header));
    for (uint16_t i = 0; i < elf->hdr.e_phnum; i++)
    {
        fread(&elf->ph[i].p_type, sizeof(uint32_t), 1, f);
        if (elf->hdr.e_ident_class == 1)
        {
            fread(&elf->ph[i].p_offset, sizeof(uint32_t), 1, f);
            fread(&elf->ph[i].p_vaddr,  sizeof(uint32_t), 1, f);
            fread(&elf->ph[i].p_paddr,  sizeof(uint32_t), 1, f);
            fread(&elf->ph[i].p_filesz, sizeof(uint32_t), 1, f);
            fread(&elf->ph[i].p_memsz,  sizeof(uint32_t), 1, f);
            fread(&elf->ph[i].p_flags,  sizeof(uint32_t), 1, f);
            fread(&elf->ph[i].p_align,  sizeof(uint32_t), 1, f);
        }
        else
        {
            fread(&elf->ph[i].p_flags,  sizeof(uint32_t), 1, f);
            fread(&elf->ph[i].p_offset, sizeof(uint64_t), 1, f);
            fread(&elf->ph[i].p_vaddr,  sizeof(uint64_t), 1, f);
            fread(&elf->ph[i].p_paddr,  sizeof(uint64_t), 1, f);
            fread(&elf->ph[i].p_filesz, sizeof(uint64_t), 1, f);
            fread(&elf->ph[i].p_memsz,  sizeof(uint64_t), 1, f);
            fread(&elf->ph[i].p_align,  sizeof(uint64_t), 1, f);
        }
    }
    
    // Read section headers.
    fseek(f, elf->hdr.e_shoff, SEEK_SET);
    elf->sh = malloc(elf->hdr.e_shnum * sizeof(elf_section_header));
    for (uint16_t i = 0; i < elf->hdr.e_shnum; i++)
    {
        fread(&elf->sh[i].sh_name, sizeof(uint32_t), 1, f);
        fread(&elf->sh[i].sh_type, sizeof(uint32_t), 1, f);
        if (elf->hdr.e_ident_class == 1)
        {
            fread(&elf->sh[i].sh_flags,     sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_addr,      sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_offset,    sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_size,      sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_link,      sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_info,      sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_addralign, sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_entsize,   sizeof(uint32_t), 1, f);
        }
        else
        {
            fread(&elf->sh[i].sh_flags,     sizeof(uint64_t), 1, f);
            fread(&elf->sh[i].sh_addr,      sizeof(uint64_t), 1, f);
            fread(&elf->sh[i].sh_offset,    sizeof(uint64_t), 1, f);
            fread(&elf->sh[i].sh_size,      sizeof(uint64_t), 1, f);
            fread(&elf->sh[i].sh_link,      sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_info,      sizeof(uint32_t), 1, f);
            fread(&elf->sh[i].sh_addralign, sizeof(uint64_t), 1, f);
            fread(&elf->sh[i].sh_entsize,   sizeof(uint64_t), 1, f);
        }
    }

    // Read section bodies.
    elf->sb = calloc(elf->hdr.e_shnum, sizeof(uint8_t*));
    for (uint16_t i = 0; i < elf->hdr.e_shnum; i++)
    {
        fseek(f, elf->sh[i].sh_offset, SEEK_SET);
        const uint64_t section_size = elf->sh[i].sh_size;
        elf->sb[i] = malloc(section_size);
        fread(elf->sb[i], sizeof(uint8_t), section_size, f);
    }

    // Set section names.
    elf->sn = calloc(elf->hdr.e_shnum, sizeof(char*));
    for (uint16_t i = 0; i < elf->hdr.e_shnum; i++)
    {
        // Seek to the string table + name offset.
        const uint64_t string_off = elf->sh[elf->hdr.e_shstrndx]
                                    .sh_offset + elf->sh[i].sh_name;
        fseek(f, string_off, SEEK_SET);
        const uint64_t section_size = elf->sh[i].sh_size;
        elf->sn[i] = malloc(section_size);
        fread(elf->sn[i], sizeof(char), section_size, f);
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
    fwrite(&elf->hdr.e_ident_magic,         sizeof(uint32_t), 1, f);
    fwrite(&elf->hdr.e_ident_class,         sizeof(uint8_t), 1, f);
    fwrite(&elf->hdr.e_ident_data,          sizeof(uint8_t), 1, f);
    fwrite(&elf->hdr.e_ident_version,       sizeof(uint8_t), 1, f);
    fwrite(&elf->hdr.e_ident_osabi,         sizeof(uint8_t), 1, f);
    fwrite(&elf->hdr.e_ident_abiversion,    sizeof(uint8_t), 1, f);

    fseek(f, 7, SEEK_CUR);

    fwrite(&elf->hdr.e_type,    sizeof(uint16_t), 1, f);
    fwrite(&elf->hdr.e_machine, sizeof(uint16_t), 1, f);
    fwrite(&elf->hdr.e_version, sizeof(uint32_t), 1, f);

    if (elf->hdr.e_ident_class == 1)
    {
        fwrite(&elf->hdr.e_entry,   sizeof(uint32_t), 1, f);
        fwrite(&elf->hdr.e_phoff,   sizeof(uint32_t), 1, f);
        fwrite(&elf->hdr.e_shoff,   sizeof(uint32_t), 1, f);
    }
    else
    {
        fwrite(&elf->hdr.e_entry,   sizeof(uint64_t), 1, f);
        fwrite(&elf->hdr.e_phoff,   sizeof(uint64_t), 1, f);
        fwrite(&elf->hdr.e_shoff,   sizeof(uint64_t), 1, f);
    }

    fwrite(&elf->hdr.e_flags,       sizeof(uint32_t), 1, f);
    fwrite(&elf->hdr.e_ehsize,      sizeof(uint16_t), 1, f);
    fwrite(&elf->hdr.e_phentsize,   sizeof(uint16_t), 1, f);
    fwrite(&elf->hdr.e_phnum,       sizeof(uint16_t), 1, f);
    fwrite(&elf->hdr.e_shentsize,   sizeof(uint16_t), 1, f);
    fwrite(&elf->hdr.e_shnum,       sizeof(uint16_t), 1, f);
    fwrite(&elf->hdr.e_shstrndx,    sizeof(uint16_t), 1, f);

    // Clean up.
    fclose(f);
    return ELF_OK;
}

bool elf_find_section(const char* name, const elf_file* elf, uint16_t* idx)
{
    // For every section header.
    for (uint16_t i = 0; i < elf->hdr.e_shnum; i++)
    {
        if (strcmp(elf->sn[i], name) == 0)
        {
            *idx = i;
            return true;
        }
    }

    return false;
}
