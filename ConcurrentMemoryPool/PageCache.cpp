#include "PageCache.h"

PageCache PageCache::_sInst;	//��̬��Ա���ⶨ��

//NewSpan����Span��3�����:(ÿ��ֻ��CentralCache 1��Span)
//1. CentralCache��Ҫ��ҳ���µ�Spanlist�д���Span����ôֱ�ӷ������1��Span�Ϳ�����
//2. CentralCache��Ҫ��ҳ���µ�Spanlist��û��Span, ��ô�ӵ�ǰλ���������Spanlist[k + 1], �ҵ�Span�󣬶�Span�����з֣�Ȼ���ٹ��أ��������CentralCache
//3. ��ǰλ�������Spanlistû��Span�ˣ���ô��ϵͳ(����)����һ��NPAGES��С��Span��Ȼ���������зֺ͹��أ��������CentralCache
Span* PageCache::NewSpan(size_t size)
{

}
