#include <iostream>
#include "thread_pool.hpp"
int main() {
	xmh::thread_pool Pool(2);
	auto promise = Pool.add_task([](int i) {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		return i + 100;
	}, 10);
	int a = 0;
	auto promise2 = Pool.add_task([](int& i) {
		std::this_thread::sleep_for(std::chrono::seconds(4));
		return i += 100;
	}, a);

	auto promise3 = Pool.add_task([]() {
		std::this_thread::sleep_for(std::chrono::seconds(3));
		std::cout << "OK\n";
	});
	std::cout<< promise.get()<<std::endl;
	std::cout << promise2.get() << std::endl;
	std::cout << "a has modified to " << a << std::endl;
	std::cout << promise3.get() << std::endl;
	std::getchar();
}