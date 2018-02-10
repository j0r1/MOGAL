#include "log.h"
#include <time.h>
#include <stdarg.h>
#include <stdio.h>

#define MAXMSGLEN	4096

static int debugLevel = 0;

void setDebugLevel(int l)
{
	debugLevel = l;
}

void writeLog(int level, const char *fmt, ...)
{
	if (level > debugLevel)
		return;

	static char str[MAXMSGLEN];
	static char *pLevelString[4]={"     ",
		                      "ERROR",
				      "INFO ",
				      "DEBUG"};
	
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(str,MAXMSGLEN,fmt,ap);
	va_end(ap);
	str[MAXMSGLEN-1] = 0;

	time_t t = time(0);
	struct tm *lt = localtime(&t);

	printf("[%02d:%02d:%02d %04d/%02d/%02d] %s %s\n", lt->tm_hour, lt->tm_min, lt->tm_sec, 
			                               1900+lt->tm_year, lt->tm_mon, lt->tm_mday,
						       pLevelString[level], str);
}

