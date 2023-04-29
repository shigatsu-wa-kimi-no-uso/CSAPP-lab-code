#include "opparser.h"

OPPARSER* new_OPPARSER(int s,int b)
{
    OPPARSER* obj=(OPPARSER*)malloc(sizeof(OPPARSER));
    assert(obj);
    obj->blockMask=(1<<b)-1;                            //缓存块偏移位的掩码
    obj->blockBitCnt=b; 
    obj->setIndexMask=((1<<s)-1)<<b;                    //缓存行索引位的掩码
    obj->setCnt=1<<s;                                   //缓存组数
    obj->setBitCnt=s;              
    obj->tagMask=~(obj->blockMask|obj->setIndexMask);   //标签位的掩码
    obj->parseOp=parseOp;
    return obj;
}

void delete_OPPARSER(OPPARSER* obj)
{
    free(obj);
}

BOOL parseOp(OPPARSER* thisObj,char* szString,OPINFO* buffer)  //此函数一次分析一条指令,并将结果放入已经分配好空间的OPINFO结构体
{
    BYTE optype;
    QWORD address;
    BYTE size;
    if(buffer==NULL || szString==NULL ||thisObj==NULL) return 0;
    if(szString[0]==' ')
    {
        if(sscanf(szString," %c %llx,%hhd",&optype,&address,&size)<3)       //读取指令的方法,sscanf控制读取格式
        {
            return 0;
        }
    }
    else
    {
        if(sscanf(szString,"%c %llx,%hhd",&optype,&address,&size)<3)
        {
            return 0;
        }
    }
    buffer->optype=(OPTYPE)optype;                                              //操作类型(L,S,M,I)
    buffer->memoryAddr=address;                                                 //内存地址
    buffer->setIndex=((address&thisObj->setIndexMask)>>(thisObj->blockBitCnt)); //计算缓存组索引的方法
    buffer->tag=address&thisObj->tagMask;                                       //获取标签位(保留标签位,其余位置0,实际中应把0去掉,此处为简便不再右移去0)
    buffer->blockOffset=address&thisObj->blockMask;                             //获取缓存块偏移位
    buffer->size=size;                                                          //访问的字节大小
    return 1;
}

