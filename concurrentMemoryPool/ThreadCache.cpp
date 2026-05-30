#include"ThreadCache.h"
#include"CentralCache.h"

void* ThreadCache::Allocate(size_t size)
{
	size_t align = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);

	if (_FreeLists[index].Empty()){
		return FetchFromCentralCache(index, align);
	}
	else{
		return _FreeLists[index].Pop();
	}
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	size_t batchNum = min(SizeClass::NumMoveSize(size), _FreeLists[index].MaxSize());
	if (_FreeLists[index].MaxSize() == batchNum){
		_FreeLists[index].MaxSize()++;
	}

	void* start = nullptr;
	void* end = nullptr;
	size_t ActualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(ActualNum >= 1);
	assert(start != nullptr && end != nullptr);

	if (ActualNum == 1){
		assert(start == end);
		return start;
	}
	else{
		_FreeLists[index].PushRange(Next(start), end, ActualNum - 1);
		return start;
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	size_t index = SizeClass::Index(size);
	_FreeLists[index].Push(ptr);

	if (_FreeLists[index].Size() >= _FreeLists[index].MaxSize())
	{
		ListTooLong(_FreeLists[index], size);
	}
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;
	list.PopRange(start, end, list.MaxSize());
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
