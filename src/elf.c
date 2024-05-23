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
		return log_msg(LOG_ERR, "no ELF was provided!\n");
	// Magic has to be exact.
	if (elf->header.e_ident_magic != ELF_MAGIC)
		return log_msg(LOG_ERR, "[%s] invalid magic! expected %#x, but got %#x\n",
			basename(elf->file_name), ELF_MAGIC, elf->header.e_ident_magic);
	// Either 32-bit or 64-bit.
	if (elf->header.e_ident_class != 1 && elf->header.e_ident_class != 2)
		return log_msg(LOG_ERR, "[%s] invalid ident class! expected 1 or 2, but got \"%i\"\n",
			basename(elf->file_name), elf->header.e_ident_class);
	// Either little endian or big endian.
	if (elf->header.e_ident_data != 1 && elf->header.e_ident_data != 2)
		return log_msg(LOG_ERR, "[%s] invalid ident data! expected 1 or 2, but got \"%i\"\n",
			basename(elf->file_name), elf->header.e_ident_data);
	// Version has to be 1.
	if (elf->header.e_version != 1)
		return log_msg(LOG_ERR, "[%s] invalid version! expected 1, but got \"%i\"\n",
			basename(elf->file_name), elf->header.e_version);

	switch (elf->header.e_machine)
	{
		// TODO: Machine has to be x86_64 for now. Add support for more later.
		case EM_X86_64:
			break;
		// Unsupported architecture.
		default:
			return log_msg(LOG_ERR, "[%s] unsupported architecture! got \"%i\"\n",
				basename(elf->file_name), elf->header.e_machine);
	}

	return true;
}

elf_obj elf_new(void)
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
		log_msg(LOG_ERR, "failed to read ELF file, no path given!\n");

	elf_obj elf = {0};
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

	// Keep a copy of the old header before we modify it.
	elf.old_header = elf.header;

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
		// Important: First check if this sections overlaps with the previous one.
		// In that case, move the offset away from it.
		if (i != 0 && elf->sections[i].header.sh_offset <= elf->sections[i - 1].header.sh_offset +
														   elf->sections[i - 1].header.sh_size)
			elf->sections[i].header.sh_offset = elf->sections[i - 1].header.sh_offset +
												elf->sections[i - 1].header.sh_size;

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
		str cur_name = elf_section_get_name(elf, sect);
		if (!strcmp(cur_name, name))
			return elf->sections + sect;
	}
	//! We know that some sections might not exist! If you need to, reenable this warning.
	//log_msg(LOG_WARN, "[%s] couldn't find section \"%s\"!\n", basename(elf->file_name), name);
	return NULL;
}

u16 elf_section_get_idx(const elf_obj* elf, const elf_section* sect)
{
	return (u16)((size)(sect - elf->sections) / sizeof(elf_section));
}

str elf_section_get_name(const elf_obj* elf, u16 idx)
{
	if (!elf)
		log_msg(LOG_ERR, "failed to get section name, no ELF given!\n");
	if (idx > elf->header.e_shnum)
		log_msg(LOG_ERR, "[%s] failed to get section name, index was out of bounds! (idx = %i, e_shnum = %i)\n", basename(elf->file_name), idx, elf->header.e_shnum);

	// Get the start of the section header string table.
	u8* shstrtab = elf->sections[elf->header.e_shstrndx].data;
	// Add the offset of the name on top.
	return (char*)(shstrtab + elf->sections[idx].header.sh_name);
}

elf_section* elf_section_add(elf_obj* elf, const str name, u64 off)
{
	if (!elf)
		log_msg(LOG_ERR, "failed to add section, no target given!");
	if (!name)
		log_msg(LOG_ERR, "[%s] failed to add section, no name given.", basename(elf->file_name));

	// If the section already exists, return that.
	elf_section* find = elf_section_get(elf, name);
	if (find)
		return find;

	// Make room for a new entry.
	elf->header.e_shnum++;
	elf->sections = reallocarray(elf->sections, elf->header.e_shnum, sizeof(elf_section));

	// Add the section name to the section header string table.
	elf_section* shstrtab = elf_section_get(elf, ".shstrtab");
	const u64 old_size = shstrtab->header.sh_size;
	const u64 new_size = old_size + strlen(name) + 1;
	shstrtab->header.sh_size = new_size;
	shstrtab->data = realloc(shstrtab->data, new_size);
	memcpy(shstrtab->data + old_size, name, strlen(name) + 1);

	// Initialize the new section.
	elf_section* result = elf->sections + (elf->header.e_shnum - 1);
	const elf_section_header hdr = elf->sections[elf->header.e_shnum - 2].header;
	result->header.sh_offset = hdr.sh_offset + hdr.sh_size;
	result->header.sh_addr = off;
	result->header.sh_type = 1; // SH_PROGBITS
	result->header.sh_flags = 6; // SH_WRITE_ALLOC
	result->header.sh_addralign = 1;
	result->header.sh_name = old_size;

	// Create a new program header just for this section.
	elf->header.e_phnum++;
	elf->segments = reallocarray(elf->segments, elf->header.e_phnum, sizeof(elf_segment));
	elf_segment* seg = elf->segments + (elf->header.e_phnum - 1);
	seg->header.p_paddr = off;
	seg->header.p_vaddr = off;
	seg->header.p_type = 1; // PT_LOAD
	seg->header.p_flags = 5; // PF_READ_EXEC
	seg->header.p_offset = result->header.sh_offset;
	seg->header.p_align = 4096;

	return result;
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
