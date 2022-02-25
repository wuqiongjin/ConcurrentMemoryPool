#include "PageCache.h"

PageCache PageCache::_sInst;	//静态成员类外定义

//NewSpan分配Span有3种情况:(每次只给CentralCache 1个Span)
//1. CentralCache索要的页号下的Spanlist中存有Span，那么直接分配给它1个Span就可以了
//2. CentralCache索要的页号下的Spanlist中没有Span, 那么从当前位置往后遍历Spanlist[k + 1], 找到Span后，对Span进行切分，然后再挂载，最后分配给CentralCache
//3. 当前位置往后的Spanlist没有Span了，那么向系统(堆区)申请一块NPAGES大小的Span，然后对其进行切分和挂载，最后分配给CentralCache
Span* PageCache::NewSpan(size_t size)
{

}
