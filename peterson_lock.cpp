#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

class PetersonLock
{
	static std::atomic<int> turn;
	static std::atomic<bool> flag[2];
	int self, other;

public:
	PetersonLock(int id)
	{
		self = id;
		other = 1 - self;
	};

	void acquire()
	{
		flag[self].store(true, std::memory_order_release);
		turn.store(self, std::memory_order_relaxed);

		while (flag[other].load(std::memory_order_acquire) &&
			turn.load(std::memory_order_acquire) != other);

		std::atomic_thread_fence(std::memory_order_acquire);
	};

	void release()
	{
		flag[self].store(false, std::memory_order_release);
	};
};

std::atomic<bool> PetersonLock::flag[2] = { false, false };
std::atomic<int> PetersonLock::turn = 0;

int shared_counter_peterson = 0;
void increment_counter_peterson(PetersonLock& lock, int iterations)
{
	for (int i = 0; i < iterations; i++)
	{
		lock.acquire();
		shared_counter_peterson++;
		lock.release();
	}
};

int shared_counter_mutex = 0;
std::mutex mtx;
void increment_counter_mutex(int iterations)
{
	for (int i = 0; i < iterations; i++)
	{
		mtx.lock();
		shared_counter_mutex++;
		mtx.unlock();
	}
};

int main()
{
	using namespace std::chrono;

	const int num_iterations = 1000000;
	PetersonLock lock0(0);
	PetersonLock lock1(1);

	auto start_peterson = high_resolution_clock::now();
	std::thread t0(increment_counter_peterson, std::ref(lock0), num_iterations);
	std::thread t1(increment_counter_peterson, std::ref(lock1), num_iterations);
	t0.join();
	t1.join();
	auto end_peterson = high_resolution_clock::now();
	auto duration_peterson = duration_cast<milliseconds>(end_peterson - start_peterson);

	auto start_mutex = high_resolution_clock::now();
	std::thread t2(increment_counter_mutex, num_iterations);
	std::thread t3(increment_counter_mutex, num_iterations);
	t2.join();
	t3.join();
	auto end_mutex = high_resolution_clock::now();
	auto duration_mutex = duration_cast<milliseconds>(end_mutex - start_mutex);

	std::cout << "PetersonLock time: " << duration_peterson.count() << " ms\n";
	std::cout << "Mutex time:        " << duration_mutex.count() << " ms\n";
	std::cout << "Final Peterson counter: " << shared_counter_peterson << std::endl;
	std::cout << "Final Mutex counter:    " << shared_counter_mutex << std::endl;
	
	return 0;
};