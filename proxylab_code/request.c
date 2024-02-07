#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include "csapp.h"
#include "request.h"
#define LOGGER_ON
#include "logger.h"



int is_number(char* string,size_t len){
    int i;
    for(i=0;i<len;i++){
        if(!('0'<=string[i] && string[i] <='9')){
            return 0;
        }
    }
    return 1;
}

request_line *parse_http_request_line(char* buf,size_t dataSize){
    request_line* pReq = malloc(sizeof(request_line));
    uri* pURI = &pReq->URI;
    char szURI[sizeof(uri)];
    char szAddr[HOSTNAME_STR_MAXLEN+17];
    char szPort[8]={};
    int count;
    if(sscanf(buf,"%7s %2047s %15s%n",pReq->szMethod,szURI,pReq->szVersion,&count) == 3 &&
        sscanf(szURI,"%15[^://]://%79[^/]%1959s",pURI->szProtocol,szAddr,pURI->szPath) == 3 ){
        if(sscanf(szAddr,"%63[^:]:%7s",pURI->szHostname,szPort) == 2){
            if(count>=dataSize){
                warn("Request Line length is smaller than given dataSize.");
            }
            if(is_number(szPort,strlen(szPort))){
                int port=atoi(szPort);
                if(0<=port && port<=0xFFFF){
                    pURI->wPort=port;
                    info("Request line parsed successfully. length:%d",count);
                    return pReq;
                }
            }else{
                warn("Malformed uri detected.");
            }    
        }else{
            if(strcasecmp(pURI->szProtocol,"http")){
                pURI->wPort=80;
                return pReq;
            }else{
                warn("Unsupported protocol detected.");
            }
        }
    }else{
        warn("Malformed request line detected.");
    }
    error("Failed to parse request line.");
    free_request_line(pReq);
    return NULL;
}


void free_request_line(request_line *p){
    free(p);
}

request_header* add_reqhdr_entry(char* szKey,char* szValue,request_header* pLast){
     request_header* pNew=malloc(sizeof(request_header));
     reqhdr_entry* pEntry=&pNew->entry;
     strcpy(pEntry->szKey,szKey);
     strcpy(pEntry->szValue,szValue);
     pLast->pNext=pNew;
     pNew->pNext=NULL;
     return pNew;
}

request_header* parse_and_add_reqhdr_entry(char* szEntry,request_header* pLast){
    //若没有创建请求头结构体,则创建结构体并插入键值对,否则按照链表方式添加下一个请求头键值对
    request_header* pNew=malloc(sizeof(request_header));
    reqhdr_entry* pEntry=&pNew->entry;
    int c;
    if((c=sscanf(szEntry,"%127[^:]: %1023s",pEntry->szKey,pEntry->szValue))!=2){
        error("Failed to parse 1 entry of request header. sscanf returned %d",c);
        debug("szKey=%s szValue=%s",pEntry->szKey,pEntry->szValue);
        free(pNew);
        return NULL;
    }
    if(pLast!=NULL){
        pLast->pNext=pNew;
    }
    pNew->pNext=NULL;
    info("1 entry of request header parsed successfully.");
    return pNew;
}


void free_request_header(request_header* p){
    if(p!=NULL){
        request_header* pNow = p, *pNext;
        for(pNow=p;pNow!=NULL;pNow=pNext){
            pNext=pNow->pNext;
            free(pNow);
        }
    }
}