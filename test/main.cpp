#include <core/vector.h>
#include <core/memory.h>
#include <core/string.h>
#include <core/table.h>
#include <core/thread.h>
#include <core/lock.h>
#include <core/log.h>
#include <core/set.h>
#include <core/atom.h>
#include <core/core.h>

using namespace core;

const cstr DIVIDE = "----------------";

struct foostruct {
	foostruct(cref<int> in) : data(in) {}
	~foostruct() {
		LOG_INFO("foostruct destructor %", data);
	}
	int data;
};

struct basicstruct {
	basicstruct(int _a, int _b, int _c, int _d) : a(_a), b(_b), c(_c), d(_d) {}
	int a, b, c, d;
};

void test_assert() {
	LOG_INFO("% assert", DIVIDE);
	JOLLY_ASSERT(3 == 4, "3 does not equal 4!");
}

void test_thread() {
	LOG_INFO("% thread", DIVIDE);
	ptr<basicstruct> myptr = ptr_create<basicstruct>(1, 2, 3, 4);

	auto coroutine = [](ref<thread>, ptr<void> in) -> int {
		ptr<int> args = in.cast<int>();

		for (int x : range(args.ref())) {
			LOG_INFO("%", x);
		}

		return 0;
	};

	thread mythread = thread(coroutine, ptr_create<int>(5).cast<void>());
	LOG_INFO("thread joined");
}

void test_atomics() {
	LOG_INFO("% atomics", DIVIDE);
	atom<u32> counter(0);
	struct my_data {
		my_data(ref<atom<u32>> in) : counter(in) {}
		ref<atom<u32>> counter;
	};

	auto coroutine = [](ref<thread>, ptr<void> in) {
		ptr<my_data> args = in.cast<my_data>();

		for (int x : range(10000)) {
			u32 val = args->counter.get(memory_order_relaxed);
			while (!args->counter.cmpxchg(val, val + 1, memory_order_release, memory_order_relaxed));
		}

		return 0;
	};

	vector<thread> threads(10);
	for (int i : range(10)) {
		threads[i] = thread(coroutine, ptr_create<my_data>(counter).cast<void>());
	}

	for (int i : range(10)) {
		threads[i].join();
	}

	LOG_INFO("counter value: %", counter.get(memory_order_acquire));
}

void test_vector() {
	LOG_INFO("% vector", DIVIDE);
	vector<int> v(0);

	int a = {};

	for (u32 i = 0; i < 32; i++) {
		v.add(i);
	}

	vector<foostruct> scopedv(0);
	for (int i = 0; i < 5; i++) {
		scopedv.add(foostruct(i));
	}

	for (auto& x : reverse(scopedv)) {
		LOG_INFO("%", x.data);
	}
}

void test_string() {
	LOG_INFO("% string", DIVIDE);
	string s("hello world!");
	for (auto x : forward(s)) {
		LOG_INFO("%", x);
	}
}

void test_table() {
	LOG_INFO("% table", DIVIDE);
	table<i32, i32> mytable;
	for (i32 x : range(128)) {
		mytable[x] = x;
	}

	i64 sum = 0;
	for (i32 x : range(128)) {
		JOLLY_ASSERT(x == mytable[x]);
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
		LOG_INFO("% %", key, val);
	}
}

void test_ptr() {
	LOG_INFO("% ptr", DIVIDE);
	ptr<string> scope = ptr_create<string>("hello world");
	ptr<int> a = ptr_create<int>(4);
	LOG_INFO("%, %", scope->data, a.ref());

	auto mylambda = [](ptr<int> in) {
		LOG_INFO("%", in.ref());
	};

	ptr<int> foo = ptr_create<int>(69);
	mylambda(foo);

	vector<ptr<int>> indirect(0);

	for (int i : range(10)) {
		indirect.add(ptr_create<int>(i));
	}

	for (cref<ptr<int>> p : reverse(indirect)) {
		LOG_INFO("%", p.ref());
	}
}

void test_log() {
	LOG_INFO("% log", DIVIDE);
	auto fmt = format_string("% % %", 5, 4, "hello");
	LOG_INFO(fmt.data);
	LOG_INFO("test test % % %", 420, -69, "hello");
	LOG_INFO("this is info");
	LOG_WARN("this is a warning");
	LOG_CRIT("this is a critical error");
}

void test_set() {
	LOG_INFO("% set", DIVIDE);

	set<int> smallset = { 2, 4, 6, 8, 2 };
	for (int x : smallset) {
		LOG_INFO("smallset %", x);
	}

	set<int> myset;
	for (int i : range(50))
		myset.add(i);

	LOG_INFO("%: %", 0, myset.has(0));
	myset.add(0);

	for (int i : range(10, 40))
		myset.del(i);

	for (int x : myset) {
		LOG_INFO("%", x);
	}
}

 void test_mutex() {
	LOG_INFO("% mutex", DIVIDE);

	{
		// win32 behaviour: should return true twice
		LOG_INFO("test mutex");
		mutex lock;
		bool acquired = lock.tryacquire();
		LOG_INFO("%", acquired);
		acquired = lock.tryacquire();
		LOG_INFO("%", acquired);

		lock.release();
	}

	{
		// win32 behaviour: return true than false
		LOG_INFO("test semaphore");
		semaphore lock(1, 1);
		bool acquired = lock.tryacquire();
		LOG_INFO("%", acquired);
		acquired = lock.tryacquire();
		LOG_INFO("%", acquired);

		lock.release();
	}
 }

int main() {
	test_assert();
	test_thread();
	test_atomics();
	test_vector();
	test_string();
	test_table();
	test_ptr();
	test_log();
	test_set();
	test_mutex();
}
