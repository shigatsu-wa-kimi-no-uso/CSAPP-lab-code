#ifndef ARGPARSER_H
#define ARGPARSER_H
#include "common.h"

typedef struct PROGARGS PROGARGS;

struct PROGARGS
{
    int s;                  //-s的参数
    int E;                  //-E参数
    int b;                  //-b参数
    int setCnt;             //由参数s计算得到的缓存组数
    int blockCnt;           //由参数b计算得到的缓存块字节数
    int lineCnt;            //由各个参数计算得到的缓存行数
    int tagBitCnt;          //由各个参数计算得到的标签位位数
    char input[30];         //-t的参数(模拟指令文件)
    BOOL isVerboseOn;       //-v命令行选项是否已打开
    BOOL (*parseArgs)(PROGARGS* thisObj,int nCmdCnt, char* szCmd[]); //命令行分析器
};

PROGARGS* new_PROGARGS();
void delete_PROGARGS(PROGARGS* obj);
void printHelp(char* szFilename);       //打印help页面
BOOL parseArgs(PROGARGS* thisObj/*对象本身,相当于this指针*/,int nCmdCnt, char* szCmd[]);

#endif


