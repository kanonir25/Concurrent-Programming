#include <condition_variable>
#include <mutex>
#include <iostream>
#include <deque>
#include <random>
#include <cmath>
#include <vector>
#include <functional>
#include <thread>
#include <exception>
#include <future>
//made by Осипов Николай 491 гр.

template <class R_V> class Task{
public:
	Task() {}
	Task(std::function<R_V(void)> _function, std::promise<R_V>&& _promis){
		function = _function;
		promise = move(_promis);
	}
public:
	std::function<R_V(void)> function;
	std::promise<R_V> promise;
};

template <class Value, class Container = std::deque<Value>> class thread_safe_queue{
public:
	explicit thread_safe_queue(){
		end_of_work = false;
	}
	thread_safe_queue(thread_safe_queue &) = delete;
	thread_safe_queue& operator = (const thread_safe_queue&) = delete;
	void enqueue(Value item){
		if (end_of_work.load()) //если shutdown был вызван, то producers больше ничего не могут добавлять в очередь, кидают исключение и завершаются
			throw std::exception();
		std::unique_lock<std::mutex> lock(m);
		thread_queue.push_back(std::move(item));
		cv_for_consumer.notify_one();
	}
	void pop(Value& item){
		if (end_of_work.load()) //если shutdown был вызван, то все consumers дорабатывают все элементы очереди, 
			//кидают исключение и завершаются
			throw std::exception();
		std::unique_lock<std::mutex> lock(m);
		while (thread_queue.empty()){
			if (end_of_work && thread_queue.empty()) throw std::exception();
			cv_for_consumer.wait(lock);
		}
		item = std::move(thread_queue.front());
		thread_queue.pop_front();
	}
	void shutdown(){
		end_of_work.store(true);
		cv_for_consumer.notify_all();
	}
private:
	//std::size_t capacity;
	Container thread_queue;
	std::condition_variable cv_for_consumer;
	std::mutex m;
	std::atomic<bool> end_of_work;
};

template <class Value>
class thread_pool {
public:
	explicit thread_pool(std::size_t num_workers){
		this->num_workers = num_workers;
		flag_for_sd.store(false);
		//this->queue() = thread_safe_queue();
		for (size_t i = 0; i < num_workers; ++i) {
			workers.emplace_back(std::thread([this](){
				try{
					Task<Value> task;
					while (1) {
						try {
							try{
								queue.pop(task);
							}
							catch (...) {
								return;
							}
							task.promise.set_value(task.function());
						}
						catch (...) {
							task.promise.set_exception(std::current_exception());
						}
					}
				}
				catch (...) {

				}
			}));
		}

	}
	std::future<Value> submit(std::function<Value()> func) {
		if (!flag_for_sd.load()){
			std::promise<Value> promis;
			std::future<Value> future = promis.get_future();
			Task<Value> task = Task<Value>(func, move(promis));
			try{
				queue.enqueue(move(task));
			}
			catch (...) {

			}
			return move(future);
		}
	else {
		throw std::exception();
	}
}
	void shutdown() {
		flag_for_sd.store(true);
		queue.shutdown();
		for (size_t i = 0; i < num_workers; ++i) {
			workers[i].join();
		}
	}
private:
	std::size_t num_workers;
	std::vector<std::thread> workers;
	thread_safe_queue< Task<Value> > queue;
	std::mutex m;
	std::atomic<bool> flag_for_sd;
};