#pragma once
#include "Common.h"

//����ȫ��ֻ����һ��CentralCache���������������Ϊ����ģʽ
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
    return &_sInst;
    //static std::mutex mtx;
    //if(_sInst == nullptr)
    //{
    //  mtx.lock();
    //  if(_sInst == nullptr)
    //  {
    //    _sInst = new CentralCache;
    //  }
    //  mtx.unlock();
    //}
    //return _sInst;
	}
	
	//��ȡһ���ǿյ�Span
	Span* GetOneSpan(SpanList& list, size_t size);

	//��CentralCache��ȡһ�������Ķ����ThreadCache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);

	//����start��ʼ��freelist�е�����С���ڴ涼�黹��spanlists[index]�еĸ��Ե�span
	void ReleaseListToSpans(void* start, size_t size);
private:
	CentralCache(){}
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;
	
	static CentralCache _sInst;	//����(������.cpp����)
private:
	SpanList _spanlists[NFREELISTS];	//��ϣͰ�ṹ
};
