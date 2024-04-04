#include <string.h>

#include <instr.h>

bool instr_get_bytes(elf_machine type, u32 offset, u8 instr[16])
{
    // TODO
    switch (type)
    {
        case EM_X86_64:
			offset -= 5; // sizeof(jmp) == 5

			instr[0] = 0xe9; // jmp
			memmove(instr + 1, &offset, sizeof(u32));
			// Padding, but make sure we never get past this point.
		    instr[5] = 0x90; // nop
		    instr[6] = 0x90;
		    instr[7] = 0x90;
		    instr[8] = 0x90;
		    instr[9] = 0x90;
		    instr[10] = 0x90;
		    instr[11] = 0x90;
		    instr[12] = 0x90;
		    instr[13] = 0x90;
		    instr[14] = 0x90;
		    instr[15] = 0xf4; // hlt
            break;
        default:
            return false;
    }

    return true;
}
