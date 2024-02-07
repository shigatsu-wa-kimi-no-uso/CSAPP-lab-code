#ifndef __REQUEST_H__
#define __REQUEST_H__

/*
HTTP/1.1 200 OK
Access-Control-Allow-Credentials: true
Access-Control-Allow-Headers: Origin,Authorization,Access-Control-Allow-Origin,Access-Control-Allow-Headers,Content-Type
Access-Control-Allow-Methods: *
Access-Control-Allow-Origin:
Access-Control-Expose-Headers: Content-Length
Content-Length: 72
Content-Type: application/json; charset=utf-8

{
  ...
  ...
  "status": true
}
*/


/*
GET http://localhost:3575/home.html HTTP/1.1
Access-Control-Allow-Credentials: true
Access-Control-Allow-Headers: Origin,Authorization,Access-Control-Allow-Origin,Access-Control-Allow-Headers,Content-Type
Access-Control-Allow-Methods: *
Access-Control-Allow-Origin:
Access-Control-Expose-Headers: Content-Length
Content-Length: 72
Content-Type: application/json; charset=utf-8
{
  ...
  ...
  "status": true
}
*/

typedef enum request_method{
    GET,POST,PUT,DELETE,HEAD,OPTIONS,TRACE,CONNECT
}request_method;

typedef enum http_ver{
    HTTP_v10,HTTP_v11,HTTP_v20,HTTP_v21
}http_ver;

typedef struct uri{
#define PROTOCOL_STR_MAXLEN 15
#define HOSTNAME_STR_MAXLEN 63
#define PATH_STR_MAXLEN 1959
    char szProtocol[PROTOCOL_STR_MAXLEN+1];
    char szHostname[HOSTNAME_STR_MAXLEN+1];
    char szPath[PATH_STR_MAXLEN+1];
    unsigned short wPort;
}uri;

typedef struct http_request_line{
#define METHOD_STR_MAXLEN 7
#define URI_STR_MAXLEN 2047
#define VERSION_STR_MAXLEN 15
#define REQUEST_LINE_STR_MAXLEN 2079
    char szMethod[METHOD_STR_MAXLEN+1];
    uri URI;
    char szVersion[VERSION_STR_MAXLEN+1];
} request_line;

typedef struct http_reqhdr_entry{
#define REQHDR_KEY_STR_MAXLEN 127
#define REQHDR_VALUE_STR_MAXLEN 1023
    char szKey[REQHDR_KEY_STR_MAXLEN+1];
    char szValue[REQHDR_VALUE_STR_MAXLEN+1];
}reqhdr_entry;

typedef struct http_request_header{
    reqhdr_entry entry;
    struct http_request_header* pNext;
}request_header;


request_line *parse_http_request_line(char* buf,size_t dataSize);

void free_request_line(request_line *p);

request_header* parse_and_add_reqhdr_entry(char* szEntry,request_header* pLast);

request_header* add_reqhdr_entry(char* szKey,char* szValue,request_header* pLast);

void free_request_header(request_header* p);

#endif