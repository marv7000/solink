#pragma once

#define SOLINK_ABOUT_TEXT "solink " SOLINK_VER_MAJ "." SOLINK_VER_MIN " (" SOLINK_VER_PATCH ")\n"
#define SOLINK_HELP_TEXT "Usage: solink [flags] [lib(s)] <target>\n" \
	"Flags:\n" \
	"\t-o, --output <file path> Save the resulting binary at the given location.\n" \
	"\t-s, --symbol <symbol>    Only match the given symbol.\n" \
	"\t-f, --force              Forcefully match all external symbols.\n" \
	"\t-q, --quiet              Don't write any messages to the standard output.\n" \
	"\t--relax                  Don't write any warnings to the standard output.\n" \
	"\t-v, --version            Write the version to standard output.\n" \
	"\t--help                   Display this message with all flags and their usage.\n"
