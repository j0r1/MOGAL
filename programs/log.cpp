/*
    
  This file is a part of MOGAL, a Multi-Objective Genetic Algorithm
  Library.
  
  Copyright (C) 2008-2012 Jori Liesenborgs

  Contact: jori.liesenborgs@gmail.com

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  
  USA

*/

#include "mogalconfig.h"
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

int getDebugLevel()
{
	return debugLevel;
}

void writeLog(int level, const char *fmt, ...)
{
	if (level > debugLevel)
		return;

	static char str[MAXMSGLEN];
	static const char *pLevelString[4]={"     ",
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

void writeLog(int level, const char *fmt, va_list ap)
{
	if (level > debugLevel)
		return;

	static char str[MAXMSGLEN];
	static const char *pLevelString[4]={"     ",
		                      "ERROR",
				      "INFO ",
				      "DEBUG"};
	
	vsnprintf(str,MAXMSGLEN,fmt,ap);
	str[MAXMSGLEN-1] = 0;

	time_t t = time(0);
	struct tm *lt = localtime(&t);

	printf("[%02d:%02d:%02d %04d/%02d/%02d] %s %s\n", lt->tm_hour, lt->tm_min, lt->tm_sec, 
			                               1900+lt->tm_year, lt->tm_mon, lt->tm_mday,
						       pLevelString[level], str);
}


