#include <condition_variable>
#include <mutex>
#include <iostream>
#include <deque>
#include <random>
#include <cmath>
#include <exception>
//made by Осипов Николай 491 гр.

template <class Value, class Container = std::deque<Value>> class thread_safe_queue{
public:
	explicit thread_safe_queue(std::size_t capacity){
		this->capacity = capacity;
		end_of_work = false;
	}
	thread_safe_queue(thread_safe_queue &) = delete;
	thread_safe_queue& operator = (const thread_safe_queue&) = delete;
	void enqueue(Value item){
		std::unique_lock<std::mutex> lock(m);
		if (end_of_work) //если shutdown был вызван, то producers больше ничего не могут добавлять в очередь, кидают исключение и завершаются
			throw std::exception();
		while (thread_queue.size() >= capacity){
			cv_for_producer.wait(lock);
		}
		thread_queue.push_back(std::move(item));
		cv_for_consumer.notify_one();
	}
	void pop(Value& item){
		std::unique_lock<std::mutex> lock(m);
		if (end_of_work && thread_queue.empty()) //если shutdown был вызван, то все consumers дорабатывают все элементы очереди, 
			//кидают исключение и завершаются
			throw std::exception();
		while (thread_queue.empty()){
			cv_for_consumer.wait(lock);
		}
		item = std::move(thread_queue.front());
		thread_queue.pop_front();
		cv_for_producer.notify_one();
	}
	void shutdown(){
		std::unique_lock<std::mutex> lock(m);
		end_of_work = true;
	}
private:
	std::size_t capacity;
	Container thread_queue;
	std::condition_variable cv_for_consumer;
	std::mutex m;
	bool end_of_work;
	std::condition_variable cv_for_producer;
};

