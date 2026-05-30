#include"ConcurrentAlloc.h"

void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&, k]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					v.push_back(malloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
			});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	printf("%zu个工作线程执行%zu轮次，每轮次malloc %zu次: 耗时：%zu ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%zu个工作线程执行%zu轮次，每轮次free %zu次: 耗时：%zu ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%zu个工作线程malloc&free %zu次，总计耗时：%zu ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}


void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					v.push_back(ConCurrentAlloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					ConCurrentFree(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
			});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	printf("%zu个工作线程执行%zu轮次，每轮次concurrent alloc %zu次: 耗时：%zu ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%zu个工作线程执行%zu轮次，每轮次concurrent dealloc %zu次: 耗时：%zu ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%zu个工作线程concurrent alloc&dealloc %zu次，总计耗时：%zu ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

int main()
{
	size_t n = 1000;
	cout << "==========================================================" << endl;
	BenchmarkConcurrentMalloc(n, 4, 10);
	cout << endl << endl;

	BenchmarkMalloc(n, 4, 10);
	cout << "==========================================================" << endl;

	return 0;
}
