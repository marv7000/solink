#include <instr.h>
#include <stdio.h>

bool instr_get_bytes(elf_machine type, uint32_t offset, uint8_t instr[16])
{
    // TODO
    switch (type)
    {
        case EM_X86_64:
			instr[0] = 0xe9;
			*(uint32_t*)(instr + 1) = offset - 5;
		    instr[5] = 0x90;
		    instr[6] = 0x90;
		    instr[7] = 0x90;
		    instr[8] = 0x90;
		    instr[9] = 0x90;
		    instr[10] = 0x90;
		    instr[11] = 0x90;
		    instr[12] = 0x90;
		    instr[13] = 0x90;
		    instr[14] = 0x90;
		    instr[15] = 0x90;
            break;
        default:
            return false;
    }

    return true;
}
