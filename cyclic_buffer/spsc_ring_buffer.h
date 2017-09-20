#include <atomic>
#include <vector>
#include <climits>
#define index_t int
#define NULL_ELEMENT INT_MAX

template <class Value>
class spsc_ring_buffer {
public:
	explicit spsc_ring_buffer(size_t _capacity){
		capacity = _capacity + 1;
		buf = std::vector<Value>(capacity);
		head.store(0);
		tail.store(0);
	}
	bool enqueue(Value v){
		index_t curTail = tail.load(std::memory_order_acquire);
		index_t curHead = head.load(std::memory_order_acquire);
		if ((curTail + 1) % capacity != curHead){
			buf[curTail] = v;
			index_t nextTail = (curTail + 1) % capacity;
			tail.store(nextTail, std::memory_order_release);
			return true;
		}
		return false;
	}
	bool dequeue(Value& v){
		index_t curHead = head.load(std::memory_order_acquire);
		index_t curTail = tail.load(std::memory_order_acquire);
		if (curHead != curTail){
			v = buf[curHead];
			head.store((curHead + 1) % capacity, std::memory_order_release);
			return true;
		}
		return false;
	}
private:
	size_t capacity;
	std::vector<Value> buf;
	std::atomic<index_t> head;
	std::atomic<index_t> tail;

};