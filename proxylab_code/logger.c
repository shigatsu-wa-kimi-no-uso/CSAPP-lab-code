#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <libgen.h>
#include "logger.h"

FILE* logfd;

const char* get_time_string(time_t timestamp,char* buffer)
{
	struct tm* ttime = localtime(&timestamp);
	sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d",
		1900 + ttime->tm_year,
		1 + ttime->tm_mon,
		ttime->tm_mday,
		ttime->tm_hour,
		ttime->tm_min,
		ttime->tm_sec);
	return buffer;
}


void init_logger(const char* szFilePath){
    logfd = fopen(szFilePath,"a+");
}

void close_logger(){
    if(logfd!=NULL){
        fflush(logfd);
        fclose(logfd);
    }
}

void write_log(enum log_type type,const char* szFile,const char* szFunc,const int szLine,const char* szLogFmt,...){
    static char buf[LOGGER_BUF_SIZE];
    static const char* szTypeName[] = {"DEBUG","INFO","WARN","ERROR","TRACE"};
    static int count = 0;
    char timestr[64];
    char szPrefix[256];
    char szTrace[256];
    va_list arglist;
	va_start(arglist, szLogFmt);
    sprintf(szTrace,"%s @ %s (Line %d)",basename(szFile),szFunc,szLine);
    sprintf(szPrefix, "%3d %-4s %s|%-25s:",count++, szTypeName[(int)type], get_time_string(time(0), timestr),szTrace);
    vsprintf(buf,szLogFmt,arglist);
    fprintf(logfd,"%s%s\n",szPrefix,buf);
    fflush(logfd);
}

