#include "cachelab.h"
#include "cache.h"
#include "argparser.h"
#include "simulator.h"
#include "opparser.h"

PROGARGS* progArgs;
CACHE* cache;
OPPARSER* opparser;
SIMULATOR* simulator;

int main(int nCmdCnt, char* szCmd[])
{
    progArgs=new_PROGARGS();

    if(progArgs->parseArgs(progArgs,nCmdCnt,szCmd)==0)  //解析命令行参数
    {
        return 0;
    }
    opparser=new_OPPARSER(progArgs->s,progArgs->b);
    cache=new_CACHE(progArgs->setCnt,progArgs->lineCnt,progArgs->blockCnt);
    simulator=new_SIMULATOR();

    FILE* input=fopen(progArgs->input,"r");
    assert(input);

    char op[40];
    OPINFO opinfo;

    while(1)
    {
        char* ptr=fgets(op,40,input);
        if(ptr==NULL) break;                            //文件结束,读取字符串返回空指针,跳出循环

        if(opparser->parseOp(opparser,ptr,&opinfo))
        {
            simulator->operate(simulator,cache,&opinfo,progArgs->isVerboseOn);
        }
    }
    fclose(input);
    printSummary(simulator->stateCnt[HIT], simulator->stateCnt[COLD_MISS]+simulator->stateCnt[CONFLICT_MISS], simulator->stateCnt[EVICTION]);
    delete_CACHE(cache);
    delete_OPPARSER(opparser);
    delete_PROGARGS(progArgs);
    return 0;
}


