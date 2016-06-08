#pragma once
#include <stdio.h>
#include <mutex>
#include <string>

/* 每个线程的buffer size*/
#define   _LOG_BUFFSIZE  1024*1024*8
/* 当前进程的 Stream IO buffer size*/
#define   _SYS_BUFFSIZE  1024*1024*10
/* log 文件字符串路径最大长度*/
#define	  _LOG_PATH_LEN  250
/* 日志对应的模块名*/
#define   _LOG_MODULE_LEN 32



namespace _WW_Log_XX
{
	typedef  enum LogLevel {
		LL_DEBUG = 1,
		LL_TRACE = 2,
		LL_NOTICE = 3,
		LL_WARNING = 4,
		LL_ERROR = 5,
	}LogLevel;

	/**
	*	Log_Writer  日志类
	*/
	class Logger
	{
	public:
		Logger();
		~Logger();
		bool loginit(LogLevel l, const  char *filelocation, bool append = true, bool issync = false);
		bool log(LogLevel l, std::string logformat, ...);
		LogLevel get_level();
		bool logclose();
	private:
		const char* logLevelToString(LogLevel l);
		bool checklevel(LogLevel l);
		int premakestr(char* m_buffer, LogLevel l);
		bool writeLog(char *_pbuffer, int len);
	private:
		enum LogLevel m_system_level;
		FILE* fp;
		bool m_issync;
		bool m_isappend;
		char m_filelocation[_LOG_PATH_LEN];
		std::mutex mutex_;
#ifndef _WIN32
		static __thread char m_buffer[_LOG_BUFFSIZE];
#else
		static __declspec(thread) char m_buffer[_LOG_BUFFSIZE];
#endif 
		//The __thread specifier may be applied to any global, file-scoped static, function-scoped static, 
		//or static data member of a class. It may not be applied to block-scoped automatic or non-static data member
		//in the log  scence,It's safe!!!!
		//一言以蔽之，此场景不用担心__thread带来资源leak,同时也不用担心多个Log_Writer会干扰，
		//因为一个线程同一时间只有一个Log_Writer在干活，干完之后m_buffer就reset了
		//所以即便一个线程用户多个Log_Write串行(因为一个线程内的运行态只有串行) 也是线程安全的！！！
	};

	/**
	* LogLevel 日志级别
	* p_LOGname 如mysql
	* p_logdir  日志输出目录
	* */
	bool log_init(LogLevel l, const char* p_LOGname, const char* p_logdir);
	//============basic===================
}

extern _WW_Log_XX::Logger ww_logger;
#define WW_LOG_ERROR(log_fmt, ...) \
    do{ \
        ww_logger.log(_WW_Log_XX::LL_ERROR,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

#define WW_LOG_WARN(log_fmt, ...) \
    do{ \
        ww_logger.log(_WW_Log_XX::LL_WARNING,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

#define WW_LOG_NOTICE(log_fmt, ...) \
    do{ \
        ww_logger.log(_WW_Log_XX::LL_NOTICE,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

#define WW_LOG_TRACE(log_fmt, ...) \
    do{ \
        ww_logger.log(_WW_Log_XX::LL_TRACE,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

#define WW_LOG_DEBUG(log_fmt, ...) \
    do{ \
        ww_logger.log(_WW_Log_XX::LL_DEBUG,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

//============extend===================
#define WW_MACRO_RET(condition, return_val) {\
    if (condition) {\
        return return_val;\
    }\
}

#define WW_MACRO_WARN(condition, log_fmt, ...) {\
    if (condition) {\
        LOG_WARN( log_fmt, ##__VA_ARGS__);\
    }\
}

#define WW_MACRO_WARN_RET(condition, return_val, log_fmt, ...) {\
    if ((condition)) {\
        LOG_WARN( log_fmt, ##__VA_ARGS__);\
		return return_val;\
    }\}
