#include <vector>
#include <forward_list>
#include <mutex>
#include <atomic>
#include <iostream>

template <class Value, class Hash = std::hash<Value> >
class striped_hash_set {
public:
	explicit striped_hash_set(size_t n_stripes){
		num_stripes = n_stripes;
		mtx = std::vector<std::mutex>(num_stripes);
		buckets = 4 * num_stripes;
		hash_table = std::vector<std::forward_list<Value> >(buckets);
		load_factor = 0.8;
		growth_factor = 15;
		amount_elements.store(0);
	}	
	size_t get_bucket_index(size_t hash_value)const {
		return hash_value % buckets;
	}
	size_t get_stripe_index(size_t hash_value)const{
		return hash_value % num_stripes;
	}
	void add(const Value& v){
		Hash hsh;
		int hash_val = hsh(v);
		std::unique_lock<std::mutex> lock(mtx[get_stripe_index(hash_val)]);
		hash_table[get_bucket_index(hash_val)].push_front(v);
		amount_elements.fetch_add(1);
		if ((double)amount_elements.load() / buckets > load_factor){
			int old_amount_of_buckets = buckets;
			lock.unlock();
			reHash(old_amount_of_buckets);
		}
	}
	void remove(const Value& v){
		if (contains(v))
			return;
		Hash hsh;
		int hash_val = hsh(v);
		std::unique_lock<std::mutex> lock(mtx[get_stripe_index(hash_val)]);
		size_t cur_bucket = get_bucket_index(hash_val);
		hash_table[cur_bucket].remove(v);
		amount_elements.fetch_sub(1);	
	}

	bool contains(const Value& v){
		Hash hsh;
		int hash_val = hsh(v);
		std::unique_lock<std::mutex> lock(mtx[get_stripe_index(hash_val)]);
		int cur_bucket = get_bucket_index(hash_val);
		for (auto & it : hash_table[cur_bucket]){
			if (it == v)
				return true;
		}
		return false;
	}

	void reHash(size_t old_buckets){
		Hash hsh;
		std::vector<std::unique_lock<std::mutex> > local_locks;
		local_locks.emplace_back(mtx[0]);
		if (old_buckets != buckets)
			return;
		for (size_t i = 1; i < num_stripes; i++){
			local_locks.emplace_back(mtx[i]);
		}
		size_t new_num_buckets = buckets * growth_factor;
		std::vector<std::forward_list<Value> > new_hash_table(new_num_buckets);

		for (size_t ind = 0; ind < buckets; ind++){
			while (!hash_table[ind].empty()){
				Value item = hash_table[ind].front();
				hash_table[ind].pop_front();
				int hash_val = hsh(item);
				new_hash_table[hash_val % new_num_buckets].push_front(item);
			}
		}
		hash_table.swap(new_hash_table);
	}
private:
	std::vector<std::forward_list<Value> > hash_table;
	size_t num_stripes;
	std::vector<std::mutex> mtx;
	size_t buckets;
	double growth_factor;
	double load_factor;
	std::atomic<int> amount_elements;
};