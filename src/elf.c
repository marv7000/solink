#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <elf.h>
#include <instr.h>
#include <log.h>

bool elf_check(const elf_obj* elf)
{
    // No ELF was provided.
    if (!elf)
        return log_msg(LOG_ERR, "no ELF was provided!");
    // Magic has to be exact.
    if (elf->header.e_ident_magic != ELF_MAGIC)
        return log_msg(LOG_ERR, "invalid magic! expected %#x, but got %#x.", ELF_MAGIC, elf->header.e_ident_magic);
    // Either 32-bit or 64-bit.
    if (elf->header.e_ident_class != 1 && elf->header.e_ident_class != 2)
        return log_msg(LOG_ERR, "invalid ident class! expected 1 or 2, but got \"%i\".", elf->header.e_ident_class);
    // Either little endian or big endian.
    if (elf->header.e_ident_data != 1 && elf->header.e_ident_data != 2)
        return log_msg(LOG_ERR, "invalid ident data! expected 1 or 2, but got \"%i\".", elf->header.e_ident_data);
    // Version has to be 1.
    if (elf->header.e_version != 1)
        return log_msg(LOG_ERR, "invalid version! expected 1, but got \"%i\".", elf->header.e_version);

    switch (elf->header.e_machine)
    {
        // TODO: Machine has to be x86_64 for now. Add support for more later.
        case EM_X86_64:
            break;
        // Unsupported architecture.
        default:
            return log_msg(LOG_ERR, "unsupported architecture! got \"%i\".", elf->header.e_machine);
    }

    return true;
}

elf_obj elf_new()
{
    elf_obj elf = {0};

    // Set magic.
    elf.header.e_ident_magic = ELF_MAGIC;
    elf.header.e_ident_class = 2;
    elf.header.e_ident_data = 1;
    elf.header.e_ident_version = 1;
    elf.header.e_ident_osabi = 0;
    elf.header.e_ident_abiversion = 0;
    elf.header.e_type = 3;
    elf.header.e_machine = 0x3e;
    elf.header.e_version = 1;

    elf.file_name = "unnamed";

    return elf;
}

void elf_free(elf_obj* elf)
{
    // Deallocate all arrays.
    if (!elf) return;
    free(elf->sections);
    free(elf->segments);

    // Initialize all values to 0.
    memset(elf, 0, sizeof(elf_obj));
}

elf_obj elf_read(const str file)
{
    if (!file)
        log_msg(LOG_ERR, "failed to open file \"%s\"\n", file);

    elf_obj elf = {0};
    bool err = false;
    FILE* f = fopen(file, "r");
    elf.file_name = file;

    // Read header.
    fread(&elf.header.e_ident_magic, sizeof(u32), 1, f);
    fread(&elf.header.e_ident_class, sizeof(u8), 1, f);
    fread(&elf.header.e_ident_data, sizeof(u8), 1, f);
    fread(&elf.header.e_ident_version, sizeof(u8), 1, f);
    fread(&elf.header.e_ident_osabi, sizeof(u8), 1, f);
    fread(&elf.header.e_ident_abiversion, sizeof(u8), 1, f);

    fseek(f, 7, SEEK_CUR);

    fread(&elf.header.e_type, sizeof(u16), 1, f);
    fread(&elf.header.e_machine, sizeof(u16), 1, f);
    fread(&elf.header.e_version, sizeof(u32), 1, f);

    if (elf.header.e_ident_class == 1)
    {
        fread(&elf.header.e_entry, sizeof(u32), 1, f);
        fread(&elf.header.e_phoff, sizeof(u32), 1, f);
        fread(&elf.header.e_shoff, sizeof(u32), 1, f);
    }
    else
    {
        fread(&elf.header.e_entry, sizeof(u64), 1, f);
        fread(&elf.header.e_phoff, sizeof(u64), 1, f);
        fread(&elf.header.e_shoff, sizeof(u64), 1, f);
    }

    fread(&elf.header.e_flags, sizeof(u32), 1, f);
    fread(&elf.header.e_ehsize, sizeof(u16), 1, f);
    fread(&elf.header.e_phentsize, sizeof(u16), 1, f);
    fread(&elf.header.e_phnum, sizeof(u16), 1, f);
    fread(&elf.header.e_shentsize, sizeof(u16), 1, f);
    fread(&elf.header.e_shnum, sizeof(u16), 1, f);
    fread(&elf.header.e_shstrndx, sizeof(u16), 1, f);

    // Check header for validity.
    elf_check(&elf);

    // Read program headers.
    fseek(f, elf.header.e_phoff, SEEK_SET);
    elf.segments = calloc(elf.header.e_phnum, sizeof(elf_segment));
    for (u16 i = 0; i < elf.header.e_phnum; i++)
    {
        fread(&elf.segments[i].header.p_type, sizeof(u32), 1, f);
        if (elf.header.e_ident_class == 1)
        {
            fread(&elf.segments[i].header.p_offset, sizeof(u32), 1, f);
            fread(&elf.segments[i].header.p_vaddr, sizeof(u32), 1, f);
            fread(&elf.segments[i].header.p_paddr, sizeof(u32), 1, f);
            fread(&elf.segments[i].header.p_filesz, sizeof(u32), 1, f);
            fread(&elf.segments[i].header.p_memsz, sizeof(u32), 1, f);
            fread(&elf.segments[i].header.p_flags, sizeof(u32), 1, f);
            fread(&elf.segments[i].header.p_align, sizeof(u32), 1, f);
        }
        else
        {
            fread(&elf.segments[i].header.p_flags, sizeof(u32), 1, f);
            fread(&elf.segments[i].header.p_offset, sizeof(u64), 1, f);
            fread(&elf.segments[i].header.p_vaddr, sizeof(u64), 1, f);
            fread(&elf.segments[i].header.p_paddr, sizeof(u64), 1, f);
            fread(&elf.segments[i].header.p_filesz, sizeof(u64), 1, f);
            fread(&elf.segments[i].header.p_memsz, sizeof(u64), 1, f);
            fread(&elf.segments[i].header.p_align, sizeof(u64), 1, f);
        }
    }

    // Read section headers.
    fseek(f, elf.header.e_shoff, SEEK_SET);
    elf.sections = calloc(elf.header.e_shnum, sizeof(elf_section));
    for (u16 i = 0; i < elf.header.e_shnum; i++)
    {
        fread(&elf.sections[i].header.sh_name, sizeof(u32), 1, f);
        fread(&elf.sections[i].header.sh_type, sizeof(u32), 1, f);
        if (elf.header.e_ident_class == 1)
        {
            fread(&elf.sections[i].header.sh_flags, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_addr, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_offset, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_size, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_link, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_info, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_addralign, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_entsize, sizeof(u32), 1, f);
        }
        else
        {
            fread(&elf.sections[i].header.sh_flags, sizeof(u64), 1, f);
            fread(&elf.sections[i].header.sh_addr, sizeof(u64), 1, f);
            fread(&elf.sections[i].header.sh_offset, sizeof(u64), 1, f);
            fread(&elf.sections[i].header.sh_size, sizeof(u64), 1, f);
            fread(&elf.sections[i].header.sh_link, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_info, sizeof(u32), 1, f);
            fread(&elf.sections[i].header.sh_addralign, sizeof(u64), 1, f);
            fread(&elf.sections[i].header.sh_entsize, sizeof(u64), 1, f);
        }
    }

    // Read section bodies.
    for (u16 i = 0; i < elf.header.e_shnum; i++)
    {
        elf.sections[i].data = malloc(elf.sections[i].header.sh_size);
        fseek(f, elf.sections[i].header.sh_offset, SEEK_SET);
        fread(elf.sections[i].data, sizeof(u8), elf.sections[i].header.sh_size, f);
    }

    // TODO: Calculate segment <-> section mapping

    // TODO: Do internal offset computation

    // Clean up.
    fclose(f);
    return elf;
}

void elf_write(const str path, const elf_obj* elf)
{
    if (!path)
        log_msg(LOG_ERR, "no path given to write to!\n");
    if (!elf)
        log_msg(LOG_ERR, "no object given to write!\n");

    elf_check(elf);

    FILE* f = fopen(path, "w");
    fseek(f, 0, SEEK_SET);

    // Write header.
    fwrite(&elf->header.e_ident_magic, sizeof(u32), 1, f);
    fwrite(&elf->header.e_ident_class, sizeof(u8), 1, f);
    fwrite(&elf->header.e_ident_data, sizeof(u8), 1, f);
    fwrite(&elf->header.e_ident_version, sizeof(u8), 1, f);
    fwrite(&elf->header.e_ident_osabi, sizeof(u8), 1, f);
    fwrite(&elf->header.e_ident_abiversion, sizeof(u8), 1, f);

    fseek(f, 7, SEEK_CUR);

    fwrite(&elf->header.e_type, sizeof(u16), 1, f);
    fwrite(&elf->header.e_machine, sizeof(u16), 1, f);
    fwrite(&elf->header.e_version, sizeof(u32), 1, f);

    if (elf->header.e_ident_class == 1)
    {
        fwrite(&elf->header.e_entry, sizeof(u32), 1, f);
        fwrite(&elf->header.e_phoff, sizeof(u32), 1, f);
        fwrite(&elf->header.e_shoff, sizeof(u32), 1, f);
    }
    else
    {
        fwrite(&elf->header.e_entry, sizeof(u64), 1, f);
        fwrite(&elf->header.e_phoff, sizeof(u64), 1, f);
        fwrite(&elf->header.e_shoff, sizeof(u64), 1, f);
    }

    fwrite(&elf->header.e_flags, sizeof(u32), 1, f);
    fwrite(&elf->header.e_ehsize, sizeof(u16), 1, f);
    fwrite(&elf->header.e_phentsize, sizeof(u16), 1, f);
    fwrite(&elf->header.e_phnum, sizeof(u16), 1, f);
    fwrite(&elf->header.e_shentsize, sizeof(u16), 1, f);
    fwrite(&elf->header.e_shnum, sizeof(u16), 1, f);
    fwrite(&elf->header.e_shstrndx, sizeof(u16), 1, f);

    fseeko(f, (off_t)elf->header.e_phoff, SEEK_SET);

    fseeko(f, (off_t)elf->header.e_phoff, SEEK_SET);
    for (u16 i = 0; i < elf->header.e_phnum; i++)
    {
        fwrite(&elf->segments[i].header.p_type, sizeof(u32), 1, f);
        if (elf->header.e_ident_class == 1)
        {
            fwrite(&elf->segments[i].header.p_offset, sizeof(u32), 1, f);
            fwrite(&elf->segments[i].header.p_vaddr, sizeof(u32), 1, f);
            fwrite(&elf->segments[i].header.p_paddr, sizeof(u32), 1, f);
            fwrite(&elf->segments[i].header.p_filesz, sizeof(u32), 1, f);
            fwrite(&elf->segments[i].header.p_memsz, sizeof(u32), 1, f);
            fwrite(&elf->segments[i].header.p_flags, sizeof(u32), 1, f);
            fwrite(&elf->segments[i].header.p_align, sizeof(u32), 1, f);
        }
        else
        {
            fwrite(&elf->segments[i].header.p_flags, sizeof(u32), 1, f);
            fwrite(&elf->segments[i].header.p_offset, sizeof(u64), 1, f);
            fwrite(&elf->segments[i].header.p_vaddr, sizeof(u64), 1, f);
            fwrite(&elf->segments[i].header.p_paddr, sizeof(u64), 1, f);
            fwrite(&elf->segments[i].header.p_filesz, sizeof(u64), 1, f);
            fwrite(&elf->segments[i].header.p_memsz, sizeof(u64), 1, f);
            fwrite(&elf->segments[i].header.p_align, sizeof(u64), 1, f);
        }
    }

    // Write the section bodies.
    for (u16 i = 0; i < elf->header.e_shnum; i++)
    {
        // Seek to the offset given by each section header.
        fseek(f, elf->sections[i].header.sh_offset, SEEK_SET);
        // Write the data.
        fwrite(elf->sections[i].data, sizeof(u8), elf->sections[i].header.sh_size, f);
    }

    // Write the section headers.
    fseek(f, (long)elf->header.e_shoff, SEEK_SET);
    for (u16 i = 0; i < elf->header.e_shnum; i++)
    {
        fwrite(&elf->sections[i].header.sh_name, sizeof(u32), 1, f);
        fwrite(&elf->sections[i].header.sh_type, sizeof(u32), 1, f);
        if (elf->header.e_ident_class == 1)
        {
            fwrite(&elf->sections[i].header.sh_flags, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_addr, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_offset, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_size, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_link, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_info, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_addralign, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_entsize, sizeof(u32), 1, f);
        }
        else
        {
            fwrite(&elf->sections[i].header.sh_flags, sizeof(u64), 1, f);
            fwrite(&elf->sections[i].header.sh_addr, sizeof(u64), 1, f);
            fwrite(&elf->sections[i].header.sh_offset, sizeof(u64), 1, f);
            fwrite(&elf->sections[i].header.sh_size, sizeof(u64), 1, f);
            fwrite(&elf->sections[i].header.sh_link, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_info, sizeof(u32), 1, f);
            fwrite(&elf->sections[i].header.sh_addralign, sizeof(u64), 1, f);
            fwrite(&elf->sections[i].header.sh_entsize, sizeof(u64), 1, f);
        }
    }

    // Clean up.
    fclose(f);
}

elf_section* elf_section_get(const elf_obj* elf, const str name)
{
    if (!name)
        log_msg(LOG_ERR, "couldn't find a section, no name given!\n");
    if (!elf)
        log_msg(LOG_ERR, "couldn't find section \"%s\", no ELF given!\n", name);

    // For every section header.
    for (u16 sect = 0; sect < elf->header.e_shnum; sect++)
    {
        // Seek to the string table + name offset.
        str cur_name = elf_get_section_name(elf, sect);
        if (!strcmp(cur_name, name))
            return elf->sections + sect;
    }
    log_msg(LOG_ERR, "couldn't find section \"%s\" in \"%s\"!\n", name, basename(elf->file_name));
    return NULL;
}

str elf_get_section_name(const elf_obj* elf, u16 idx)
{
    if (!elf)
        log_msg(LOG_ERR, "failed to get section name, no ELF given!\n");
    if (idx > elf->header.e_shnum)
        log_msg(LOG_ERR, "\"%s\": failed to get section name, index was out of bounds! (idx = %i, e_shnum = %i)\n", basename(elf->file_name), idx, elf->header.e_shnum);

    // Get the start of the section header string table.
    u8* shstrtab = elf->sections[elf->header.e_shstrndx].data;
    char* name = (char*)(shstrtab + elf->sections[idx].header.sh_name);
    // Add the offset of the name on top.
    return name;
}

size elf_gnu_hash(const str name)
{
    str cur = name;
    size result = 5381;
    u8 ch;
    while ((ch = *cur++) != '\0') {
        result = (result << 5) + result + ch;
    }
    return result & 0xffffffff;
}
