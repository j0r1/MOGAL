#ifndef LOG_H

#define LOG_H

#define LOG_DEBUG	3
#define LOG_INFO	2
#define LOG_ERROR	1

void setDebugLevel(int level);
void writeLog(int level, const char *fmt, ...);

#endif // LOG_H

