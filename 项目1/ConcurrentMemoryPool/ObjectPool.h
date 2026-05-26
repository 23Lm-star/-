#pragma once

#include"Common.h"

inline static void* SystemAlloc(size_t page);
static void*& Next(void* obj);


template<class T>
class ObjectPool{
public:
	T* New(){
		T* object = nullptr;
		if (_FreeList){
			object = (T*)_FreeList;
			_FreeList = Next(_FreeList);
		}
		else{
			size_t ObjectByte = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			if(_MemoryByte< ObjectByte){
				_MemoryByte = 1024 * 1024;
				_Memory = (char*)SystemAlloc(_MemoryByte >> 13);
			}
			object = (T*)_Memory;
			_Memory += ObjectByte;
			_MemoryByte -= ObjectByte;
		}
		new(object)T;
		return object;
	}

	void Delete(T* object){
		object->~T();

		Next(object) = _FreeList;
		_FreeList = object;
	}
private:
	char* _Memory = nullptr;
	size_t _MemoryByte = 0;
	void* _FreeList = nullptr;

};
