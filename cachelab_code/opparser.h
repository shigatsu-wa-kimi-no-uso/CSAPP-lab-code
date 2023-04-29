#ifndef OPPARSER_H
#define OPPARSER_H
#include "common.h"

typedef struct OPPARSER OPPARSER;
typedef struct OPINFO OPINFO;

typedef enum OPERATION_TYPE       //操作类型枚举
{
    INSTRUCT='I',LOAD='L',STORE='S',MODIFY='M'
}OPTYPE;

struct OPINFO
{
    OPTYPE optype;              //操作类型
    QWORD memoryAddr;           //内存地址
    QWORD setIndex;             //组索引
    QWORD tag;                  //标签位的值
    QWORD blockOffset;          //缓存块的偏移
    BYTE size;                  
};

struct OPPARSER
{
    QWORD tagMask;              //标签位的掩码, 使用内存地址&对应掩码即可得到需要的二进制位的值,去除其他位的干扰
    QWORD blockMask;            //缓存块偏移位区的掩码
    QWORD setIndexMask;         //缓存组位的掩码
    DWORD setCnt;               //缓存组数(S)
    DWORD setBitCnt;            //缓存组位数(s)
    DWORD blockBitCnt;          //缓存块偏移位区的位数(b)

    BOOL (*parseOp)(OPPARSER* thisObj,char* szString,OPINFO* buffer); //解释器,接收一条指令,解释结果放入OPINFO结构体,同时返回成功1/失败0的信息
};

OPPARSER* new_OPPARSER(int s,int b);
void delete_OPPARSER(OPPARSER* obj);
BOOL parseOp(OPPARSER* thisObj,char* szString,OPINFO* buffer);

#endif

