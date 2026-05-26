struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;
	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};

void test1()
{
	std::vector<void*> v;
	for (int i = 0; i < 5; ++i)
	{
		void* ptr=ConCurrentAlloc(5);
		v.push_back(ptr);
	}

}

void test2()
{
	std::vector<void*> v;
	for (int i = 0; i < 5; ++i)
	{
		void* ptr = ConCurrentAlloc(6);
		v.push_back(ptr);
	}
}

void TestCCA()
{
	std::thread t1(test1);
	t1.join();
	std::thread t2(test2);
	t2.join();
}

void TestAlloc()
{
	void* p1 = ConCurrentAlloc(2);
	void* p2 = ConCurrentAlloc(3);
	void* p3 = ConCurrentAlloc(5);
	void* p4 = ConCurrentAlloc(8);
	cout << p1 << ' ' << p2 << ' ' << p3 << ' ' << p4 << endl;
}

void TestDeAlloc()
{
	void* p1 = ConCurrentAlloc(8);
	void* p2 = ConCurrentAlloc(6);
	void* p3 = ConCurrentAlloc(3);
	ConCurrentFree(p1);
	ConCurrentFree(p2);
	ConCurrentFree(p3);
}

void TestMoreBytes()
{
	void* p1 = ConCurrentAlloc(257 * 1024);
	ConCurrentFree(p1);

	void* p2 = ConCurrentAlloc(129 * 8 * 1024);
	ConCurrentFree(p2);
}

int main()
{
	cout << "=== Test Alloc ===" << endl;
	TestAlloc();

	cout << "\n=== Test DeAlloc ===" << endl;
	TestDeAlloc();

	cout << "\n=== Test Concurrent Alloc ===" << endl;
	TestCCA();

	cout << "\n=== Test Big Memory Alloc ===" << endl;
	TestMoreBytes();

	return 0;
}
