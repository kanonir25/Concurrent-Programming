#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
//made by Osipov Nickolay from 491

class barrier {
public:
	explicit barrier(size_t num_threads){
		this->num_threads = num_threads;
		input = 0;
		output = this->num_threads;
	}
	void enter(){
		std::unique_lock<std::mutex> lock(mtx);
		if (output && output != (int)num_threads)
			cv_for_input.wait(lock);
		output = 0;
		++input;
		if (input != (int)num_threads)
			cv_for_output.wait(lock);
		input = 0;
		cv_for_output.notify_all();
		++output;
		if (output == (int)num_threads)
			cv_for_input.notify_all();
	}
public:
	std::size_t num_threads;
	int input;
	int output;
	std::mutex mtx;
	std::condition_variable cv_for_output;
	std::condition_variable cv_for_input;
};