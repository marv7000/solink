#include <stdint.h>
#include <stdio.h>

#include <elf.h>

int32_t main(const int32_t argc, const char** argv)
{
    struct elf_file file;
    elf_read(&file, argv[0]);

    // Print all section names.
    printf("Sections:\n");
    for (uint16_t i = 1; i < file.header.e_shnum; i++)
    {
        printf("%s\n", file.section_names[i]);
    }

    return 0;
}
