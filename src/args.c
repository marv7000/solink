#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>

#include <args.h>
#include <about.h>
#include <log.h>

arguments ARGS = {0};

bool args_check_file(str path)
{
	FILE* file;
	file = fopen(path, "r");
	if (!file)
		log_msg(LOG_ERR, "\"%s\": %s\n", path, strerror(errno));
	fclose(file);
	return true;
}

void args_parse(i32 argc, str* argv)
{
	// If no additional arguments are provided, just print the About text.
	if (argc == 1)
	{
		log_msg(LOG_INFO, SOLINK_ABOUT_TEXT);
		exit(0);
	}

	// Initialize file vector. Size isn't exact here, but close to the amount we need.
	size files_cap = 8;
	if (!ARGS.files)
		ARGS.files = calloc(files_cap, sizeof(str));

	// Parse all arguments, but exclude first (self).
	for (i32 i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
		{
			// Check if we have sufficient arguments.
			if (i + 1 >= argc)
				log_msg(LOG_ERR, "%s is missing an argument!\n", argv[i]);
			// Touch the file.
			fclose(fopen(argv[i + 1], "w"));
			ARGS.output = realpath(argv[i + 1], NULL);
			i++;
		}
		else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--force"))
			ARGS.force = true;
		else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet"))
			log_quiet = true;
		else if (!strcmp(argv[i], "--relax"))
			log_warn = false;
		else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
		{
			log_msg(LOG_INFO, SOLINK_ABOUT_TEXT);
			exit(0);
		}
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
		{
			log_msg(LOG_INFO, SOLINK_ABOUT_TEXT);
			log_msg(LOG_INFO, SOLINK_HELP_TEXT);
			exit(0);
		}
		else if (argv[i][0] == '-')
			log_msg(LOG_ERR, "unknown argument \"%s\"\n", argv[i]);
		else
		{
			// Check if the file exists.
			args_check_file(argv[i]);

			ARGS.files[ARGS.num_files] = realpath(argv[i], NULL);
			ARGS.num_files++;
			if (ARGS.num_files > files_cap)
			{
				files_cap *= 2;
				ARGS.files = reallocarray(ARGS.files, files_cap, sizeof(str));
			}
		}
	}

	// We need at least 1 library and 1 executable.
	if (ARGS.num_files < 2)
		log_msg(LOG_ERR, "need at least 2 files to link.\n");

	// If no output file was given, use "a.out" as a default.
	if (!ARGS.output)
		ARGS.output = "a.out";

	// We can't link to ourselves, that won't do anything.
	for (u16 f = 0; f < ARGS.num_files - 1; f++)
	{
		if (!strcmp(ARGS.files[f], ARGS.files[ARGS.num_files - 1]))
			log_msg(LOG_ERR, "can't use \"%s\" as target and library!\n", basename(ARGS.files[f]));
	}
}
