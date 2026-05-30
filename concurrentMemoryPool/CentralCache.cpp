#include"CentralCache.h"
#include"PageCache.h"

CentralCache CentralCache::_CenIns;
ObjectPool<span> SpanList::_SpanPool;

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	assert(batchNum > 0);
	size_t index = SizeClass::Index(size);
	_SpanList[index].Mutex().lock();
	span* GetSpan = GetOneSpan(_SpanList[index], size);
	assert(GetSpan);
	assert(GetSpan->_FreeList);

	start = GetSpan->_FreeList;
	end = GetSpan->_FreeList;
	size_t ActualNum = 1;
	size_t i = 0;
	while (i < batchNum - 1 && Next(end) != nullptr)
	{
		end = Next(end);
		i++;
		ActualNum++;
	}
	GetSpan->_FreeList = Next(end);
	Next(end) = nullptr;

	GetSpan->_UseCount += ActualNum;

	_SpanList[index].Mutex().unlock();

	return ActualNum;
}

span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	span* it = list.begin();
	while (it != list.end())
	{
		if (it->_FreeList != nullptr)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	list.Mutex().unlock();

	PageCache::GetInstance()->GetMutex().lock();
	span* Span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	Span->_FreeKnowSize = size;
	Span->_IsUse = true;

	PageCache::GetInstance()->GetMutex().unlock();

	char* start = (char*)(Span->Page_Adder << PageShift);
	char* end = start + (Span->_n << PageShift);

	Span->_FreeList = start;
	void* tail = start;
	start += size;
	while (start < end)
	{
		Next(tail) = start;
		tail = start;
		start += size;
	}
	Next(tail) = nullptr;

	list.Mutex().lock();
	list.InsertFront(Span);

	return Span;
}

void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	assert(start != nullptr);
	size_t index = SizeClass::Index(size);
	_SpanList[index].Mutex().lock();

	while (start != nullptr)
	{
		void* next = Next(start);
		span* Span = PageCache::GetInstance()->MapObjectToSpan(start);

		Next(start) = Span->_FreeList;
		Span->_FreeList = start;

		Span->_UseCount--;
		if (Span->_UseCount == 0)
		{
			_SpanList[index].Erase(Span);
			Span->_FreeList = nullptr;
			Span->_next = nullptr;
			Span->_prev = nullptr;

			_SpanList[index].Mutex().unlock();
			PageCache::GetInstance()->GetMutex().lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(Span);
			PageCache::GetInstance()->GetMutex().unlock();
			_SpanList[index].Mutex().lock();
		}

		start = next;
	}

	_SpanList[index].Mutex().unlock();
}
