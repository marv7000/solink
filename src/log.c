#include <log.h>
#include <stdlib.h>
#include <stdio.h>

bool log_quiet = false;
bool log_warn = true;

bool log_msg(log_level level, const str fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	FILE* f = stdout;
	str log = _ERR;
	str col = _BOLD _RED;
	// Should we produce any output?
	bool talk = !log_quiet;

	switch (level)
	{
		case LOG_INFO:
			log = _INFO;
			col = _REGULAR;
			break;
		case LOG_WARN:
			log = _WARN;
			col = _YELLOW;
			talk &= log_warn;
			break;
		default:
			f = stderr;
			break;
	}
	if (talk)
	{
		fprintf(f, "%s%s", col, log);
		vfprintf(f, fmt, args);
	}
	va_end(args);

	if (level > 0)
		exit(level);

	return false;
}
