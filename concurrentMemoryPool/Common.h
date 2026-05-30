#pragma once

#include<iostream>
#include<vector>
#include<unordered_map>

#include<time.h>
#include<assert.h>

#include<thread>
#include<mutex>

#include<algorithm>
#include<atomic>

#include"ObjectPool.h"

using std::cout;
using std::endl;

static const size_t MaxFreeLists = 208;
static const size_t MaxBytes = 256 * 1024;
static const size_t MaxPage = 129;
static const size_t PageShift = 13;

#ifdef _WIN64
	typedef unsigned long long Page_t;
#elif _WIN32
	typedef size_t Page_t;
#else
#endif

#ifdef _WIN32
#include<Windows.h>
#else
#endif

inline static void* SystemAlloc(size_t page)
{
#ifdef _WIN32
		void* ptr = VirtualAlloc(
			0,
			page<<PageShift,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READWRITE
		);
#else
		void* ptr = mmap(
			nullptr,
			page * (1 << 13(),
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			-1,
			0
		);
#endif
		if (ptr == nullptr)
		{
			throw std::bad_alloc();
			exit(-1);
		}
		return ptr;
	}
	inline static void SystemFree(void* ptr){
#ifdef _WIN32
		VirtualFree(ptr, 0, MEM_RELEASE);
#else
#endif
	}

static void*& Next(void* obj){
	return *(void**)obj;
}


class FreeList{
public:
	void Push(void* obj){
		assert(obj);
		Next(obj) = _FreeList;
		_FreeList = obj;
		++_size;
	}

	void* Pop(){
		assert(_FreeList != nullptr);
		void* obj = _FreeList;
		_FreeList = Next(obj);
		--_size;
		return obj;
	}

	void PushRange(void* start, void* end, size_t n){
		Next(end) = _FreeList;
		_FreeList = start;
		_size += n;
	}

	void PopRange(void*& start, void*& end, size_t n){
		assert(n <= _size);
		start = _FreeList;
		end = _FreeList;
		for (size_t i = 0; i < n - 1; i++)
		{
			end = Next(end);
		}
		_FreeList = Next(end);
		Next(end) = nullptr;
		_size -= n;
	}

	bool Empty(){
		return _FreeList == nullptr;
	}

	size_t& MaxSize(){
		return _MaxSize;
	}

	size_t Size(){
		return _size;
	}

private:
	void* _FreeList=nullptr;
	size_t _MaxSize = 1;
	size_t _size = 0;
};

class SizeClass{
public:
	static inline size_t _RoundUp(size_t bytes, size_t align){
		return (bytes + align - 1) & (~(align - 1));
	}

	static inline size_t RoundUp(size_t bytes){
		if (bytes <= 128){
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024){
			return _RoundUp(bytes, 16);
		}
		else if (bytes <= 8 * 1024){
			return _RoundUp(bytes, 128);
		}
		else if (bytes <= 64 * 1024){
			return _RoundUp(bytes, 1024);
		}
		else if (bytes <= 256 * 1024){
			return _RoundUp(bytes, 8 * 1024);
		}
		else{
			return _RoundUp(bytes, 1 << PageShift);
		}
	}

	static inline size_t _Index(size_t bytes, size_t AlignIndex)
{
		return ((bytes + ((size_t)1 << AlignIndex) - 1) >> AlignIndex) - 1;
	}

	static inline size_t Index(size_t bytes){
		if (bytes <= 128){
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024){
			return _Index(bytes, 4);
		}
		else if (bytes <= 8 * 1024){
			return _Index(bytes, 7);
		}
		else if (bytes <= 64 * 1024){
			return _Index(bytes, 10);
		}
		else if (bytes <= 256 * 1024){
			return _Index(bytes, 13);
		}
		else{
			assert(false);
			return -1;
		}
	}

	static size_t NumMoveSize(size_t size){
		assert(size <= MaxBytes);
		size_t num = MaxBytes / size;
		if (num < 2){
			num = 2;
		}
		if (num > 512){
			num = 512;
		}
		return num;
	}

	static size_t NumMovePage(size_t size){
		size_t num = NumMoveSize(size);
		size_t npage = num * size;
		npage >>= PageShift;

		if (npage == 0)
			npage = 1;
		return npage;
	}
};

struct span{
	Page_t Page_Adder = 0;
	size_t _n = 0;
	span* _prev=nullptr;
	span* _next=nullptr;

	size_t _UseCount = 0;
	void* _FreeList = nullptr;

	bool _IsUse = false;
	size_t _FreeKnowSize = 0;
};

class SpanList{
public:
	SpanList(){
		_head = _SpanPool.New();
		_head->_next = _head;
		_head->_prev = _head;
	}

	void InsertFront(span* NewPos){
		Insert(begin(), NewPos);
	}

	void Insert(span* pos, span* NewPos){
		assert(NewPos != nullptr);
		span* prev = pos->_prev;

		prev->_next = NewPos;
		NewPos->_prev = prev;
		NewPos->_next = pos;
		pos->_prev = NewPos;
	}

	span* EraseFront(){
		span* EraseSpan = _head->_next;
		Erase(EraseSpan);
		return EraseSpan;
	}

	void Erase(span* pos){
		assert(pos != nullptr);
		assert(pos != _head);
		span* prev = pos->_prev;
		span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	std::mutex& Mutex(){
		return _mtx;
	}

	span* begin(){
		return _head->_next;
	}

	span* end(){
		return _head;
	}

	bool Empty(){
		return _head->_next == _head;
	}

private:
	span* _head;
	std::mutex _mtx;

	static ObjectPool<span> _SpanPool;
};
