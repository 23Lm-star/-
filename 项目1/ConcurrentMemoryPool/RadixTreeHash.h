#pragma once

#include"Common.h"


template <int BITS>
class TCMalloc_PageMap1
{
private:
	static const int LENGTH = 1 << BITS;
	void** array_;

public:
	typedef uintptr_t Number;
	TCMalloc_PageMap1()
	{
		size_t size = sizeof(void*) << BITS;
		size_t bytes = SizeClass::RoundUp(size);
		array_ = (void**)SystemAlloc(bytes>>PageShift);
		memset(array_, 0, sizeof(void*) << BITS);
	}

	void* get(Number k) const
	{
		if ((k >> BITS) > 0)
		{
			return NULL;
		}
		return array_[k];
	}

	void set(Number k, void* v)
	{
		array_[k] = v;
	}
};

template <int BITS>
class TCMalloc_PageMap2 {
private:
	static const int ROOT_BITS = 5;
	static const int ROOT_LENGTH = 1 << ROOT_BITS;
	static const int LEAF_BITS = BITS - ROOT_BITS;
	static const int LEAF_LENGTH = 1 << LEAF_BITS;

	struct Leaf {
		void* values[LEAF_LENGTH];
	};
	Leaf* root_[ROOT_LENGTH];

public:
	typedef uintptr_t Number;

	explicit TCMalloc_PageMap2()
	{
		memset(root_, 0, sizeof(root_));
		Ensure(0, 1 << BITS);
	}

	bool Ensure(Number start, size_t n)
	{
		for (Number key = start; key <= start + n - 1;)
		{
			const Number i1 = key >> LEAF_BITS;
			if (root_[i1] == NULL) {
				static ObjectPool<Leaf> LeafPool;
				Leaf* leaf = LeafPool.New();
				memset(leaf, 0, sizeof(*leaf));
				root_[i1] = leaf;
			}
			key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;
		}
		return true;
	}

	void* get(Number k) const {
		const Number i1 = k >> LEAF_BITS;
		const Number i2 = k & (LEAF_LENGTH - 1);
		if ((k >> BITS) > 0 || root_[i1] == NULL) {
			return NULL;
		}
		return root_[i1]->values[i2];
	}

	void set(Number k, void* v) {
		const Number i1 = k >> LEAF_BITS;
		const Number i2 = k & (LEAF_LENGTH - 1);
		assert(i1 < ROOT_LENGTH);
		root_[i1]->values[i2] = v;
	}
};

template <int BITS>
class TCMalloc_PageMap3 {
private:
	static const int INTERIOR_BITS = (BITS + 2) / 3;
	static const int INTERIOR_LENGTH = 1 << INTERIOR_BITS;
	static const int LEAF_BITS = BITS - 2 * INTERIOR_BITS;
	static const int LEAF_LENGTH = 1 << LEAF_BITS;

	struct Node {
		Node* ptrs[INTERIOR_LENGTH];
	};

	struct Leaf {
		void* values[LEAF_LENGTH];
	};

	Node* root_;

	Node* NewNode() {
		static ObjectPool<Node> ObjectNode;
		Node* result = ObjectNode.New();

		if (result != NULL) {
			memset(result, 0, sizeof(*result));
		}
		return result;
	}
public:
	typedef uintptr_t Number;

	explicit TCMalloc_PageMap3() {
		root_ = NewNode();
	}

	void* get(Number k) const {
		const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
		const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1);
		const Number i3 = k & (LEAF_LENGTH - 1);
		if ((k >> BITS) > 0 || root_->ptrs[i1] == NULL || root_->ptrs[i1]->ptrs[i2] == NULL){
			return NULL;
		}
		return reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3];
	}

	void set(Number k, void* v) {
		assert(k >> BITS == 0);
		const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
		const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1);
		const Number i3 = k & (LEAF_LENGTH - 1);
		Ensure(k, 1);
		reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3] = v;
	}

	bool Ensure(Number start, size_t n) {
		for (Number key = start; key <= start + n - 1;) {
			const Number i1 = key >> (LEAF_BITS + INTERIOR_BITS);
			const Number i2 = (key >> LEAF_BITS) & (INTERIOR_LENGTH - 1);
			if (i1 >= INTERIOR_LENGTH || i2 >= INTERIOR_LENGTH){
				return false;
			}

			if (root_->ptrs[i1] == NULL) {
				Node* n = NewNode();
				if (n == NULL) return false;
				root_->ptrs[i1] = n;
			}

			if (root_->ptrs[i1]->ptrs[i2] == NULL) {
				static ObjectPool<Leaf> ObjectLeaf;
				Leaf* leaf = ObjectLeaf.New();

				memset(leaf, 0, sizeof(*leaf));
				root_->ptrs[i1]->ptrs[i2] = reinterpret_cast<Node*>(leaf);
			}
			key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;
		}
		return true;
	}
};
