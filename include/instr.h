#pragma once
#include <elf.h>

bool instr_get_bytes(elf_machine type, uint64_t* buf);
