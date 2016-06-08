#include "log.h"
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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
_WW_Log_XX::Logger ww_logger;

namespace _WW_Log_XX
{
#ifndef _WIN32
	__thread char Logger::m_buffer[_LOG_BUFFSIZE];
#else
	__declspec(thread) char Logger::m_buffer[_LOG_BUFFSIZE];
#endif 
	bool log_init(LogLevel lev, const char* p_LOGname, const char* p_logdir)
	{
		//如果路径存在文件夹，则判断是否存在
#ifndef WIN32
		if (access(p_logdir, F_OK) == -1)
		{
			if (mkdir(p_logdir, S_IREAD | S_IWRITE) < 0)
				fprintf(stderr, "create folder failed\n");
		}
#else
		if (access(p_logdir,0 ) == -1)
		{
			if (mkdir(p_logdir) < 0)
				fprintf(stderr, "create folder failed\n");
		}
#endif 
		
		char _location_str[_LOG_PATH_LEN];
		snprintf(_location_str, _LOG_PATH_LEN, "%s/%s.log", p_logdir, p_LOGname);
		ww_logger.loginit(lev, _location_str);
		return true;
	}

	const char* Logger::logLevelToString(LogLevel lev) {
		switch (lev) {
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

	bool Logger::checklevel(LogLevel lev)
	{
		if (lev >= m_system_level)
			return true;
		else
			return false;
	}

	Logger::Logger()
	{
		m_system_level = LL_NOTICE;
		//fp = stderr;
		fp = NULL;
		m_issync = false;
		m_isappend = true;
		m_filelocation[0] = '\0';
	}

	Logger::~Logger()
	{
		logclose();
	}

	bool Logger::loginit(LogLevel lev, const  char *filelocation, bool append, bool issync)
	{
		WW_MACRO_RET(NULL != fp, false);
		m_system_level = lev;
		m_isappend = append;
		m_issync = issync;
		if (strlen(filelocation) >= (sizeof(m_filelocation) - 1))
		{
			fprintf(stderr, "the path of log file is too long:%d limit:%d\n", strlen(filelocation), sizeof(m_filelocation) - 1);
			exit(0);
		}
		//本地存储filelocation  以防止在栈上的非法调用调用
		strncpy(m_filelocation, filelocation, sizeof(m_filelocation));
		m_filelocation[sizeof(m_filelocation) - 1] = '\0';

		if ('\0' == m_filelocation[0])
		{
			fp = stdout;
			fprintf(stderr, "now all the running-information are going to put to stderr\n");
			return true;
		}

		fp = fopen(m_filelocation, append ? "a" : "w");
		if (fp == NULL)
		{
			fprintf(stderr, "cannot open log file,file location is %s\n", m_filelocation);
			exit(0);
		}
		//setvbuf (fp, io_cached_buf, _IOLBF, sizeof(io_cached_buf)); //buf set _IONBF  _IOLBF  _IOFBF
		setvbuf(fp, (char *)NULL, _IOLBF, 1024*1024*10);
		fprintf(stderr, "now all the running-information are going to the file %s\n", m_filelocation);
		return true;
	}

	int Logger::premakestr(char* m_buffer, LogLevel lev)
	{
		time_t nowtime;
		nowtime = time(NULL);;
		localtime(&nowtime);
		char tmp[64] = { 0 };
		//char tmp1[64] = { 0 };
		strftime(tmp, sizeof(tmp), "%Y/%m/%d %X", localtime(&nowtime));
#ifndef _WIN32
		struct timeval val;
		gettimeofday(&val,NULL);
		return snprintf(m_buffer, _LOG_BUFFSIZE, "%s: %s(%6.3d)", logLevelToString(lev), tmp,val.tv_usec);
#else
		SYSTEMTIME time;
		GetLocalTime(&time);
		return snprintf(m_buffer, _LOG_BUFFSIZE, "%s: %s(%d) ", logLevelToString(lev), tmp, time.wMilliseconds);
#endif
		
	}

	bool Logger::log(LogLevel lev, std::string logformat, ...)
	{
		WW_MACRO_RET(!checklevel(lev), false);
		int size;
		int prestrlen = 0;

		char * star = m_buffer;
		prestrlen = premakestr(star, lev);
		star += prestrlen;

		va_list args;
		va_start(args, logformat);
		size = vsnprintf(star, _LOG_BUFFSIZE - prestrlen, logformat.c_str(), args);
		va_end(args);

		if (NULL == fp)
			fprintf(stderr, "%s", m_buffer);
		else
			writeLog(m_buffer, prestrlen + size);
		return true;
	}

	bool Logger::writeLog(char *_pbuffer, int len)
	{
#ifndef WIN32
		if (0 != access(m_filelocation, W_OK))
		{
			std::lock_guard<std::mutex> lock(mutex_);
			//锁内校验 access 看是否在等待锁过程中被其他线程loginit了  避免多线程多次close 和init
			if (0 != access(m_filelocation, W_OK))
			{
				logclose();
				loginit(m_system_level, m_filelocation, m_isappend, m_issync);
			}
		}
#else
		if (0 != access(m_filelocation, 2)) //WRITE permission
		{
			std::lock_guard<std::mutex> lock(mutex_);
			//锁内校验 access 看是否在等待锁过程中被其他线程loginit了  避免多线程多次close 和init
			if (0 != access(m_filelocation, 2))
			{
				logclose();
				loginit(m_system_level, m_filelocation, m_isappend, m_issync);
			}
		}
#endif

		if (1 == fwrite(_pbuffer, len, 1, fp)) //only write 1 item
		{
			if (m_issync)
				fflush(fp);
			*_pbuffer = '\0';
		}
		else
		{
			fprintf(stderr, "Failed to write to logfile. errno:%s    message:%s", strerror(errno), _pbuffer);
			return false;
		}
		return true;
	}

	LogLevel Logger::get_level()
	{
		return m_system_level;
	}

	bool Logger::logclose()
	{
		if (fp == NULL)
			return false;
		fflush(fp);
		fclose(fp);
		fp = NULL;
		return true;
	}
}