#pragma once
#include <stdint.h>
#include <stdbool.h>

#include <elf.h>

/// \brief          Gets the assembly instructions for the given machine and writes them to instr.
bool instr_get_bytes(elf_machine type, uint64_t offset, uint64_t* instr);
