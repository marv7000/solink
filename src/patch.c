#include "elf.h"
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include <patch.h>
#include <string.h>
#include <instr.h>
#include <args.h>
#include <log.h>

size patch_get_symbols(const elf_obj* elf, str** names)
{
	if (!names)
		return log_msg(LOG_ERR, "couldn't get symbols, no name buffer given!\n");
	if (!elf)
		return log_msg(LOG_ERR, "couldn't get symbols, no ELF given!\n");

	// Get the dynamic symbol table.
	elf_section* lib_sym = elf_section_get(elf, ".dynsym");
	elf_section* lib_str = elf_section_get(elf, ".dynstr");
	size num_sym = lib_sym->header.sh_size / lib_sym->header.sh_entsize;

	// Allocate a max size of all dynamic symbols.
	str* buf = calloc(num_sym, sizeof(str));
	// Write all present function symbols to the buffer.
	for (size i = 0; i < num_sym; i++)
	{
		// Reinterpret the data as an array of symbols.
		elf_symtab* sym = ((elf_symtab*)lib_sym->data) + i;
		// Only match symbols with info == STB_GLOBAL | STB_FUNC
		if (sym->sym_info == 0x12)
			buf[i] = (char*)(lib_str->data + sym->sym_name);
		else
			buf[i] = NULL;
	}
	*names = buf;
	return num_sym;
}

elf_symtab* patch_find_sym(const elf_obj* elf, str name)
{
	if (!name)
		log_msg(LOG_ERR, "[%s] couldn't find symbol, no name given!\n", basename(elf->file_name));
	if (!elf)
		log_msg(LOG_ERR, "couldn't find symbol \"%s\", no ELF given!\n", name);

	elf_section* lib_sym = elf_section_get(elf, ".dynsym");

	str* sym_names;
	size num_names = patch_get_symbols(elf, &sym_names);
	for (size i = 0; i < num_names; i++)
	{
		if (sym_names[i] == NULL)
			continue;
		if (!strcmp(sym_names[i], name))
			return ((elf_symtab*)lib_sym->data) + i;
	}
	return NULL;
}

bool patch_link_library(elf_obj* target, const elf_obj* library, u16 num_lib)
{
	// Get all symbols of the target.
	str* names;
	size num_names = patch_get_symbols(target, &names);

	// Find which symbols are available in each library. -1 means nothing provides it.
	i32* provides = calloc(num_names, sizeof(i32));
	for (size sym = 0; sym < num_names; sym++)
	{
		if (names[sym] == NULL)
			continue;
		provides[sym] = -1;
		for (u16 lib = 0; lib < num_lib; lib++)
		{
			if (patch_find_sym(library + lib, names[sym]))
			{
				// If another library has already provided this symbol, we have a conflict!
				if (provides[sym] != -1)
					return log_msg(LOG_ERR, "conflict detected: %s and %s both provide \"%s\"\n",
						basename(library[lib].file_name), basename(library[provides[sym]].file_name), names[sym]
					);
				provides[sym] = (i32)lib;
			}
		}
	}

	// Create new section for all libraries on the target, or find an existing one.
	str sect_name = ".solink";
	elf_section* add_sect = elf_section_add(target, sect_name, 0x410000);

	// FIXME: This could probably be written nicer.
	for (size sym = 0; sym < num_names; sym++)
	{
		if (names[sym] == NULL)
			continue;
		// If nothing provides this symbol.
		if (provides[sym] == -1)
		{
			log_msg(LOG_WARN, "[%s <- ?] nothing provides symbol \"%s\"\n",
				basename(target->file_name), names[sym]);
			continue;
		}

		// Deliberately ignoring result, as not all symbols might be used.
		bool linked = patch_link_symbol(target, add_sect, library + provides[sym], names[sym]);
		// Unless the force flag is set.
		if (ARGS.force && !linked)
			return log_msg(LOG_WARN, "[%s <- %s] failed to link symbol \"%s\"\n",
				basename(target->file_name), basename(library->file_name), names[sym]);
	}
	patch_fix_offsets(target);
	return true;
}

static u64 basic = 0;

bool patch_link_symbol(elf_obj* target, elf_section* sect, const elf_obj* library, str name)
{
	if (!name)
		return log_msg(LOG_WARN, "failed to link a symbol, no name given\n");
	if (!target)
		return log_msg(LOG_WARN, "[? <- ?] failed to link symbol \"%s\", no target given\n", name);
	if (!sect)
		return log_msg(LOG_WARN, "[%s <- ?] failed to link symbol \"%s\", no section given\n", basename(target->file_name), name);
	if (!library)
		return log_msg(LOG_WARN, "[%s <- ?] failed to link symbol \"%s\", no library given\n",
			basename(target->file_name), name);

	// Get bytes from library function.
	elf_symtab* sym = patch_find_sym(library, name);
	elf_symtab* target_sym = patch_find_sym(target, name);
	if (!sym)
		return log_msg(LOG_WARN, "[%s <- %s] couldn't find symbol \"%s\" in the library\n",
			basename(target->file_name), basename(library->file_name), name);
	if (sym->sym_size == 0)
		return log_msg(LOG_WARN, "[%s <- %s] symbol \"%s\" has no data, skipping...\n",
			basename(target->file_name), basename(library->file_name), name);

	const u64 old_size = sect->header.sh_size;
	const u64 new_size = old_size + sym->sym_size;
	sect->data = reallocarray(sect->data, new_size, sizeof(u64));

	// Get the section this symbol is located in, take its file offset and use that as a baseline
	// to get the relative offset.
	const elf_section* sym_section = library->sections + sym->sym_shndx;
	const u64 offset = sym->sym_value - sym_section->header.sh_offset;
	// Append the bytes to the end of the section.
	memcpy(sect->data + old_size, sym_section->data + offset, sym->sym_size);

	// Update the new section header.
	sect->header.sh_size = new_size;
	target->segments[target->header.e_phnum - 1].header.p_memsz = new_size;
	target->segments[target->header.e_phnum - 1].header.p_filesz = new_size;

	// TODO: Let the symbol know where the data lives now.
	if (!target_sym)
		// This should never happen, we've already established that the symbol exists.
		// This means memory got corrupted!
		return log_msg(LOG_WARN, "[%s <- %s] couldn't find symbol \"%s\" in the target, possible memory corruption!\n",
			basename(target->file_name), basename(library->file_name), name);

	//target_sym->sym_value = sym_section->header.sh_addr + new_size;
	//target_sym->sym_size = new_size;
	//target_sym->sym_shndx = elf_section_get_idx(target, sect);

	// Update the PLT section.
	elf_section* plt = elf_section_get(target, ".plt");

	// TODO: Get the offset of the symbol in the PLT.
	u64 plt_symoff = 0x30;
	plt_symoff += basic;
	basic += 0x10;

	// Get the vaddr of the PLT section and calculate the offset to the .solink section.
	u64 addr = (sect->header.sh_addr + old_size) - // Symbol text offset.
			   (plt->header.sh_addr + plt_symoff); // PLT entry for this symbol.

	// Get the instruction bytes.
	u8 instr[0x10];
	if (!instr_get_bytes(target->header.e_machine, addr, instr))
		log_msg(LOG_ERR, "unsupported architecture! (%x)\n", target->header.e_machine);
	// Overwrite the PLT entry.
	memcpy(plt->data + plt_symoff, instr, 0x10);

	log_msg(LOG_INFO, "[%s <- %s] linked \"%s\" <%p>\n",
		basename(target->file_name), basename(library->file_name), name, sym->sym_value);
	return true;
}

void patch_fix_offsets(elf_obj* elf)
{
	for (u16 i = 0; i < elf->header.e_shnum; i++)
	{
		const u64 segm_limit = elf->header.e_ehsize + (elf->header.e_phentsize * elf->header.e_phnum);
		if (elf->sections[i].header.sh_offset < segm_limit)
		{
			elf->sections[i].header.sh_offset = segm_limit;
		}
		if (i != 0)
		{
			// Important: First check if this sections overlaps with the previous one.
			// In that case, move the offset away from it.
			const u64 prev_limit = elf->sections[i - 1].header.sh_offset + elf->sections[i - 1].header.sh_size;
			if (elf->sections[i].header.sh_offset < prev_limit)
			{
				elf->sections[i].header.sh_offset = prev_limit + prev_limit % elf->sections[i].header.sh_addralign;
			}
		}
	}

	// Finally, adjust the ELF header.
	const elf_section* last = elf->sections + (elf->header.e_shnum - 1);
	elf->header.e_shoff = last->header.sh_offset + last->header.sh_size;
	// Align.
	elf->header.e_shoff += 16 - (elf->header.e_shoff % 16);

	// Fix special program headers.
	for (u16 i = 0; i < elf->header.e_phnum; i++)
	{
		// PT_INERP
		if (elf->segments[i].header.p_type == 3)
		{
			elf->segments[i].header.p_offset = elf_section_get(elf, ".interp")->header.sh_offset;
		}
	}

}
