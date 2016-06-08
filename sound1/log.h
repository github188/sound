#pragma once
#include<string>



namespace WW_LOG_XX
{
	/* log 文件字符串路径最大长度*/
#define	  _LOG_PATH_LEN  250
	/* 每个线程的buffer size*/
#define   _LOG_BUFFSIZE  1024*500

	typedef  enum LogLevel {
		LL_DEBUG = 1,
		LL_TRACE = 2,
		LL_NOTICE = 3,
		LL_WARNING = 4,
		LL_ERROR = 5,
	}LogLevel;

	class Logger
	{
	public:
		Logger();
		~Logger();
		bool log_init(LogLevel lev, const std::string p_LOGname, const std::string p_logdir);
		bool log(LogLevel lev, std::string logformat, ...);

		LogLevel get_level();
		LogLevel set_level(LogLevel lev);
	private:
		const char* logLevelToString(LogLevel lev);
		bool checklevel(LogLevel lev);
		int getTime(char* m_buffer, LogLevel lev);
	private:
		FILE* fp;
		LogLevel system_level;
	};

#define WW_LOG_ERROR(ww_logger,log_fmt, ...) \
    do{ \
        ww_logger.log(WW_LOG_XX::LL_ERROR,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

#define WW_LOG_WARN(ww_logger,log_fmt, ...) \
    do{ \
        ww_logger.log(WW_LOG_XX::LL_WARNING,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

#define WW_LOG_NOTICE(ww_logger,log_fmt, ...) \
    do{ \
        ww_logger.log(WW_LOG_XX::LL_NOTICE,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

#define WW_LOG_TRACE(ww_logger,log_fmt, ...) \
    do{ \
        ww_logger.log(WW_LOG_XX::LL_TRACE,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 

#define WW_LOG_DEBUG(ww_logger,log_fmt, ...) \
    do{ \
        ww_logger.log(WW_LOG_XX::LL_DEBUG,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0) 
}