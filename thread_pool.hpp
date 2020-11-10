#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <tuple>
namespace xmh {
	struct return_void {
		operator bool() {
			return accomplished;
		}
		bool accomplished = false;
	};
	class thread_pool {
	public:
		thread_pool(std::size_t size) :thread_size_(size) {
			init();
		}
		thread_pool(thread_pool const&) = delete;
		void init() {
			for (int i = 0; i < thread_size_; i++) {
				thread_pool_.emplace_back(std::make_unique<std::thread>([this]() {
					while (!off_) {
						std::unique_lock<std::mutex> lock(task_mutex_);
						condition_variable_.wait(lock, [this]() {
							return tasks_.empty() == false || off_ == true;
							});
						if (off_ == true) {
							break;
						}
						auto task = std::move(tasks_.front());
						tasks_.pop();
						task();
					}
					}));
			}
		}
	public:
		template<typename Function, typename...Params>
		std::enable_if_t<!std::is_same_v<void, decltype(std::declval<Function>()(std::declval<Params>()...))>, std::future<decltype(std::declval<Function>()(std::declval<Params>()...))>>  add_task(Function&& function, Params&&...args) {
			std::unique_lock<std::mutex> lock(task_mutex_);
			auto tp = std::tuple<Params...>(std::forward<Params>(args)...);
			auto indexs = std::make_index_sequence<sizeof...(Params)>{};
			using return_type = decltype(function(std::forward<Params>(args)...));
			auto sp = std::make_shared<std::promise<return_type>>();
			auto task = [function, tp, indexs, sp]() {
				decomposition_tuple_0(sp, function, tp, indexs);
			};
			tasks_.emplace(std::move(task));
			return  sp->get_future();
		}
		template<typename Function, typename...Params>
		std::enable_if_t<std::is_same_v<void, decltype(std::declval<Function>()(std::declval<Params>()...))>, std::future<return_void>> add_task(Function&& function, Params&&...args) {
			std::unique_lock<std::mutex> lock(task_mutex_);
			auto sp = std::make_shared<std::promise<return_void>>();
			auto tp = std::tuple<Params...>(std::forward<Params>(args)...);
			auto indexs = std::make_index_sequence<sizeof...(Params)>{};
			auto task = [function, tp, indexs, sp]() {
				decomposition_tuple_1(sp, function, tp, indexs);
			};
			tasks_.emplace(std::move(task));
			return  sp->get_future();
		}
	private:
		template<typename Smarter, typename Function, typename Tuple, std::size_t...Indexs>
		static void decomposition_tuple_0(Smarter&& smart, Function&& function, Tuple&& tup, std::index_sequence<Indexs...>) {
			smart->set_value(function(std::get<Indexs>(tup)...));
		}
		template<typename Smarter, typename Function, typename Tuple, std::size_t...Indexs>
		static void decomposition_tuple_1(Smarter&& smart, Function&& function, Tuple&& tup, std::index_sequence<Indexs...>) {
			function(std::get<Indexs>(tup)...);
			smart->set_value(return_void{ true });
		}
	public:
		~thread_pool() {
			off_ = true;
			condition_variable_.notify_all();
			for (auto&& iter : thread_pool_) {
				if (iter && iter->joinable()) {
					iter->join();
				}
			}
		}
	private:
		//std::unique_ptr<std::thread> self_thread_;
		std::size_t thread_size_;
		std::vector<std::unique_ptr<std::thread>> thread_pool_;
		std::queue<std::function<void()>> tasks_;
		std::atomic_bool off_{ false };
		std::mutex task_mutex_;
		std::condition_variable condition_variable_;
	};
}
