#include <strings.h>
#include "csapp.h"
#include "request.h"
#include "client.h"
#define LOGGER_ON
#include "logger.h"



void modify_header_entries(request_line* pReqLine,request_header* pReqHdr){
    request_header* pNow,*pLast;
    char hasHost=0,hasUserAgent=0,hasConn=0,hasProxyConn=0;
    const char *szUserAgent = "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";
    for(pNow=pReqHdr;pNow!=NULL;pNow=pNow->pNext){
        debug("modifying header entry: %s: %s",pNow->entry.szKey,pNow->entry.szValue);
        if(hasHost==0 && strcasecmp(pNow->entry.szKey,"Host") == 0){
            hasHost=1;
        }else if(hasUserAgent==0 && strcasecmp(pNow->entry.szKey,"User-Agent") == 0){
            hasUserAgent=1;
            strcpy(pNow->entry.szValue,szUserAgent);
        }else if(hasConn==0 && strcasecmp(pNow->entry.szKey,"Connection") == 0){
            hasConn=1;
            strcpy(pNow->entry.szValue,"close");
        }else if(hasProxyConn==0 && strcasecmp(pNow->entry.szKey,"Proxy-Connection") == 0){
            hasProxyConn=1;
            strcpy(pNow->entry.szValue,"close");
        }
        pLast=pNow;
    }

    if(hasHost == 0){
        char* szHost;
        char buf[HOSTNAME_STR_MAXLEN+5+1];
        if(pReqLine->URI.wPort == 80){
            szHost = pReqLine->URI.szHostname;
        }else{
            sprintf(buf,"%s:%d",pReqLine->URI.szHostname,pReqLine->URI.wPort);
            szHost=buf;
        }
        pLast = add_reqhdr_entry("Host",szHost,pLast);
    }
    if(hasUserAgent == 0){
        pLast = add_reqhdr_entry("User-Agent",szUserAgent,pLast);
    }
    if(hasConn == 0){
        pLast = add_reqhdr_entry("Connection","close",pLast);
    }
    if(hasProxyConn == 0){
        add_reqhdr_entry("Proxy-Connection","close",pLast);
    }

    for(pNow=pReqHdr;pNow!=NULL;pNow=pNow->pNext){
        debug("final header entry: %s: %s",pNow->entry.szKey,pNow->entry.szValue);
    }
}

void send_request_string(int clientfd,request_line* pReqLine,request_header* pReqHeader){
    char buf[MAXLINE];
    char temp[MAXLINE];
    temp[0]=0;
    uri* pURI=&pReqLine->URI;
    sprintf(buf,"%s %s %s\r\n",pReqLine->szMethod,pURI->szPath,pReqLine->szVersion);
    strcat(temp,buf);
    Rio_writen(clientfd,buf,strlen(buf));
    request_header* pNow;
    for(pNow=pReqHeader;pNow!=NULL;pNow=pNow->pNext){
        sprintf(buf,"%s: %s\r\n",pNow->entry.szKey,pNow->entry.szValue);
        strcat(temp,buf);
        Rio_writen(clientfd,buf,strlen(buf));
    }
    strcat(temp,"\r\n");
    Rio_writen(clientfd,"\r\n",strlen(buf));
    debug("Sent request string:\n%s",temp);
}


int forward_request(request_line* pReqLine,request_header* pReqHdr){
    strcpy(pReqLine->szVersion,"HTTP/1.0");
    modify_header_entries(pReqLine,pReqHdr);
    char szPort[8];
    sprintf(szPort,"%d",pReqLine->URI.wPort);
    info("Try connecting: %s:%s",pReqLine->URI.szHostname,szPort);
    int clientfd=open_clientfd(pReqLine->URI.szHostname,szPort);
    if(clientfd<0){
        error("open_clientfd failed.");
    }else{
        info("open_clientfd success. clientfd=%d",clientfd);
    }
    send_request_string(clientfd,pReqLine,pReqHdr);

    return clientfd;
}