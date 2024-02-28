#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <elf.h>

void elf_read(struct elf_file* result, const char* path)
{
    // Open the ELF for reading.
    FILE* f = fopen(path, "r");

    // Read header.
    fread(&result->header.e_ident_magic, sizeof(uint32_t), 1, f);
    if (result->header.e_ident_magic != 0x464c457f)
    {
        fprintf(stderr, "%s: Invalid magic! (%#x)\n", path, result->header.e_ident_magic);
        exit(1);
    }

    fread(&result->header.e_ident_class,        sizeof(uint8_t), 1, f);
    fread(&result->header.e_ident_data,         sizeof(uint8_t), 1, f);
    fread(&result->header.e_ident_version,      sizeof(uint8_t), 1, f);
    fread(&result->header.e_ident_osabi,        sizeof(uint8_t), 1, f);
    fread(&result->header.e_ident_abiversion,   sizeof(uint8_t), 1, f);

    fseek(f, 7, SEEK_CUR);

    fread(&result->header.e_type,       sizeof(uint16_t), 1, f);
    fread(&result->header.e_machine,    sizeof(uint16_t), 1, f);
    fread(&result->header.e_version,    sizeof(uint32_t), 1, f);

    if (result->header.e_ident_class == 1)
    {
        fread(&result->header.e_entry,  sizeof(uint32_t), 1, f);
        fread(&result->header.e_phoff,  sizeof(uint32_t), 1, f);
        fread(&result->header.e_shoff,  sizeof(uint32_t), 1, f);
    }
    else
    {
        fread(&result->header.e_entry,  sizeof(uint64_t), 1, f);
        fread(&result->header.e_phoff,  sizeof(uint64_t), 1, f);
        fread(&result->header.e_shoff,  sizeof(uint64_t), 1, f);
    }

    fread(&result->header.e_flags,      sizeof(uint32_t), 1, f);
    fread(&result->header.e_ehsize,     sizeof(uint16_t), 1, f);
    fread(&result->header.e_phentsize,  sizeof(uint16_t), 1, f);
    fread(&result->header.e_phnum,      sizeof(uint16_t), 1, f);
    fread(&result->header.e_shentsize,  sizeof(uint16_t), 1, f);
    fread(&result->header.e_shnum,      sizeof(uint16_t), 1, f);
    fread(&result->header.e_shstrndx,   sizeof(uint16_t), 1, f);

    // Read program headers.
    fseek(f, result->header.e_phoff, SEEK_SET);
    result->program_headers = malloc(result->header.e_phnum * sizeof(struct elf_program_header));
    for (uint16_t i = 0; i < result->header.e_phnum; i++)
    {
        fread(&result->program_headers[i].p_type, sizeof(uint32_t), 1, f);
        if (result->header.e_ident_class == 1)
        {
            fread(&result->program_headers[i].p_offset, sizeof(uint32_t), 1, f);
            fread(&result->program_headers[i].p_vaddr,  sizeof(uint32_t), 1, f);
            fread(&result->program_headers[i].p_paddr,  sizeof(uint32_t), 1, f);
            fread(&result->program_headers[i].p_filesz, sizeof(uint32_t), 1, f);
            fread(&result->program_headers[i].p_memsz,  sizeof(uint32_t), 1, f);
            fread(&result->program_headers[i].p_flags,  sizeof(uint32_t), 1, f);
            fread(&result->program_headers[i].p_align,  sizeof(uint32_t), 1, f);
        }
        else
        {
            fread(&result->program_headers[i].p_flags,  sizeof(uint32_t), 1, f);
            fread(&result->program_headers[i].p_offset, sizeof(uint64_t), 1, f);
            fread(&result->program_headers[i].p_vaddr,  sizeof(uint64_t), 1, f);
            fread(&result->program_headers[i].p_paddr,  sizeof(uint64_t), 1, f);
            fread(&result->program_headers[i].p_filesz, sizeof(uint64_t), 1, f);
            fread(&result->program_headers[i].p_memsz,  sizeof(uint64_t), 1, f);
            fread(&result->program_headers[i].p_align,  sizeof(uint64_t), 1, f);
        }
    }
    
    // Read section headers.
    fseek(f, result->header.e_shoff, SEEK_SET);
    result->section_headers = malloc(result->header.e_shnum * sizeof(struct elf_section_header));
    for (uint16_t i = 0; i < result->header.e_shnum; i++)
    {
        fread(&result->section_headers[i].sh_name, sizeof(uint32_t), 1, f);
        fread(&result->section_headers[i].sh_type, sizeof(uint32_t), 1, f);
        if (result->header.e_ident_class == 1)
        {
            fread(&result->section_headers[i].sh_flags,     sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_addr,      sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_offset,    sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_size,      sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_link,      sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_info,      sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_addralign, sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_entsize,   sizeof(uint32_t), 1, f);
        }
        else
        {
            fread(&result->section_headers[i].sh_flags,     sizeof(uint64_t), 1, f);
            fread(&result->section_headers[i].sh_addr,      sizeof(uint64_t), 1, f);
            fread(&result->section_headers[i].sh_offset,    sizeof(uint64_t), 1, f);
            fread(&result->section_headers[i].sh_size,      sizeof(uint64_t), 1, f);
            fread(&result->section_headers[i].sh_link,      sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_info,      sizeof(uint32_t), 1, f);
            fread(&result->section_headers[i].sh_addralign, sizeof(uint64_t), 1, f);
            fread(&result->section_headers[i].sh_entsize,   sizeof(uint64_t), 1, f);
        }
    }

    // Read section bodies.
    result->section_bodies = calloc(result->header.e_shnum, sizeof(uint8_t*));
    for (uint16_t i = 0; i < result->header.e_shnum; i++)
    {
        fseek(f, result->section_headers[i].sh_offset, SEEK_SET);
        const uint64_t section_size = result->section_headers[i].sh_size;
        result->section_bodies[i] = malloc(section_size);
        fread(result->section_bodies[i], sizeof(uint8_t), section_size, f);
    }

    // Set section names.
    result->section_names = calloc(result->header.e_shnum, sizeof(char*));
    for (uint16_t i = 0; i < result->header.e_shnum; i++)
    {
        // Seek to the string table + name offset.
        const uint64_t string_off = result->section_headers[result->header.e_shstrndx].sh_offset + result->section_headers[i].sh_name;
        fseek(f, string_off, SEEK_SET);
        const uint64_t section_size = result->section_headers[i].sh_size;
        result->section_names[i] = malloc(section_size);
        fread(result->section_names[i], sizeof(char), section_size, f);
    }

    // Clean up.
    fclose(f);
}
