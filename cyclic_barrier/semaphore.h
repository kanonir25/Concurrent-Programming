#include <atomic>
#include <condition_variable>
#include <mutex>
enum steps{ left, right };

class CVrobot{
public:
	CVrobot(){
		step = left;
	}
	void step_right(){
		std::unique_lock<std::mutex> lock(mtx);
		while (step == left){
			cv_for_right_step.wait(lock);
		}
		step = left;
		cv_for_left_step.notify_one();
	}
	void step_left(){
		std::unique_lock <std::mutex> lock(mtx);
		while (step == right){
			cv_for_left_step.wait(lock);
		}
		step = right;
		cv_for_right_step.notify_one();
	}
private:
	std::mutex mtx;
	steps step;
	std::condition_variable cv_for_left_step;
	std::condition_variable cv_for_right_step;
};

class semaphore{
public:
	semaphore(){};
	semaphore(int beg){
		counter = beg;
	}
	semaphore(const semaphore& s){
		counter = s.counter;
	}
	semaphore & operator = (const semaphore & s){
		this->counter = s.counter;
		return *this;
	}
	void wait(){
		std::unique_lock<std::mutex> lock(mtx);
		while (!counter)
			cv_for_semaphore.wait(lock);
		--counter;
	}
	void signal(){
		std::unique_lock<std::mutex> lock(mtx);
		++counter;
		cv_for_semaphore.notify_one();
	}
private:
	std::mutex mtx;
	std::condition_variable cv_for_semaphore;
	int counter;
};

class Srobot{
public:
	Srobot(){
		sem_on_right_leg = semaphore(0);
		sem_on_left_leg = semaphore(1);
	}
	void left(){
		std::unique_lock<std::mutex> lock(mtx);
		sem_on_left_leg.wait();
		sem_on_right_leg.signal();
	}
	void right(){
		std::unique_lock<std::mutex> lock(mtx);
		sem_on_right_leg.wait();
		sem_on_left_leg.signal();
	}
private:
	std::mutex mtx;
	semaphore sem_on_left_leg;
	semaphore sem_on_right_leg;
};