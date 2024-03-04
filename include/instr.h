#pragma once
#include <stdint.h>
#include <stdbool.h>

#include <elf.h>

/// \brief          Gets the assembly instructions for a relative jump on the given machine and writes them to instr.
bool instr_get_bytes(elf_machine type, uint32_t offset, uint8_t instr[16]);
