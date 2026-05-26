#pragma once
#include"Common.h"

class CentralCache{
public:
	static CentralCache* GetInstance(){
		return &_CenIns;
	}

	span* GetOneSpan(SpanList& list, size_t size);
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);
	void ReleaseListToSpans(void* start, size_t size);

private:
	SpanList _SpanList[MaxFreeLists];
private:
	CentralCache()
	{}
	CentralCache(const CentralCache&) = delete;

	static CentralCache _CenIns;
};
