#include <instr.h>

bool instr_get_bytes(elf_machine type, uint64_t offset, uint64_t* instr)
{
    // TODO
    switch (type)
    {
        case EM_X86_64:
            break;
        default:
            return false;
    }

    return true;
}
