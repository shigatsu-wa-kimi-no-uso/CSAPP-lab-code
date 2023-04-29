#include "simulator.h"

SIMULATOR* new_SIMULATOR()
{
    SIMULATOR* obj=malloc(sizeof(SIMULATOR));
    obj->lastState=0;
    obj->lineForUse=0;
    obj->stateCnt[0]=0;
    obj->stateCnt[1]=0;
    obj->stateCnt[2]=0;
    obj->stateCnt[3]=0;
    obj->szAccessState[COLD_MISS]="miss";
    obj->szAccessState[CONFLICT_MISS]="miss";
    obj->szAccessState[HIT]="hit";
    obj->szAccessState[EVICTION]="eviction";
    obj->accessOrder=0;
    obj->accessCache=accessCache;
    obj->loadToCache=loadToCache;
    obj->evictCache=evictCache;
    obj->operate=operate;
    return obj;
}

void delete_SIMULATOR(SIMULATOR* obj)
{
    free(obj);
}

void accessCache(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,QWORD tag,QWORD blockOffset,ACCESS_MODE mode,BYTE size,BYTE data[])
{
    thisObj->accessOrder++;                                                             //每次执行一次访问都要加一次访问次序!
    CACHESET* thisSet=cache->cacheSet[setIndex];                                        //抽取缓存组
    thisObj->lastState=CONFLICT_MISS;                                                   //预先设置访问状态为冲突不命中
    QWORD lruAccessOrder=ULLONG_MAX;                                                    

    for(int i=0;i<cache->lineCnt;i++)                                                   //遍历缓存行
    {
        if(thisSet->cacheLine[i]->isValid==1)                                           //检测缓存行是否有效
        {
            if(thisSet->cacheLine[i]->tag==tag)                                         //匹配tag
            {
                if(mode==READ)                                                         
                {
                    for(int j=0;j<size;j++)
                    {                                     
                        data[i]=thisSet->cacheLine[i]->data[blockOffset+j];             //模拟从缓存读取数据到寄存器,data数组模拟一个寄存器(LOAD操作)
                    }
                }
                else if(mode==WRITE)                                                    
                {
                    for(int j=0;j<size;j++)
                    {                                     
                        thisSet->cacheLine[i]->data[blockOffset+i]=data[j];             //模拟将寄存器数据写入缓存,data数组模拟一个寄存器(STORE操作)
                    }
                }
                thisSet->cacheLine[i]->accessOrder=thisObj->accessOrder;                    //记录访问次序
                thisObj->lastState=HIT;                                                 //设置状态为HIT
                break;
            }

            if(lruAccessOrder > thisSet->cacheLine[i]->accessOrder)                       //记录访问次序最小的缓存行号，以备不命中时使用
            {
                lruAccessOrder=thisSet->cacheLine[i]->accessOrder;
                thisObj->lineForUse=i;
            }
        }
        else                                                                            //缓存有效位为0的情况
        {
            lruAccessOrder=0;                                                            //将lru置0,防止有效的缓存行干扰
            thisObj->lineForUse=i;                                                      //设置为待写入的缓存行
            thisObj->lastState=COLD_MISS;                                               //有空缺的缓存行,设置访问状态为COLD_MISS
        }
    }
}

void loadToCache(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,QWORD tag,DWORD lineId,BYTE data[])     //加载内存到缓存
{
    thisObj->accessOrder++;                                                                             //每次执行一次访问都要加一次访问次序!
    CACHESET* thisSet=cache->cacheSet[setIndex];                                                        //抽取缓存组
    for(int i=0;i<cache->blockCnt;i++)                                                                  //一次固定从内存读满整个block
    {
        thisSet->cacheLine[lineId]->data[i]=data[i];                                                    //模拟将block大小的内存块写入缓存
    }
    thisSet->cacheLine[lineId]->tag=tag;                                                                //设置tag
    thisSet->cacheLine[lineId]->accessOrder=thisObj->accessOrder;                                           //记录访问次序
    thisSet->cacheLine[lineId]->isValid=1;                                                              //设置有效位为1
    thisObj->lastState=WARMED_UP;                                                                       //设置访问状态为变暖
}

void evictCache(SIMULATOR* thisObj,CACHE* cache,QWORD setIndex,DWORD lineId,BYTE data[])               //模拟缓存行驱逐
{
    //驱逐不算访问,accessOrder不需要+1了
    CACHESET* thisSet=cache->cacheSet[setIndex];
    thisSet->cacheLine[lineId]->isValid=0;                                                              //设置有效位为0
    for(int i=0;i<cache->blockCnt;i++)                                                  
    {
        data[i]=thisSet->cacheLine[lineId]->data[i];                                                    //模拟将缓存写回内存
    }
    thisObj->lastState=EVICTION;                                                                        //设置访问状态为驱逐
}

void operate(SIMULATOR* thisObj,CACHE* cache,OPINFO* opinfo,BOOL isVerboseOn)
{
    int state[6]={-1,-1,-1,-1,-1,-1};                                                                    //状态存储数组,存储每次操作的状态以备后续打印
    int k=0;                                                                                                //要保存的状态数量计数
    BYTE* reg=(BYTE*)malloc(8*sizeof(BYTE));                                                               //模拟一个64位寄存器
    BYTE* mem=(BYTE*)malloc(cache->blockCnt*sizeof(BYTE));                                                 //模拟一块block大小的内存
    switch(opinfo->optype)
    {
    case INSTRUCT: break;                                                                                   //INSTRUCT读取指令操作直接忽略
    case LOAD:   
        //LOAD操作为读取内存到寄存器,先尝试直接读取缓存,如果不命中,则需要将这块内存写入缓存以备后续使用
        accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,READ,opinfo->size,reg); //尝试直接读取缓存内容到寄存器
        state[k++]=thisObj->lastState;                                                                      //将访问状态存储起来,以备后续打印缓存访问状态
        thisObj->stateCnt[thisObj->lastState]++;                                                            //对应状态计数+1
        switch (thisObj->lastState)
        {
        case COLD_MISS:    
        {   
            //不命中类型为冷不命中,有行缓存空缺,无需驱逐直接写入缓存即可
            loadToCache(thisObj,cache,opinfo->setIndex,opinfo->tag,thisObj->lineForUse,mem);              //模拟加载内存到缓存,读取整个block的内存,mem数组模拟一块内存
            accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,READ,opinfo->size,reg); //加载缓存后，读取缓存内容到寄存器
            //因为必定命中，且题目没有要求加载后记录访问状态信息，因此,此access状态不记录
            break;
        }
        case CONFLICT_MISS:
        {
            //不命中类型为冲突不命中,需要驱逐一个行缓存,mem数组模拟内存
            evictCache(thisObj,cache,opinfo->setIndex,thisObj->lineForUse,mem);                                
            state[k++]=thisObj->lastState;                                                                     //记录状态                  
            thisObj->stateCnt[thisObj->lastState]++;                                                         //对应状态计数+1           
            loadToCache(thisObj,cache,opinfo->setIndex,opinfo->tag,thisObj->lineForUse,mem);                    //驱逐后,加载内存到缓存,读取整个block的内存,mem数组模拟一块内存
            accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,READ,opinfo->size,reg); //加载缓存后，读取缓存内容到寄存器
            //因为必定命中，且题目没有要求加载后记录访问状态信息，因此,此access状态不记录
            break;
        }
        default:
            break;
        }
        break;
    case STORE:          
        //STORE操作为将寄存器值写入内存,先尝试直接写入缓存,如果缓存中没有,则需要将这块内存写入缓存以备后续使用(写分配法)
        accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,WRITE,opinfo->size,reg); //模拟写入缓存
        state[k++]=thisObj->lastState;                                                                      //记录状态
        thisObj->stateCnt[thisObj->lastState]++;                                                            //对应状态计数+1
        switch (thisObj->lastState)
        {
        case COLD_MISS:     
        {       
            //不命中类型为冷不命中,有行缓存空缺,无需驱逐直接写入缓存即可                                                 
            loadToCache(thisObj,cache,opinfo->setIndex,opinfo->tag,thisObj->lineForUse,mem);               //模拟加载内存到缓存
            accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,READ,opinfo->size,reg); //加载缓存后，读取缓存内容到寄存器
            //因为必定命中，且题目没有要求加载后记录访问状态信息，因此此状态不记录
            break;
        }
        case CONFLICT_MISS:
        {         
            //不命中类型为冲突不命中,需要驱逐一个行缓存,mem模拟内存
            evictCache(thisObj,cache,opinfo->setIndex,thisObj->lineForUse,mem);                       
            state[k++]=thisObj->lastState;                                                                  //记录状态
            thisObj->stateCnt[thisObj->lastState]++;                                                         //对应状态计数+1
            loadToCache(thisObj,cache,opinfo->setIndex,opinfo->tag,thisObj->lineForUse,mem);               //模拟加载内存到缓存
            accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,READ,opinfo->size,reg); //加载缓存后，读取缓存内容到寄存器
            //因为必定命中，且题目没有要求加载后记录访问状态信息，因此此状态不记录
            break;
        }
        default:
            break;
        }
        break;
    case MODIFY:                                                                                           
        //MODIFY为修改操作,读取内存到寄存器,修改后将寄存器值保存到该缓存
        //先尝试直接修改缓存,则操作为:读取缓存到寄存器->修改->将寄存器值保存到缓存
        accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,READ,opinfo->size,reg);  //尝试直接读取缓存中的值到寄存器
        state[k++]=thisObj->lastState;                                                                      //记录状态        
        thisObj->stateCnt[thisObj->lastState]++;                                                         //对应状态计数+1                                                     
        switch (thisObj->lastState)
        {
        case COLD_MISS:
        {  
            //不命中类型为冷不命中,有行缓存空缺,无需驱逐直接写入缓存即可
            loadToCache(thisObj,cache,opinfo->setIndex,opinfo->tag,thisObj->lineForUse,mem);               //模拟加载内存到缓存
            accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,READ,opinfo->size,reg); //加载后读取
            //因为必定命中，且题目没有要求加载后记录访问状态信息，因此此状态不记录
            break;
        }
        case CONFLICT_MISS:
        {
            //不命中类型为冲突不命中,需要驱逐一个行缓存,data数组模拟内存
            evictCache(thisObj,cache,opinfo->setIndex,thisObj->lineForUse,mem);                       
            state[k++]=thisObj->lastState;                                                                  //记录状态          
            thisObj->stateCnt[thisObj->lastState]++;                                                         //对应状态计数+1                       
            loadToCache(thisObj,cache,opinfo->setIndex,opinfo->tag,thisObj->lineForUse,mem);                //模拟加载内存到缓存
            accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,READ,opinfo->size,reg);//加载后读取
            //因为必定命中，且题目没有要求加载后记录访问状态信息，因此此状态不记录
            break;
        }
        default:
            break;
        }
        accessCache(thisObj,cache,opinfo->setIndex,opinfo->tag,opinfo->blockOffset,WRITE,opinfo->size,reg); //将寄存器值写入写入缓存
        state[k++]=thisObj->lastState;                                                                       //记录状态
        thisObj->stateCnt[thisObj->lastState]++;                                                         //对应状态计数+1
        break;
    }
    free(reg);
    free(mem);

    if(isVerboseOn)                                                                                           //打印细节
    {
        printf("%c %llx,%hhd ",opinfo->optype,opinfo->memoryAddr,opinfo->size);
        for(int i=0;i<6;i++)
        {
            if(state[i]==-1||state[i]==WARMED_UP) continue;
            printf("%s ",thisObj->szAccessState[state[i]]);
        }
        putchar('\n');
    }
}