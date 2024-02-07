#ifndef __LOGGER_H__
#define __LOGGER_H__
#define LOGGER_BUF_SIZE 1048576

enum log_type{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_TRACE
};


void init_logger(const char* szFilePath);
void close_logger();
#ifdef LOGGER_ON
void write_log(enum log_type type,const char* szFile,const char* szFunc,const int szLine,const char* szLogFmt,...);
#define info(format,args...) write_log(LOG_INFO,__FILE__,__FUNCTION__,__LINE__,format,##args)
#define error(format,args...) write_log(LOG_ERROR,__FILE__,__FUNCTION__,__LINE__,format,##args)
#define warn(format,args...) write_log(LOG_WARN, __FILE__,__FUNCTION__,__LINE__,format,##args)
#define debug(format,args...) write_log(LOG_DEBUG,__FILE__,__FUNCTION__,__LINE__,format,##args)
#define trace write_log(LOG_TRACE, "\nIn file %s\nLine: %d\nFunction:%s\n", __FILE__, __LINE__, __FUNCTION__)
#else
#define info(format,args...) ((void)0)
#define error(format,args...) ((void)0)
#define warn(format,args...) ((void)0)
#define debug(format,args...) ((void)0)
#define trace(format,args...) ((void)0)
#endif

#endif