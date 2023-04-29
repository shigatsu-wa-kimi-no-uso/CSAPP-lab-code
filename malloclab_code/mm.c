/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "kiminouso",
    /* First member's email address */
    "kiminouso@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define TYPEDEFS
#define FUNC_DECLS
#define GLOBAL_VARS
#define MACROS
//#define DEBUG
//#define HEAP_CHECKER

//维护一个大根堆, 堆的节点是指向每个空闲的块的header的指针
#ifdef TYPEDEFS
typedef size_t unit;
typedef void* pointer;
typedef unit* freetree_node;
typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;
#endif 

#ifdef FUNC_DECLS
static freetree_node* left_son_of_node(freetree_node* pNode);
static freetree_node* right_son_of_node(freetree_node* pNode);
static freetree_node* parent_of_node(freetree_node* pNode);
static void freetree_swap_nodes(freetree_node* a,freetree_node* b);
static size_t freetree_node_offset(freetree_node* pNode);
static BYTE block_status(unit* pHeader);
static size_t block_size(unit* pHeader);
static size_t block_unit_count(unit* pHeader);
static size_t block_node_offset(unit* pHeader);
static BYTE block_footer_status(unit* pHeader);
static unit* block_next_header(unit* pHeader);
static void block_set_header_attrib_status(unit* pHeader,int status);
static void block_set_header_attrib_size(unit* pHeader,size_t size);
static void block_set_footer_attrib_node_offset(unit* pHeader,freetree_node* pNode);
static void block_set_footer_attrib_status(unit* pHeader,BYTE status);
static freetree_node* block_node_ptr(unit* pHeader);
static unit* block_header(unit* pFreedBlockFooter);
static unit* block_tryget_previous_available(unit* pHeader);
static unit* block_tryget_next_available(unit* pHeader);
static pointer block_allocate(unit* pHeader, size_t size);
static void block_setup_new_wandering(unit* pUnit);
static void block_setup_new_allocated(pointer pBlockStart,size_t size);
static void block_setup_new_freed(pointer pBlockStart,size_t size,freetree_node* pNode);
static void freetree_sink_node(freetree_node* pNode);
static void freetree_delete_node(freetree_node* pNode);
static void freetree_emerge_node(freetree_node* pNode);
static void freetree_adjust_capacity_if_necessary();
static freetree_node* freetree_new_node();
static freetree_node* freetree_find_suitable_node(size_t size);
static pointer lowest_address(pointer a,pointer b);
static pointer highest_address(pointer a,pointer b);
static void block_coalesce_two(unit* pNewFreedHeader,unit* pAnotherHeader);
static void block_coalesce_three(unit* pNewFreedHeader, unit* pPrevHeader, unit* pNextHeader);
static void block_bind_freed_with_existed_node(unit* pNewHeader,size_t newFreedSize,freetree_node* pRelatedOldNode);
static void mm_check();
static pointer* mem_enlarge_and_allocate(size_t size);
#endif

#ifdef GLOBAL_VARS
pointer heap_start;    //堆的起始地址
pointer heap_end;       //堆的结束地址
//堆地址范围: [heap_start,heap_end)
freetree_node* freetree_root_ptr; //大根堆的根节点指针, 最初大根堆的起始地址, 后续可能会由于分配释放的次数增加而移动
size_t freetree_node_end_offset;  //大根堆空间的最后一个叶子的指针相对root的偏移(以unit为单位)
size_t freetree_capacity;
//大根堆的地址范围:[freetree_root_ptr,freetree_root_ptr + freetree_capacity)
//大根堆节点的地址范围:[freetree_root_ptr,freetree_node_end_offset + freetree_root_ptr)
int pagesize;
int freetree_is_reallocating;
#endif 

#ifdef MACROS
#define FREETREE_MINIMUM_CAP 12
#define FREETREE_CAP_GROWTH_RATIO 1.5f
#define FREETREE_SHRINK_CAP_SCALE_THRESHOLD 0.6f
#define BLOCK_FREED 0
#define BLOCK_ALLOCATED 1
#define BLOCK_WANDERING 0b10
#define BLOCK_IS_WANDERING(p) (*(unit*)p==(unit)0x020000000000000A)
#define BLOCK_FREED_PADDINGS ((unit)(0xDDDDDDDDDDDDDDDD))
#define ADDRESS_POINTS_TO_FREETREE(ptr) (ptr>=freetree_root_ptr && ptr<freetree_root_ptr + freetree_capacity)
#define ADDRESS_OUT_OF_HEAP_BOUNDRY(ptr) ((size_t)ptr<(size_t)heap_start || (size_t)ptr>= (size_t)heap_end)
#define FOOTER_IS_VALID(pFooter) ((size_t)(BLOCK_FOOTER_OFFSET_VALUE(pFooter)  < (size_t)freetree_node_end_offset))
#define FREETREE_NODE_NOT_EXIST ((freetree_node*)(~0))
#define BLOCK_FOOTER_PTR(pHeader) ((unit*)pHeader + block_unit_count((unit*)pHeader) - 1)
#define BLOCK_FOOTER_OFFSET_VALUE(pFooter) ((*(unit*)pFooter)&(unit)0x00FFFFFFFFFFFFFF)
#define BLOCK_FOOTER_STATUS_PTR(pFooter) ((BYTE*)((BYTE*)pFooter + 7))
#define BLOCK_SIZE_VALUE(pHeader) ((*(unit*)pHeader)&(unit)0x00007FFFFFFFFFF8)
#define BLOCK_HEADER_STATUS_VALUE(pHeader) ((*(unit*)pHeader)&(size_t)0b111)
#define BLOCK_FOOTER_STATUS_VALUE(pFooter) ((BYTE)*((BYTE*)(((BYTE*)pFooter) + 7)))
#define BLOCK_PREVIOUS_FOOTER_PTR(pHeader) ((unit*)pHeader - 1)
#endif



#ifdef DEBUG
#define debug(fmt,...) printf(fmt,__VA_ARGS__)
#define PTR_INCR_BYTE(ptr,size) ((BYTE*)(ptr)+size)
#define BYTE_OFFSET(ptr,base_ptr) ((BYTE*)(ptr)-(BYTE*)(base_ptr))
#else
#define debug(fmt,...) ((void)0)
#endif

static freetree_node* left_son_of_node(freetree_node* pNode)
{
    return ((((pNode - freetree_root_ptr)<<1) + 1) + freetree_root_ptr);
}

static freetree_node* right_son_of_node(freetree_node* pNode)
{
    return ((((pNode - freetree_root_ptr)<<1) + 2) + freetree_root_ptr);
}

static freetree_node* parent_of_node(freetree_node* pNode)
{
    return ((((pNode - freetree_root_ptr) - 1)>>1) + freetree_root_ptr);
}

//交换freetree的节点, 保证对应指向的空闲块的footer与实际节点同步
static void freetree_swap_nodes(freetree_node* a,freetree_node* b)
{
    freetree_node tmp = *a;
    *a = *b;
    *b = tmp;
    block_set_footer_attrib_node_offset(*a,a);    //与指向的块同步
    block_set_footer_attrib_node_offset(*b,b);
}

static size_t freetree_node_offset(freetree_node* pNode)
{
    return pNode - freetree_root_ptr;
}

static BYTE block_status(unit* pHeader)
{
    return BLOCK_HEADER_STATUS_VALUE(pHeader);
}

static size_t block_size(unit* pHeader)
{
    //低3位,高16位屏蔽(一方面,用户可用地址范围中,高16位固定为0;另一方面,游离块的高8位值是0x2)
    return BLOCK_SIZE_VALUE(pHeader);
}

static size_t block_unit_count(unit* pHeader)
{
    return block_size(pHeader) / sizeof(unit);
}

static size_t block_node_offset(unit* pHeader)
{
    unit* pFooter = BLOCK_FOOTER_PTR(pHeader);
    if(pFooter == pHeader){
        return FREETREE_NODE_NOT_EXIST;
    }
    return BLOCK_FOOTER_OFFSET_VALUE(pFooter);
}

//返回已分配块的footer的status信息
static BYTE block_footer_status(unit* pHeader)
{
    return BLOCK_FOOTER_STATUS_VALUE(BLOCK_FOOTER_PTR(pHeader));
}

static unit* block_next_header(unit* pHeader)
{
    return pHeader + block_unit_count(pHeader);
}

static void block_set_header_attrib_status(unit* pHeader,int status)
{
    unit size = block_size(pHeader);
    *pHeader = size | (unit)status;    //先将最低3位状态位置0, 再加一个状态值
}

static void block_set_header_attrib_size(unit* pHeader,size_t size)
{
    BYTE status = block_status(pHeader); //提取状态位,以防止赋值时抹去
    *pHeader = size | status;       //设置块的信息单元的大小信息
}

//设置footer的节点偏移属性, 不会修改status, 最高的8位为status信息
static void block_set_footer_attrib_node_offset(unit* pHeader,freetree_node* pNode)
{
    BYTE status = block_footer_status(pHeader); //暂存
    *BLOCK_FOOTER_PTR(pHeader)  = freetree_node_offset(pNode); //写入offset会覆盖掉status
    block_set_footer_attrib_status(pHeader,status);//以前的状态信息重新写进去
}

//设置footer的status
static void block_set_footer_attrib_status(unit* pHeader,BYTE status)
{
    *BLOCK_FOOTER_STATUS_PTR(BLOCK_FOOTER_PTR(pHeader))= (BYTE)status;
}

static freetree_node* block_node_ptr(unit* pHeader)
{
    if(BLOCK_FOOTER_PTR(pHeader) == pHeader){
        return FREETREE_NODE_NOT_EXIST;
    }
    size_t offset = block_node_offset(pHeader);
	return offset + freetree_root_ptr;
}

static unit* block_header(unit* pFreedBlockFooter)
{
    return freetree_root_ptr[BLOCK_FOOTER_OFFSET_VALUE(pFreedBlockFooter)];
}

//获取紧邻的前一个块的指针, 如果该块已经被分配, 则返回空, 否则返回头指针
//游离块同样可以返回
static unit* block_tryget_previous_available(unit* pHeader)
{
    unit* pPrevBlockFooter = BLOCK_PREVIOUS_FOOTER_PTR(pHeader);
    BYTE footerStatusByte = BLOCK_FOOTER_STATUS_VALUE(pPrevBlockFooter);
    //当前块的头部左侧紧邻的8字节为该块在freetree中的节点指针相对偏移量或是已分配标记, 最高位为0则是偏移量,为1则是已分配标记.
    //当最高位为1时, 其8字节的偏移值作用在freetree的根指针时一定大于freetree的空间边界偏移freetree_node_end_offset, 因此根据这个可以判断左侧块是已分配还是空闲
    if( !ADDRESS_OUT_OF_HEAP_BOUNDRY(pPrevBlockFooter) 
        && FOOTER_IS_VALID(pPrevBlockFooter)){
        if(footerStatusByte == BLOCK_FREED
        && block_status(block_header(pPrevBlockFooter)) == BLOCK_FREED){
            return block_header(pPrevBlockFooter);
        }

//尝试获取前面一个块是否是游离块(空闲块但没有对应的堆节点指向它,只有大小为 1 unit 的块才是游离块)
//当前块header的前一个unit是header, 且大小为8, 状态位是0b010才是游离块
//如果是已经分配的块, 则大小不可能为8, 如果不是游离块, 则大小也不可能为8且状态位不会是0b010
        if(BLOCK_IS_WANDERING(pPrevBlockFooter)){
            return pPrevBlockFooter;
        }
    }
    return NULL;
}

static unit* block_tryget_next_available(unit* pHeader)
{
    unit* pNextHeader = block_next_header(pHeader);
    if( !ADDRESS_OUT_OF_HEAP_BOUNDRY(pNextHeader)){
        if(block_status(pNextHeader) == BLOCK_FREED || BLOCK_IS_WANDERING(pNextHeader)){
            return pNextHeader;
        }
    }
    return NULL;
}


//分配当前的空闲块, 返回用户实际使用的指针, 修改节点指向到下一个空闲块, 并维护freetree
static pointer block_allocate(unit* pHeader, size_t size)
{
    freetree_node* pNode = block_node_ptr(pHeader);
    unit* pRBound = block_next_header(pHeader);
 
    block_setup_new_allocated(pHeader,size);
    
    unit* pNewHeader = block_next_header(pHeader);
    size_t newSize = ((pRBound - pNewHeader)*sizeof(unit));
    if(newSize == 0){
        freetree_delete_node(pNode);
    }else{
        block_bind_freed_with_existed_node(pNewHeader,newSize,pNode);
    }
    return ((pointer)((unit*)pHeader + 1)); //返回用户实际使用的指针
}

static void block_setup_new_wandering(unit* pUnit)
{
    block_set_header_attrib_size(pUnit,sizeof(unit));
    block_set_header_attrib_status(pUnit,BLOCK_WANDERING);
    block_set_footer_attrib_status(pUnit,BLOCK_WANDERING);
    debug("setup new wandering:[%#x,%#x)\n",BYTE_OFFSET(pUnit,heap_start),BYTE_OFFSET(pUnit,heap_start)+sizeof(unit));
}

static void block_setup_new_allocated(pointer pBlockStart,size_t size)
{
    unit* pHeader = (unit*)pBlockStart;
    block_set_header_attrib_size(pHeader,size);
    block_set_header_attrib_status(pHeader,BLOCK_ALLOCATED);
    block_set_footer_attrib_status(pHeader,BLOCK_ALLOCATED);
    debug("setup new allocated:[%#x,%#x)\n",BYTE_OFFSET(pBlockStart,heap_start),BYTE_OFFSET(pBlockStart,heap_start)+size);
}


static void block_setup_new_freed(pointer pBlockStart,size_t size,freetree_node* pNode)
{
    unit* pHeader = (unit*)pBlockStart;
    block_set_header_attrib_size(pHeader,size);
    block_set_header_attrib_status(pHeader, BLOCK_FREED);
    block_set_footer_attrib_node_offset(pHeader, pNode);   //footer设置好之后, node位置的改变会同步到所指向块的footer
    block_set_footer_attrib_status(pHeader,BLOCK_FREED);
    *pNode = pHeader;
    debug("setup new freed:[%#x,%#x)\n",BYTE_OFFSET(pBlockStart,heap_start),BYTE_OFFSET(pBlockStart,heap_start)+size);
#ifdef HEAP_CHECKER
    size_t unitCount = block_unit_count(pHeader);
    memset(pHeader + 1,BLOCK_FREED_PADDINGS,(unitCount-2)*sizeof(unit));
#endif
}
//沉降节点, 当节点指向的块大小变小时, 沉降以找到合适的位置,以保证父节点始终大于子节点
//块被分配一部分空间后, 原节点改变指向到剩余空闲块的起始地址, 块大小改变, 需要沉降
//如果块被恰好分配出去, 无剩余空闲空间(实际上剩余2个紧挨着的header和footer), 则原节点指向剩余的header, 大小按0处理并沉降
static void freetree_sink_node(freetree_node* pNode)
{
    freetree_node* pCurr = pNode;
    freetree_node* pEnd = freetree_root_ptr + freetree_node_end_offset;
    while(pCurr < pEnd ){
        freetree_node* ls = left_son_of_node(pCurr);
        freetree_node* rs = right_son_of_node(pCurr);
        freetree_node* pNextNode = pCurr;
        //找到左右字节点中较大的那个, 交换, 如果都比当前节点小则沉降完毕
        if(ls < pEnd && (long long)block_size(*pCurr) < (long long)block_size(*ls)){
            pNextNode = ls;
        }
        if(rs < pEnd && (long long)block_size(*pNextNode) < (long long)block_size(*rs)){
            pNextNode = rs;
        }
        //没有找到比当前节点指向的块更大的节点时, break
        if(pCurr == pNextNode){ 
            break;
        }
        freetree_swap_nodes(pCurr,pNextNode);
        pCurr = pNextNode;
    }
}

//删除堆中的节点,与堆的最后一个叶子元素交换, freetree_node_end_offset减小, 被交换上来的节点下沉或上浮.
static void freetree_delete_node(freetree_node* pNode)
{
    freetree_node* pEnd = freetree_root_ptr + freetree_node_end_offset;
    if(pNode == pEnd - 1){
        freetree_node_end_offset--;
    }else{
        //当前节点设置为最后一个叶子节点
        *pNode = *(pEnd - 1);
        block_set_footer_attrib_node_offset(*pNode,pNode);  //同步
        freetree_node_end_offset--;
        freetree_sink_node(pNode);  //如果当前节点比子节点大,则不会下沉
        freetree_emerge_node(pNode); //如果当前节点比父节点小,则不会上浮
    }
   
}


//上浮节点, 是沉降节点的反操作
//节点指向的块大小变大或有新节点添加时使用
//free新的块时,有添加, free后与相邻块合并时, 会改变节点指向的块的大小
static void freetree_emerge_node(freetree_node* pNode){
    freetree_node* pCurr = pNode;
    while(pCurr!=freetree_root_ptr){
        freetree_node* parent = parent_of_node(pCurr);
        //与父节点比较, 如果父节点小,则交换, 否则上浮完毕
        if(block_size(*pCurr) > block_size(*parent)){
           freetree_swap_nodes(pCurr,parent);
           pCurr = parent;
        }else{
            break;
        }
    }
}

static void freetree_adjust_capacity_if_necessary()
{
    if(freetree_is_reallocating){
        return;
    }
    if(freetree_node_end_offset > freetree_capacity - 10){
        //防止嵌套调用
        freetree_is_reallocating = 1;
        size_t newCapacity = (size_t)(ALIGN((size_t)((float)freetree_capacity*FREETREE_CAP_GROWTH_RATIO)));  
        debug("cap:%lld-->%lld\n",freetree_capacity,newCapacity);
        freetree_root_ptr = mm_realloc(freetree_root_ptr,newCapacity*sizeof(unit));
        freetree_capacity = newCapacity;
        freetree_capacity = newCapacity;
        freetree_is_reallocating = 0;
    }else if(freetree_capacity > 256 && (float)freetree_node_end_offset/(float)freetree_capacity < FREETREE_SHRINK_CAP_SCALE_THRESHOLD){
        //防止嵌套调用
        freetree_is_reallocating = 1;
        size_t newCapacity = (size_t)ALIGN((freetree_node_end_offset/2 + freetree_capacity/2));  
        if(newCapacity == freetree_capacity || newCapacity < FREETREE_MINIMUM_CAP){
            freetree_is_reallocating = 0;
            return;
        }
        debug("cap:%lld-->%lld\n",freetree_capacity,newCapacity);
        freetree_root_ptr = mm_realloc(freetree_root_ptr,newCapacity*sizeof(unit));
        freetree_capacity = newCapacity;
        freetree_is_reallocating = 0;
    }
}

static freetree_node* freetree_new_node()
{
    freetree_node*  pNewNode = freetree_root_ptr + freetree_node_end_offset;
    freetree_node_end_offset++;
    return pNewNode;
}


static freetree_node* freetree_find_suitable_node(size_t size)
{
    freetree_node* pEnd = freetree_root_ptr + freetree_node_end_offset;
    if(pEnd==freetree_root_ptr || block_size(*freetree_root_ptr) < size){
        return NULL;
    }
    
    for(freetree_node* p = freetree_root_ptr; p<pEnd; ){
        if (block_size(*p)==size){
            return p;
        }else{
            freetree_node* pNext = p;
            freetree_node* ls = left_son_of_node(p);
            freetree_node* rs = right_son_of_node(p);
            if(ls<pEnd && block_size(*ls) >= size){
                pNext = ls;
            }
            if(rs<pEnd && block_size(*rs) >= size && block_size(*pNext) >= block_size(*rs)){
                pNext = rs;
            }
            if(pNext == p){
                return p;
            }
            p = pNext;
        }
    }
    return NULL;
}

static pointer lowest_address(pointer a,pointer b)
{
    return a < b ? a : b;
}

static pointer highest_address(pointer a,pointer b)
{
    return a < b ? b : a;
}


//新释放的块与一个相邻空闲块合并
//修改已有空闲块的大小, 并
static void block_coalesce_two(unit* pNewFreedHeader,unit* pAnotherHeader)
{
    freetree_node*  pNode = block_node_ptr(pAnotherHeader);
    size_t newSize = block_size(pNewFreedHeader) + block_size(pAnotherHeader);
    pointer pBlockStart = lowest_address(pAnotherHeader,pNewFreedHeader);
    if(pNode == FREETREE_NODE_NOT_EXIST){
        pNode = freetree_new_node();
    }
    block_setup_new_freed(pBlockStart,newSize,pNode);
    freetree_emerge_node(pNode); //交换节点时, 所指向的空闲块的footer会与之同步
}

//两个空闲块节点,新释放的块需要与左右两个块合并, 因此需要保留一个节点并上浮, 并删除另外一个节点
static void block_coalesce_three(unit* pNewFreedHeader, unit* pPrevHeader, unit* pNextHeader)
{
    //获取空闲块对应的freetree节点,如果是游离块则返回FREETREE_NODE_NOT_EXIST
    freetree_node* pNode1 = block_node_ptr(pPrevHeader);
    freetree_node* pNode2 = block_node_ptr(pNextHeader);
    // 游离块的freetree节点地址用FREETREE_NODE_NOT_EXIST的值代替, 为最大
    // 如果一个块在freetree中有节点而另一个是游离块没有对应的节点, 则待删除节点不会是已经存在的那个
    freetree_node* pNodeToBeDeleted = highest_address(pNode1,pNode2);   
    freetree_node* pNodeToBeEmerged = lowest_address(pNode1,pNode2); //待被合并的节点也不会是不存在的那个
    size_t newSize = block_size(pNewFreedHeader) + block_size(pPrevHeader) + block_size(pNextHeader); //游离块可以正常获取其大小
    pointer pBlockStart = pPrevHeader; //块头地址最低的作为整块起始地址
    //如果两个都是游离块, 则没有可以合并的节点, 需要新添
    if(pNodeToBeEmerged == FREETREE_NODE_NOT_EXIST){
        pNodeToBeEmerged = freetree_new_node();
    }
    block_setup_new_freed(pBlockStart,newSize,pNodeToBeEmerged);
    freetree_emerge_node(pNodeToBeEmerged);
    if(pNodeToBeDeleted!=FREETREE_NODE_NOT_EXIST){
        freetree_delete_node(pNodeToBeDeleted);
    }
}

//当一块空闲块被分配一部分空间后,余下的部分使用这个函数处理
//参数为这块空间的新头,新大小,和指向这块空间原来的头的旧节点指针
static void block_bind_freed_with_existed_node(unit* pNewHeader,size_t newFreedSize,freetree_node* pRelatedOldNode)
{
    freetree_node* pNewNode;
    if(pRelatedOldNode!=FREETREE_NODE_NOT_EXIST){
        if(newFreedSize == 0){
            freetree_delete_node(pRelatedOldNode);
            return;
        }else if(newFreedSize ==sizeof(unit)){
            freetree_delete_node(pRelatedOldNode);
            block_setup_new_wandering(pNewHeader);
            return;
        }else{
            pNewNode = pRelatedOldNode;
        }
    }else{
        if(newFreedSize==sizeof(unit)){
            block_setup_new_wandering(pNewHeader);
            return;
        }else{
            pNewNode = freetree_new_node();
        }
    }
    block_setup_new_freed(pNewHeader ,newFreedSize, pNewNode);

    freetree_emerge_node(pNewNode);
    freetree_sink_node(pNewNode);
}


static void mm_check()
{
#ifdef HEAP_CHECKER
    freetree_node* pEnd = freetree_root_ptr + freetree_node_end_offset;
    for(freetree_node* p=freetree_root_ptr;p<pEnd;++p){
        unit* pHeader = *p;
        size_t unitCount = block_unit_count(pHeader);
        unit* pRBound = pHeader + unitCount;
        if(*pHeader==BLOCK_FREED_PADDINGS){
            printf("Oops!! Block %p header has been overwritten by another freed block!\n",pHeader);
        }
        if(block_status(pHeader)!=BLOCK_FREED){
            printf("Oops!! Block %p has marked ALLOCATED or was not 8-byte aligned.\n",pHeader);
        }
        if(block_footer_status(pHeader)!=BLOCK_FREED){
            printf("Oops!! Block %p footer was not properly set.\n",pHeader);
        }
        if(block_node_ptr(pHeader)!=p){
            printf("Oops!! Block %p footer implied node pointer doesn't equal the real node address.\n",pHeader);
        }
 
        for(int offset = 1;offset < unitCount - 1; ++offset){
            if(pHeader[offset] != BLOCK_FREED_PADDINGS){
                printf("Oops!! Block %p freed units has corrupted.\n",pHeader);
                break;
            }
        }
    }
#endif
}


static pointer* mem_enlarge_and_allocate(size_t size)
{
    unit* pNew;
    //检测堆的边界是否是空闲块,如果是则利用上去
    unit* pEndBlockHeader = block_tryget_previous_available((unit*)heap_end);
 
    if(pEndBlockHeader!=NULL){
        freetree_node* pEndBlockNode = block_node_ptr(pEndBlockHeader);
        freetree_delete_node(pEndBlockNode);
        size_t endBlockSize = block_size(pEndBlockHeader);
        size_t sizeToEnlarge = size - endBlockSize;
        pNew = mem_sbrk(sizeToEnlarge);
        if (pNew == (unit*)(-1)){
            return NULL;
        }
        pNew = pEndBlockHeader;
    }else{
        pNew = mem_sbrk(size);
        if (pNew == (unit*)(-1)){
            return NULL;
        } 
    }
    heap_end = mem_heap_hi() + 1; 
    block_setup_new_allocated(pNew,size);
    return (pointer)((unit*)pNew + 1);
    
}


int mm_init(void)
{
    //初始化堆
    pagesize = mem_pagesize();
    freetree_capacity = FREETREE_MINIMUM_CAP;
    heap_end = heap_start = mem_heap_lo();
    freetree_node_end_offset = 0;
    freetree_root_ptr = heap_start;
    freetree_root_ptr = mm_malloc(freetree_capacity*sizeof(unit));
    return 0;
}


void *mm_malloc(size_t size)
{
    debug("\nbegin malloc:%llu Bytes\n",size);
    //Payload (0x7ffff769bb08:0x7ffff769fcb2) overlaps another payload (0x7ffff769f070:0x7ffff769fcf5)
    //对齐后的实际分配大小, 作为请求的块负载大小, 包含header和footer, header占用一个unit(8Bytes), footer占用1Byte
    mm_check();
    size_t newsize = ALIGN(size + sizeof(unit) + sizeof(BYTE));    
    freetree_node* pNode = freetree_find_suitable_node(newsize); 
    pointer user_ptr;
    //没找到就分配新的
    if(pNode!=NULL){
        user_ptr = block_allocate(*pNode,newsize);     //根据所需大小更新此块的信息
    } else {
        user_ptr = mem_enlarge_and_allocate(newsize);
    }
    freetree_adjust_capacity_if_necessary();
    mm_check();
    debug("malloc:[%#x,%#x)\n",BYTE_OFFSET((BYTE*)user_ptr-8,heap_start),BYTE_OFFSET((BYTE*)user_ptr-8,heap_start)+newsize);
    debug("%s","\nend malloc\n");
    return user_ptr;
}


void mm_free(void *ptr)
{
    debug("\nbegin free:%p\n",ptr);
    unit* pHeader = (unit*)ptr - 1;
    debug("free:[%#x,%#x)\n",BYTE_OFFSET(pHeader,heap_start),BYTE_OFFSET(pHeader,heap_start) + block_size(pHeader));
    unit* pPrevBlockHeader = block_tryget_previous_available(pHeader);
    unit* pNextBlockHeader = block_tryget_next_available(pHeader);

    if(pPrevBlockHeader!=NULL && pNextBlockHeader!=NULL){
        block_coalesce_three(pHeader,pPrevBlockHeader,pNextBlockHeader); //游离块会自动处理
    } else {
        unit* pValidOne = pPrevBlockHeader == NULL ? pNextBlockHeader : pPrevBlockHeader;
        if(pValidOne !=NULL){
            block_coalesce_two(pHeader,pValidOne);
        }else{
            freetree_node* pNewNode = freetree_new_node();
            block_setup_new_freed(pHeader,block_size(pHeader),pNewNode);
            freetree_emerge_node(pNewNode);
        }
    }

    freetree_adjust_capacity_if_necessary();
    mm_check();
    debug("\nend free:%p\n",ptr);
}


void *mm_realloc(void *ptr, size_t size)
{
 
    mm_check();
    size_t newSize = ALIGN(size + sizeof(unit) + sizeof(BYTE));   
    unit* pHeader = (unit*)ptr - 1;
    debug("\nbegin realloc: [%#x,%#x) %llu -> %llu Bytes\n",BYTE_OFFSET(pHeader,heap_start),BYTE_OFFSET(pHeader,heap_start) + block_size(pHeader),block_size(pHeader),newSize);
    unit* pNextHeader;
    unit* pPrevHeader;
    unit* pReallocatedHeader = NULL;   //重新分配的块的头
    freetree_node* pNextNode = NULL;
    freetree_node* pPrevNode = NULL;
    size_t adoptedSize = block_size(pHeader);   //在寻找块的过程中采用的块的大小总和
    size_t newFreedSize;    //新产生的空闲块的大小
    unit* pNewNextHeader;   //新产生的空闲块的头
    freetree_node* pRelatedNode = FREETREE_NODE_NOT_EXIST; //原来的指向空闲块的freetree节点指针
    size_t copySize = block_size(pHeader);
    if (size < copySize){
       copySize = size;
    }
    //缩容
    if(newSize < adoptedSize){
        block_setup_new_allocated(pHeader,newSize);
        pReallocatedHeader = pHeader;
    }else if(newSize == adoptedSize){
        mm_check();
            debug("%s","\nend realloc: no change\n");
        return pHeader + 1;
    }else{
        //扩容, 如果后面刚好有合适的空闲块,则合并并产生新的空闲块
        pNextHeader = block_tryget_next_available(pHeader);
        pPrevHeader = block_tryget_previous_available(pHeader);
        //情况1:后有空闲,且总和空间合适,需要将后面空闲块的节点从freetree中删除或者是修改大小并沉降
        if(pNextHeader!=NULL &&  (adoptedSize = block_size(pNextHeader) + block_size(pHeader)) >= newSize){
            pNextNode = block_node_ptr(pNextHeader);
            pReallocatedHeader = pHeader;
            pRelatedNode = pNextNode;
        //情况2, 前有空闲,前面的空间大小小于等于newSize,且总和空间合适, 需要将在后面新产生的空闲块添加到freetree, 利用前面已有的块的node,修改指向后沉降或上浮(如果不存在就新建)
        }else if(pPrevHeader!=NULL &&  block_size(pPrevHeader) <= newSize && (adoptedSize = block_size(pPrevHeader) + block_size(pHeader)) >=newSize){  
            //后面memmove后覆盖掉footer的node_offset属性,先读取出来
            //realloc的对象有可能是freetree,因此不能直接获取node指针
            size_t prevNodeOffset = block_node_offset(pPrevHeader);
            size_t nextNodeOffset = pNextHeader != NULL ? block_node_offset(pNextHeader) : (size_t)(-1);
            memmove(pPrevHeader + 1,ptr,copySize);
            pReallocatedHeader = pPrevHeader;
            if(freetree_is_reallocating){
                debug("%s","freetree reallocating\n");
                freetree_root_ptr = pReallocatedHeader + 1;
            }
            pPrevNode = freetree_root_ptr + prevNodeOffset;
            pRelatedNode = pPrevNode;
            //如果后面有空闲则将其考虑进去,待后面合并空闲块
            if(nextNodeOffset!=(size_t)(-1)){
                adoptedSize += block_size(pNextHeader);
                pNextNode = freetree_root_ptr + nextNodeOffset;
                if(nextNodeOffset!=(size_t)FREETREE_NODE_NOT_EXIST){
                    freetree_delete_node(pNextNode);
                }
            }
        //情况3, 前后均有空闲,且总和空间合适
        }else if(pPrevHeader!=NULL 
                && pNextHeader!=NULL 
                && (adoptedSize = block_size(pPrevHeader) + block_size(pNextHeader) + block_size(pHeader)) >=newSize){ 
                //后面memmove后覆盖掉footer的node_offset属性,先读取出来
                //realloc的对象有可能是freetree,因此不能直接获取node指针
                size_t prevNodeOffset = block_node_offset(pPrevHeader); //block_node_offset函数已经考虑是游离块无node情况
                size_t nextNodeOffset = block_node_offset(pNextHeader); 
                memmove(pPrevHeader + 1,ptr,copySize);
                pReallocatedHeader = pPrevHeader;
                if(freetree_is_reallocating){
                    debug("%s","freetree reallocating\n");
                    freetree_root_ptr = pReallocatedHeader + 1;
                }
                pPrevNode = freetree_root_ptr + prevNodeOffset;
                pNextNode = freetree_root_ptr + nextNodeOffset; 
                pRelatedNode = pPrevNode;

                //删除一个节点,另外一个在后面用来绑定新产生的空闲块
                //如果这个空闲块不是游离块(游离块在freetree中无节点)
                if(nextNodeOffset != (size_t)FREETREE_NODE_NOT_EXIST){
                    freetree_delete_node(pNextNode);
                }
        }
    }
    //没找到已有的块,开辟新空间
    if(pReallocatedHeader == NULL){
        pointer newPtr = mm_malloc(newSize);
        memcpy(newPtr,ptr,copySize);
        if(freetree_is_reallocating){
            debug("%s","freetree reallocating\n");
            freetree_root_ptr = newPtr;
        }
        mm_free(ptr);
        mm_check();
        freetree_adjust_capacity_if_necessary();
        debug("\nend realloc: [%#x,%#x)\n",BYTE_OFFSET((BYTE*)newPtr-sizeof(unit),heap_start),BYTE_OFFSET((BYTE*)newPtr-sizeof(unit),heap_start) + block_size(newPtr-sizeof(unit)));
        return newPtr;
    }else{
        block_setup_new_allocated(pReallocatedHeader,newSize);
        //将后面空闲的块添加到freetree
        newFreedSize = adoptedSize - newSize;
        if(newFreedSize != 0){
            //新产生的空闲块的头
            pNewNextHeader = block_next_header(pReallocatedHeader);
            block_bind_freed_with_existed_node(pNewNextHeader,newFreedSize,pRelatedNode); //无node情况会自动处理
        }else{
            if(pRelatedNode!=FREETREE_NODE_NOT_EXIST){
                freetree_delete_node(pRelatedNode);
            }
        }
        mm_check();
        freetree_adjust_capacity_if_necessary();
        debug("\nend realloc: [%#x,%#x)\n",BYTE_OFFSET(pReallocatedHeader,heap_start),BYTE_OFFSET(pReallocatedHeader,heap_start) + block_size(pReallocatedHeader));
        return pReallocatedHeader + 1;
    }
}













    