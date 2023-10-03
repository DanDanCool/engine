#include <core/vector.h>
#include <core/memory.h>
#include <core/string.h>
#include <core/table.h>
#include <core/thread.h>
#include <iostream>

using namespace core;

const cstr DIVIDE = "----------------";

struct foostruct {
	foostruct(cref<int> in) : data(in) {}
	~foostruct() {
		std::cout << "foostruct destructor " << data << std::endl;
	}
	int data;
};

struct basicstruct {
	basicstruct(int _a, int _b, int _c, int _d) : a(_a), b(_b), c(_c), d(_d) {}
	int a, b, c, d;
};

void test_thread() {
	std::cout << DIVIDE << "thread" << std::endl;
	ptr<basicstruct> myptr = ptr_create<basicstruct>(1, 2, 3, 4);

	auto coroutine = [](ref<thread>, ptr<void> in) -> int {
		ptr<int> args = in.cast<int>();

		for (int x : range(args.ref())) {
			std::cout << x << std::endl;
		}

		return 0;
	};

	thread mythread = thread(coroutine, ptr_create<int>(5).cast<void>());
}

void test_vector() {
	std::cout << DIVIDE << "vector" << std::endl;
	vector<int> v(0);

	int a = {};

	for (u32 i = 0; i < 32; i++) {
		v.add(i);
	}

	vector<foostruct> scopedv(0);
	for (int i = 0; i < 10; i++) {
		scopedv.add(foostruct(i));
	}

	for (auto x : reverse(scopedv)) {
		std::cout << x.data << std::endl;
	}
}

void test_string() {
	std::cout << DIVIDE << "string" << std::endl;
	string s("hello world!");
	for (auto x : forward(s)) {
		std::cout << x << std::endl;
	}
}

void test_table() {
	std::cout << DIVIDE << "table" << std::endl;
	table<i32, i32> mytable;
	for (i32 x : range(128)) {
		mytable[x] = x;
	}

	i64 sum = 0;
	for (i32 x : range(128)) {
		std::cout << x << " " << mytable[x] << std::endl;
	}

	table<string, i32> words;
	const vector<string> sentence = { "the", "quick", "brown", "fox", "jumps", "over", "the2", "lazy", "dog" };
	for (int i : range(sentence.size)) {
		words[sentence[i]] = i;
	}

	words.del("quick");
	words.del("brown");
	words.del("lazy");

	for (auto& [key, val] : words) {
		std::cout << key << " " << val << std::endl;
	}
}

void test_ptr() {
	std::cout << DIVIDE << "ptr" << std::endl;
	ptr<string> scope = ptr_create<string>("hello world");
	ptr<int> a = ptr_create<int>(4);
	std::cout << scope->data << std::endl;
	std::cout << a.ref() << std::endl;

	auto mylambda = [](ptr<int> in) {
		std::cout << in.ref() << std::endl;
	};

	ptr<int> foo = ptr_create<int>(69);
	mylambda(foo);

	vector<ptr<int>> indirect(0);

	for (int i : range(20)) {
		indirect.add(ptr_create<int>(i));
	}

	for (cref<ptr<int>> p : reverse(indirect)) {
		std::cout << p.ref() << std::endl;
	}
}

int main() {
	test_thread();
	test_vector();
	test_string();
	test_table();
	test_ptr();
}
