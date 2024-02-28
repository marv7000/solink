#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <args.h>
#include <about.h>

#define ARGS_CHECK(short_name, long_name) 

void args_parse(arguments* args, int32_t argc, const char** argv)
{
    // If no arguments are provided, just print the About text.
    if (argc == 1)
    {
        printf(SOLINK_ABOUT_TEXT);
        exit(0);
    }


    // Set default values.
    args->output = "";

    // Parse all arguments.
    for (int32_t i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
        {
            // Check if we have sufficient arguments.
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: %s is missing an argument!", argv[i]);
                exit(1);
            }
            // Check if the file exists.
            if (!args_check_file(argv[i + 1]))
            {
                fprintf(stderr, "Error: ");
                exit(1);
            }

            // If everything else passes, set the file and skip the next arg.
            args->output = argv[i + 1];
            i++;
        }
        if (!strcmp(argv[i], "--help"))
        {
            printf(SOLINK_ABOUT_TEXT);
            printf("\n");
            printf(SOLINK_HELP_TEXT);
            exit(0);
        }
    }

    // We need at least 3 arguments (self, library, target).
    if (argc <= 2)
    {
        fprintf(stderr, "Error: Need at least one library to link against!");
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
