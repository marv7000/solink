#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <args.h>
#include <elf.h>
#include <patch.h>

int32_t main(const int32_t argc, const char** argv)
{
    int32_t ret = 0;

    // Parse arguments.
    arguments args = {0};
    str* args_str = (str*)malloc(sizeof(str) * argc);
    for (int32_t i = 0; i < argc; i++)
        args_str[i] = str_new_text(argv[i]);
    args_parse(&args, argc, args_str);

    // Open all libraries.
    elf_file* libs = (elf_file*)malloc(args.num_files * sizeof(elf_file));
    for (uint16_t i = 0; i < args.num_files; i++)
    {
        elf_error read = elf_read(args.files[i], &libs[i]);
        if (read != ELF_OK)
        {
            fprintf(stderr, "Error: Failed to read the ELF \"%s\" from file!\n", str_cstr(args.files[i]));
            fprintf(stderr, "       %s\n", elf_error_str(read));
            ret = 1;
            goto exit;
        }
    }

    const uint64_t last = args.num_files - 1;

    // Print library symbols.
    if (!args.quiet)
    {
        str* names;
        uint64_t num_names;
        if (!patch_match_symbols(&libs[last], libs, last, &names, &num_names))
        {
            fprintf(stderr, "Error: Failed to match symbols!\n");
            ret = 1;
            goto exit;
        }
        printf("Linking %lu symbol%s:\n", num_names, num_names == 1 ? "" : "s");
        for (uint64_t sym = 0; sym < num_names; sym++)
            printf("[%lu]\t%s\n", sym, str_cstr(names[sym]));
    }

    // Get the output ELF.
    elf_file* target = libs + last;

    // Patch input executable with all libraries.
    for (uint16_t i = 0; i < last; i++)
    {
        if (!patch_link_library(target, &libs[i]) != ELF_OK)
        {
            fprintf(stderr, "Error: Failed to link against \"%s\"!\n", str_cstr(args.files[i]));
            ret = 1;
            goto exit;
        }
    }

    // Write the result to file.
    elf_error written = elf_write(args.output, target);
    if (written != ELF_OK)
    {
        fprintf(stderr, "Error: Failed to write the ELF to \"%s\"!\n", str_cstr(args.output));
        fprintf(stderr, "       %s\n", elf_error_str(written));
        ret = 1;
        goto exit;
    }
    if (!args.quiet)
        printf("Wrote the patched binary to \"%s\".\n", str_cstr(args.output));

    // Print a warning if symbols didn't match.
    if (!args.quiet)
    {
        // TODO
    }

exit:
    free(libs);
    return ret;
}
