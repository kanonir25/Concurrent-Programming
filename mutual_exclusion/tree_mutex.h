#include <atomic>
#include <vector>
#include <cmath>
#include <iostream>
#include <array>
#include <thread>

class mutexPeterson{
public:
	mutexPeterson(){
		want[0] = false;
		want[1] = false;
		victim = 0;
	}
	void lock(std::size_t thread_index){
		want[thread_index] = true;
		victim = (int)thread_index;
		while ((want[1 - thread_index] == true) && (victim == (int)thread_index))
			std::this_thread::yield();
	}
	void unlock(std::size_t thread_index){
		want[thread_index] = false;
	}
private:
	std::atomic<bool> want[2];
	std::atomic_int victim;
};
size_t findNearestDegreeTwo(size_t num){
	size_t cur = 1;
	while (cur < num)
		cur *= 2;
	return cur;
}

struct own_Peterson_mutex{
	int owner;
	mutexPeterson mtx;
};

class tree_mutex{
public:
	tree_mutex(size_t _num_threads){
		num_threads = _num_threads;
		num_mtx = findNearestDegreeTwo(num_threads) - 1;
		startIndex = num_mtx;
		mutex = std::vector<own_Peterson_mutex>(num_mtx);
		for (auto &t : mutex){
			t.owner = -1;
		}
	}
	void lock(int thread_index){
		if (num_threads <= 1)
			return;
		int cur_index = startIndex + thread_index;
		while (cur_index != 0){
			int number_in_mutex = cur_index % 2;
			cur_index = (cur_index - 1) / 2;
			mutex[cur_index].mtx.lock(number_in_mutex);
			mutex[cur_index].owner = thread_index;
		//	cur_index /= cur_index;
		}
	}
	void unlock(int thread_index){
		if (num_threads <= 1)
			return;
		int cur_index = 0;
		int leftSon, rightSon;
		while (cur_index < startIndex){
			leftSon = cur_index * 2 + 1;
			rightSon = leftSon + 1;
			int index_for_free_mutex;
			if (leftSon >= startIndex)
				index_for_free_mutex = thread_index + startIndex;
			else{
				if (mutex[leftSon].owner == (int)thread_index)
					index_for_free_mutex = leftSon;
				else
					index_for_free_mutex = rightSon;
			}
			mutex[cur_index].owner = -1;
			mutex[cur_index].mtx.unlock(index_for_free_mutex % 2);
			cur_index = index_for_free_mutex;
		}
	}
private:
	std::vector<own_Peterson_mutex> mutex;
	size_t num_threads;
	size_t num_mtx;
	int startIndex;
};
