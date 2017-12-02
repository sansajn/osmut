/*! \file sync_queue.hpp */
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

//! thread safe queue implementation
template<typename T>
class sync_queue
{
public:
	sync_queue() {}

	sync_queue(sync_queue const & rhs)
	{
		std::lock_guard<std::mutex> lk{rhs._m};
		_data = rhs._data;
	}

	void push(T new_value)  // TODO: why not const reference, but value ?
	{
		std::lock_guard<std::mutex> lk{_m};
		_data.push(new_value);
		_cond.notify_one();
	}

	void wait_and_pop(T & result)
	{
		std::unique_lock<std::mutex> lk{_m};
		_cond.wait(lk, [this](){return !_data.empty();});
		result = _data.front();
		_data.pop();
	}
	
	template <typename Rep, typename Period>
	bool wait_and_pop(T & result, std::chrono::duration<Rep, Period> const & rel_time)
	{
		std::unique_lock<std::mutex> lk{_m};
		bool timeout = !_cond.wait_for(lk, rel_time, [this](){return !_data.empty();});
		if (timeout)
			return false;
		result = _data.front();
		_data.pop();
		return true;
	}

	std::shared_ptr<T> wait_and_pop()  // TODO: preco nevraciam T ?
	{
		std::unique_lock<std::mutex> lk{_m};
		_cond.wait(lk, [this]{return !_data.empty();});
		std::shared_ptr<T> result{std::make_shared<T>(_data.front())};  // TODO: only for types not throw an exception
		_data.pop();
		return result;
	}
	
	bool try_pop(T & result)
	{
		std::lock_guard<std::mutex> lk{_m};
		if (_data.empty())
			return false;
		result = _data.front();
		_data.pop();
		return true;
	}

	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk{_m};
		if (_data.empty())
			return std::shared_ptr<T>{};
		std::shared_ptr<T> result{std::make_shared<T>(_data.front())};
		_data.pop();
		return result;
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lk{_m};
		return _data.empty();
	}

private:
	std::queue<T> _data;
	mutable std::mutex _m;
	std::condition_variable _cond;
};
