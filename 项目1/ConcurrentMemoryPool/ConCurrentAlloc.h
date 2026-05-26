#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

static void* ConCurrentAlloc(size_t size){
	if (size > MaxBytes){
		size_t align = SizeClass::RoundUp(size);
		size_t page = align >> PageShift;

		PageCache::GetInstance()->GetMutex().lock();
		span* Span = PageCache::GetInstance()->NewSpan(page);
		Span->_FreeKnowSize = align;
		PageCache::GetInstance()->GetMutex().unlock();

		return (void*)(Span->Page_Adder << PageShift);

	}
	else{
		if (pTLSThreadCache == nullptr)
		{
			static std::mutex tcMtx;
			static ObjectPool<ThreadCache> TLPool;
			tcMtx.lock();
			pTLSThreadCache = TLPool.New();
			tcMtx.unlock();
		}

		return pTLSThreadCache->Allocate(size);
	}
}

static void ConCurrentFree(void* ptr){
	span* Span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = Span->_FreeKnowSize;
	if (size > MaxBytes){
		PageCache::GetInstance()->GetMutex().lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(Span);
		PageCache::GetInstance()->GetMutex().unlock();
	}
	else{
		assert(pTLSThreadCache);
		pTLSThreadCache->Deallocate(ptr, size);
	}
}
