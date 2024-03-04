#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <args.h>
#include <about.h>

void args_parse(arguments* args, int32_t argc, str* argv)
{
    // If no additional arguments are provided, just print the About text.
    if (argc == 1)
    {
        printf(SOLINK_ABOUT_TEXT);
        exit(0);
    }

    // Initialize library array. Size isn't exact here, but close to the amount we need.
    if (!args->files)
        args->files = calloc(sizeof(str), argc);

    // Parse all arguments, but exclude first (self).
    for (int32_t i = 1; i < argc; i++)
    {
        if (str_equal_c(argv[i], "-o") || str_equal_c(argv[i], "--output"))
        {
            // Check if we have sufficient arguments.
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: %s is missing an argument!\n", str_cstr(argv[i]));
                exit(1);
            }
            args->output = argv[i + 1];
            i++;
        }
        else if (str_equal_c(argv[i], "-s")|| str_equal_c(argv[i], "--symbol"))
        {
            // Check if we have sufficient arguments.
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: %s is missing an argument!\n", str_cstr(argv[i]));
                exit(1);
            }

            i++;
        }
        else if (str_equal_c(argv[i], "-f") || str_equal_c(argv[i], "--force"))
        {
            args->force = true;
        }
        else if (str_equal_c(argv[i], "-q") || str_equal_c(argv[i], "--quiet"))
        {
            args->quiet = true;
        }
        else if (str_equal_c(argv[i], "-v") || str_equal_c(argv[i], "--version"))
        {
            args->version = true;
            break;
        }
        else if (str_equal_c(argv[i], "--help"))
        {
            args->help = true;
            break;
        }
        else if (str_cstr(argv[i])[0] == '-')
        {
            fprintf(stderr, "Error: Unknown argument \"%s\"\n", str_cstr(argv[i]));
            exit(1);
        }
        else
        {
            // Check if the file exists.
            if (!args_check_file(argv[i]))
            {
                fprintf(stderr, "Error: \"%s\": %s\n", str_cstr(argv[i]), strerror(errno));
                exit(1);
            }

            args->files[args->num_files] = argv[i];
            args->num_files++;
        }
    }

    if (args->help)
    {
        printf(SOLINK_ABOUT_TEXT);
        printf("\n");
        printf(SOLINK_HELP_TEXT);
        exit(0);
    }

    if (args->version)
    {
        printf(SOLINK_ABOUT_TEXT);
        exit(0);
    }

    // We need at least 1 library and 1 executable.
    if (args->num_files < 2)
    {
        fprintf(stderr, "Error: Need at least 1 library and exactly 1 executable to link!\n");
        exit(1);
    }

    // If no output file was given, append a suffix it to the input path.
    if (str_empty(args->output))
    {
        str suffix = str_new_text("_patch");
		str name = str_copy(args->files[args->num_files - 1]);
        str_concat(&name, suffix);
        str_free(suffix);
		args->output = name;
    }
}

bool args_check_file(str path)
{
    FILE* file;
    file = fopen(str_cstr(path), "r");
    if (!file)
        return false;
    fclose(file);
    return true;
}
