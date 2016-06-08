#include "log.h"
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include <sys/file.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#else
#include<io.h>
#include<direct.h>
#include<Windows.h>
#endif


#pragma warning(disable:4996)

#ifndef WIN32
__thread char buffer[_LOG_BUFFSIZE];
#else
__declspec(thread) char m_buffer[_LOG_BUFFSIZE];
#endif 
WW_LOG_XX::Logger::Logger()
{
	fp = NULL;
	system_level = LL_NOTICE;
}

WW_LOG_XX::Logger::~Logger()
{
	if (fp != NULL)
	{
		fflush(fp);
		fclose(fp);
		fp = NULL;
	}
}

bool WW_LOG_XX::Logger::log_init(LogLevel lev, const std::string p_LOGname, const std::string p_logdir)
{
	//判断路径是否存在，不存在，则创建路径
#ifndef WIN32
	if (access(p_logdir.c_str(), F_OK) == -1)
	{
		if (mkdir(p_logdir.c_str(), S_IREAD | S_IWRITE) < 0)
		{
			fprintf(stderr, "create folder failed\n");
			return false;
		}
	}
#else
	if (access(p_logdir, 0) == -1)
	{
		if (mkdir(p_logdir) < 0)
		{
			fprintf(stderr, "create folder failed\n");
			return false;
		}
	}
#endif 
	std::string filelocation;
	filelocation = p_logdir + p_LOGname;
	if (filelocation.size() > _LOG_PATH_LEN)
	{
		fprintf(stderr, "the path of log file is too long:%d limit:%d\n", filelocation.size(), _LOG_PATH_LEN);
		return false;
	}
	if (NULL != fp)
	{
		return false;
	}
	fp = fopen(filelocation.c_str(), "a+");
	if (fp == NULL)
	{
		fprintf(stderr, "cannot open log file,file location is %s\n", filelocation);
		return false;
	}
	setvbuf(fp, (char *)NULL, _IOLBF, 1024 * 1024 * 10);
	return true;
}

bool WW_LOG_XX::Logger::log(LogLevel lev, std::string logformat, ...)
{
	if (!checklevel(lev))
		return false;
	int size;
	int prestrlen = 0;

	char * start = buffer;
	prestrlen = getTime(start, lev);
	start += prestrlen;

	va_list args;
	va_start(args, logformat);
	size = vsnprintf(start, _LOG_BUFFSIZE - prestrlen, logformat.c_str(), args);
	va_end(args);

	if (NULL == fp)
		fprintf(stderr, "%s", buffer);
	else
	{
		fwrite(buffer, prestrlen + size, 1, fp);
		*buffer = '\0';
	}	
	return true;
}

WW_LOG_XX::LogLevel WW_LOG_XX::Logger::get_level()
{
	return system_level;
}

WW_LOG_XX::LogLevel WW_LOG_XX::Logger::set_level(LogLevel lev)
{
	system_level=lev;
}





const char * WW_LOG_XX::Logger::logLevelToString(LogLevel lev)
{
	switch (lev)
	{
	case LL_DEBUG:
		return "DEBUG";
	case LL_TRACE:
		return "TRACE";
	case LL_NOTICE:
		return "NOTICE";
	case LL_WARNING:
		return "WARN";
	case LL_ERROR:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

bool WW_LOG_XX::Logger::checklevel(LogLevel lev)
{
	if (lev >= system_level)
		return true;
	else
		return false;
}

int WW_LOG_XX::Logger::getTime(char * m_buffer, LogLevel lev)
{
	time_t nowtime;
	nowtime = time(NULL);;
	localtime(&nowtime);
	char tmp[64] = { 0 };
	strftime(tmp, sizeof(tmp), "%Y/%m/%d %X", localtime(&nowtime));
#ifndef WIN32
	struct timeval val;
	gettimeofday(&val, NULL);
	return snprintf(m_buffer, _LOG_BUFFSIZE, "%s: %s(%6.3d)", logLevelToString(lev), tmp, val.tv_usec);
#else
	SYSTEMTIME time;
	GetLocalTime(&time);
	return snprintf(m_buffer, _LOG_BUFFSIZE, "%s: %s(%d) ", logLevelToString(lev), tmp, time.wMilliseconds);
#endif
	return 0;
}
