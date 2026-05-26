#include"PageCache.h"

PageCache PageCache::_PCIns;


span* PageCache::NewSpan(size_t k){
	if (k > MaxPage - 1){
		void* ptr= SystemAlloc(k);
		span* Span = _pool.New();
		Span->Page_Adder = (Page_t)ptr >> PageShift;
		Span->_n = k;

		_PageToSpan.set(Span->Page_Adder, Span);
		return Span;
	}

	if (!_PageList[k].Empty()){
		span* NewSpan = _PageList[k].EraseFront();

		for (Page_t i = NewSpan->Page_Adder; i < NewSpan->Page_Adder + k; i++){
			_PageToSpan.set(i, NewSpan);
		}
		return NewSpan;
	}

	for (size_t i = k + 1; i < MaxPage; i++)
	{
		if (!_PageList[i].Empty())
		{
			span* BigSpan = _PageList[i].EraseFront();
			span* NewSpan = _pool.New();
			NewSpan->Page_Adder = BigSpan->Page_Adder;
			NewSpan->_n = k;
			BigSpan->Page_Adder += k;
			BigSpan->_n -= k;
			_PageList[BigSpan->_n].InsertFront(BigSpan);

			for (Page_t i = NewSpan->Page_Adder; i < NewSpan->Page_Adder + k; i++){
				_PageToSpan.set(i, NewSpan);
			}
			_PageToSpan.set(BigSpan->Page_Adder, BigSpan);
			_PageToSpan.set(BigSpan->Page_Adder + BigSpan->_n - 1, BigSpan);
			return NewSpan;
		}
	}

	void* ptr = SystemAlloc(MaxPage - 1);
	span* BigSpan = _pool.New();
	BigSpan->Page_Adder = (Page_t)ptr >> PageShift;
	BigSpan->_n = MaxPage - 1;
	_PageList[MaxPage - 1].InsertFront(BigSpan);
	return NewSpan(k);
}


span* PageCache::MapObjectToSpan(void* obj){
	span* FindSpan = (span*)_PageToSpan.get((Page_t)obj >> PageShift);
	assert(FindSpan != nullptr);
	return FindSpan;
}

void PageCache::ReleaseSpanToPageCache(span* Span){
	assert(Span!=nullptr);
	if (Span->_n > MaxPage - 1){
		SystemFree((void*)(Span->Page_Adder << PageShift));
		_pool.Delete(Span);
		return;
	}

	while (1){
		span* tmp = (span*)_PageToSpan.get(Span->Page_Adder - 1);
		if (tmp == nullptr){
			break;
		}
		if (tmp->_IsUse == true){
			break;
		}
		if (Span->_n + tmp->_n > MaxPage - 1){
			break;
		}
		Span->Page_Adder = tmp->Page_Adder;
		Span->_n += tmp->_n;
		_PageList[tmp->_n].Erase(tmp);
		_pool.Delete(tmp);
	}

	while (1){
		span* tmp = (span*)_PageToSpan.get(Span->Page_Adder + Span->_n);
		if (tmp == nullptr){
			break;
		}
		if (tmp->_IsUse == true){
			break;
		}
		if (Span->_n + tmp->_n > MaxPage - 1){
			break;
		}
		Span->_n += tmp->_n;
		_PageList[tmp->_n].Erase(tmp);
		_pool.Delete(tmp);
	}

	Span->_IsUse = false;
	_PageList[Span->_n].InsertFront(Span);
	_PageToSpan.set(Span->Page_Adder, Span);
	_PageToSpan.set(Span->Page_Adder + Span->_n - 1, Span);
}
