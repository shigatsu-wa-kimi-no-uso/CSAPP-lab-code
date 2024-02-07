#include <stdio.h>
#include "csapp.h"
#include "server.h"
#include "request.h"
#include "client.h"
#include "cache.h"
#define LOGGER_ON
#include "logger.h"

/*
HTTP请求基本格式:
GET http://hostname:port/abc/def HTTP/1.1\r\n
Host: hostname:port\r\n
User-Agent: curl/7.68.0\r\n
...
key: value\r\n
\r\n
[raw data]...
*/

conn_queue connQueue;
cache respCache;

void init_connection_queue(conn_queue* p){
    p->front=0;
    p->rear=0;
    Sem_init(&p->mutex,0,1);
    Sem_init(&p->slot_cnt,0,QUEUE_CAPACITY);
    Sem_init(&p->item_cnt,0,0);
}


void add_new_connection(conn_queue* p,int connfd){
    P(&p->slot_cnt); //take
    P(&p->mutex);
    p->connfds[p->rear] = connfd;
    info("New connfd %d added to queue at index %d",connfd,p->rear);
    p->rear = (p->rear+1)%QUEUE_CAPACITY;
    V(&p->mutex);
    V(&p->item_cnt); //put
}

int get_new_connection(conn_queue* p){
    P(&p->item_cnt); //take
    P(&p->mutex);
    int connfd = p->connfds[p->front];
    info("Connfd %d popped from queue with index %d.",connfd,p->front);
    p->front = (p->front+1)%QUEUE_CAPACITY;
    V(&p->mutex);
    V(&p->slot_cnt); //put
    return connfd;
}

void doit(int fd);

void* service_thread(void* argp){
    pthread_t tid = pthread_self();
    Pthread_detach(tid);
    while(1){
        int connfd = get_new_connection(&connQueue);
        info("Connection %d taken over by thread %u",connfd,tid);
        doit(connfd);
        Close(connfd);
        info("Thread %u finished serving connection %d",tid,connfd);
    }
}

void start(char* szPort){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE], port[MAXLINE];
    listenfd = Open_listenfd(szPort);
#define THREAD_CNT 16
    int i;
    pthread_t tid;
    init_connection_queue(&connQueue);
    init_cache(&respCache);
    for(i=0;i<THREAD_CNT;i++){
        Pthread_create(&tid,NULL,service_thread,NULL);
        info("Thread created. tid=%u",tid);
    }

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        info("Accepted connection from (%s, %s)", hostname, port);      
        add_new_connection(&connQueue,connfd);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}


request_line* read_request_line(rio_t *rp)
{
    char buf[REQUEST_LINE_STR_MAXLEN];
    if (!Rio_readlineb(rp, buf, REQUEST_LINE_STR_MAXLEN)){ 
        return NULL;
    }
    request_line* pReqLine=parse_http_request_line(buf,strlen(buf));
    if(pReqLine == NULL){
        return NULL;
    }
    uri* pURI=&pReqLine->URI;
    debug("\nParse result:\nmethod=%s\nprotocol=%s\nhostname=%s\nport=%u\npath=%s\nversion=%s",
        pReqLine->szMethod,
        pURI->szProtocol,
        pURI->szHostname,
        pURI->wPort,
        pURI->szPath,
        pReqLine->szVersion
    );
    return pReqLine;
}

request_header* read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];
    char szResult[MAXLINE+MAXLINE/2+MAXLINE/4+MAXLINE/8+MAXLINE/16];

    request_header* pReqHdr, *pLast, *pNow;
    Rio_readlineb(rp, buf, MAXLINE);
    if(strcmp(buf, "\r\n") == 0){  //当无请求头时(请求行与请求头之间只有一个"\r\n")
        warn("No request header entry but terminator \"\\n\\r\" read.");
        return NULL;
    }
    pReqHdr = parse_and_add_reqhdr_entry(buf,NULL);
    pLast = pReqHdr;
    while(TRUE) {          
	    Rio_readlineb(rp, buf, MAXLINE);
        if(strcmp(buf, "\r\n") == 0){  
            info("Request header terminator \"\\n\\r\" parsed.");
            break;
        }
        pLast = parse_and_add_reqhdr_entry(buf,pLast);
        if(pLast == NULL){
            warn("Malformed request header detected.");
            return NULL;
        }
    }
    sprintf(szResult,"\nParse result:\n");
    for(pNow=pReqHdr;pNow!=NULL;pNow=pNow->pNext){
        sprintf(szResult,"%skey=%s value=%s\n",szResult,pNow->entry.szKey,pNow->entry.szValue);
    }
    debug(szResult);
    return pReqHdr;
}

size_t output_response_from_remote(int inputfd,int outputfd,char* respBuf,size_t respBufLen){
    int n;
    size_t len = 0;
    rio_t rio;
    char buf[MAXLINE];
    rio_readinitb(&rio,inputfd);
    while ((n = Rio_readnb(&rio, buf, sizeof(buf))) > 0) {
        if(len + n <= respBufLen){
            memncat(respBuf,len, buf, n * sizeof(char));    //忽略'\0'而复制指定大小的数据到buf
        }
        Rio_writen(outputfd, buf, n * sizeof(char));
        len += n;
    }
    debug("reponse output(show first 1024 bytes most):\n%.1024s",respBuf);
    return len;
}


void output_response_from_buffer(int outputfd,char* resp,size_t respLen){
    Rio_writen(outputfd, resp, respLen * sizeof(char));
    debug("reponse output(show first 1024 bytes most):\n%.1024s",resp);
}

void doit(int fd) 
{
    int is_static;
    char buf[MAXLINE];
    char respBuf[MAX_OBJECT_SIZE];
    char szKey[REQUEST_LINE_STR_MAXLEN];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd); //初始化rio内部buf
    request_line* pReqLine = read_request_line(&rio);
    if(pReqLine == NULL){
        warn("No valid request line.");
        return;
    }
    debug("Following request header:");
    request_header* pReqHeader = read_requesthdrs(&rio);
    if(pReqHeader == NULL){
        warn("No valid request header.");
    }
    sprintf(szKey,"%s %s:%d%s",pReqLine->szMethod,pReqLine->URI.szHostname,pReqLine->URI.wPort,pReqLine->URI.szPath);
    info("szKey=%s",szKey);
    size_t respLen;
    if(cache_retrieve_if_possible(&respCache,szKey,respBuf,&respLen) == 1){
        info("Retrieved response from cache.");
        output_response_from_buffer(fd,respBuf,respLen);
    }else{
        int clientfd = forward_request(pReqLine,pReqHeader);
        info("Try output %d's response to %d",clientfd,fd);
        respLen = output_response_from_remote(clientfd,fd,respBuf,MAX_OBJECT_SIZE);
        if(respLen > MAX_OBJECT_SIZE){
            info("Max cacheable object size exceeded.");
        }else{
            cache_insert_if_possible(&respCache,szKey,respBuf,respLen);
            info("Object cached.");
        }
    }
    info("ok");
    free_request_line(pReqLine);
    free_request_header(pReqHeader);
}

