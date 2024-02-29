#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <args.h>
#include <about.h>

void args_parse(arguments* args, int32_t argc, const char** argv)
{
    // If no additional arguments are provided, just print the About text.
    if (argc == 1)
    {
        printf(SOLINK_ABOUT_TEXT);
        exit(0);
    }

    // Parse all arguments, but exclude first (self).
    for (int32_t i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
        {
            // Check if we have sufficient arguments.
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: %s is missing an argument!\n", argv[i]);
                exit(1);
            }
            args->output = argv[i + 1];
            i++;
        }
        else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--symbol"))
        {
            // Check if we have sufficient arguments.
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: %s is missing an argument!\n", argv[i]);
                exit(1);
            }

            i++;
        }
        else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--force"))
        {
            args->force = true;
        }
        else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet"))
        {
            args->quiet = true;
        }
        else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
        {
            args->version = true;
            break;
        }
        else if (!strcmp(argv[i], "--help"))
        {
            args->help = true;
            break;
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "Error: Unknown argument \"%s\"\n", argv[i]);
            exit(1);
        }
        else
        {
            // Initialize array. Size isn't exact here, but close to the amount we need.
            if (!args->libraries)
                args->libraries = calloc(sizeof(char*), argc - 2);
            
            // Check if the library file exists.
            if (!args_check_file(argv[argc - 1]))
            {
                fprintf(stderr, "Error: \"%s\": %s\n", argv[argc - 1], strerror(errno));
                exit(1);
            }

            args->libraries[args->num_libraries] = argv[i];
            args->num_libraries++;
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

    // Check if the input file exists.
    if (!args_check_file(argv[argc - 1]))
    {
        fprintf(stderr, "Error: \"%s\": %s\n", argv[argc - 1], strerror(errno));
        exit(1);
    }

    args->input = argv[argc - 1];

    // We need at least 1 library.
    if (args->num_libraries == 0)
    {
        fprintf(stderr, "Error: Need at least 1 library to link against!\n");
        exit(1);
    }
}

bool args_check_file(const char* path)
{
    FILE* file;
    file = fopen(path, "r");
    if (!file)
        return false;
    fclose(file);
    return true;
}
