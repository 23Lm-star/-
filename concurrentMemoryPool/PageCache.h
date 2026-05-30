#pragma once

#include "Common.h"
#include "ObjectPool.h"
#include "RadixTreeHash.h"

class PageCache{
public:
	static PageCache* GetInstance(){
		return &_PCIns;
	}
	std::mutex& GetMutex(){
		return _mtx;
	}

	span* NewSpan(size_t k);
	span* MapObjectToSpan(void* obj);
	void ReleaseSpanToPageCache(span* span);

private:
	SpanList _PageList[MaxPage];
	std::mutex _mtx;

#ifdef _WIN64
	TCMalloc_PageMap3<64 - PageShift> _PageToSpan;
#elif _WIN32
	TCMalloc_PageMap1<32 - PageShift> _PageToSpan;
#endif
	ObjectPool<span> _pool;
private:
	PageCache() {}
	PageCache(const PageCache&) = delete;
	static PageCache _PCIns;
};
