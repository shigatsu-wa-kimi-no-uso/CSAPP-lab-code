#include <stdio.h>
#include "server.h"
#define LOGGER_ON
#include "logger.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *szSuggestedPort = "37302";


int main(int argc,char** argv)
{
    char* szPort;
    /* Check command line args */
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }else{
        szPort = argv[1];
    }
    init_logger("proxy.log");
    info("---------proxy server started---------");
    start(szPort);
    info("proxy server exited normally.");
    close_logger();
    return 0;
}