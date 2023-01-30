#include <iostream>
#include <thread>
#include <vector>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>

template <typename T>
class BlockingQueue {
public:
	void Push(const T& elem) {
		std::lock_guard lk(m_);
		d_.push_back(elem);
		cv_.notify_one();
	}
	std::optional<T> Get() {
		std::unique_lock lk(m_);
		while (d_.size() == 0 && !is_closed_) {
			cv_.wait(lk);
		}
		if (d_.size() != 0) {
			auto temp = std::move(d_.front());
			d_.pop_front();
			return temp;
		}
		return std::nullopt;
	}
	void Close(bool delete_all) {
		std::lock_guard lk(m_);
		is_closed_ = true;
		if (delete_all) {
			d_.clear();
		}
		cv_.notify_all();
	}
private:
	std::deque<T> d_;
	std::mutex m_;
	std::condition_variable cv_;
	bool is_closed_{false};
};



namespace tp{
	class ThreadPool;
	thread_local ThreadPool* tp_ptr{nullptr};
	class ThreadPool {
	public:
		ThreadPool(size_t size) {
			for (size_t i = 0; i < size; ++i) {
				workers_.emplace_back([&]{
					RunWorker();
				});
			}
		}
		void Submit(const std::function<void()> &fn) {
			task_q_.Push(fn);
		}
		void Join() {
			task_q_.Close(false);
			for (auto& th : workers_) {
				th.join();
			}

		}

		void Stop() {
			task_q_.Close(true);
			Join();
		}


	private:
		void RunWorker() {
			tp_ptr = this;
			while (auto task = task_q_.Get()){
				(*task)();
			}
		}
		BlockingQueue<std::function<void()>> task_q_;
		std::vector<std::thread> workers_;

	};

	inline ThreadPool* Current() {
		return tp_ptr;
	}
};
