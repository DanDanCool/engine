#include <core/core.h>

import core.vector;
import core.memory;
import core.string;
import core.table;
import core.thread;
import core.lock;
import core.log;
import core.set;
import core.atom;
import core.iterator;
import jolly.ecs;

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
	//JOLLY_ASSERT(3 == 4, "3 does not equal 4!");
}

void test_thread() {
	LOG_INFO("% thread", DIVIDE);
	ptr<basicstruct> myptr = ptr_create<basicstruct>(1, 2, 3, 4);

	auto coroutine = [](ref<thread>, ptr<void>&& in) -> int {
		ptr<int> args = in.cast<int>();

		for (int x : range(args.get())) {
			LOG_INFO("%", x);
		}

		return 0;
	};

	auto args = ptr_create<int>(5);
	thread mythread = thread(coroutine, forward_data(args.cast<void>()));
	LOG_INFO("thread joined");
}

void test_atomics() {
	LOG_INFO("% atomics", DIVIDE);
	atom<u32> counter(0);
	struct my_data {
		my_data(ref<atom<u32>> in) : counter(in) {}
		ref<atom<u32>> counter;
	};

	auto coroutine = [](ref<thread>, ptr<void>&& in) {
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

	for (i32 x : range(128)) {
		JOLLY_ASSERT(x == mytable[x]);
	}

	for (i32 x : range(128)) {
		mytable.del(x);
	}

	table<string, i32> words;
	const vector<string> sentence = { "the", "quick", "brown", "fox", "jumps", "over", "the2", "lazy", "dog" };
	for (int i : range(sentence.size)) {
		words[sentence[i]] = i;
	}

	words.del("quick");
	words.del("brown");
	words.del("lazy");

	for (auto [key, val] : words) {
		LOG_INFO("% %", key, val);
	}
}

void test_ptr() {
	LOG_INFO("% ptr", DIVIDE);
	ptr<string> scope = ptr_create<string>("hello world");
	ptr<int> a = ptr_create<int>(4);
	LOG_INFO("%, %", scope->data, a.get());

	auto mylambda = [](cref<ptr<int>> in) {
		LOG_INFO("%", in.get());
	};

	ptr<int> foo = ptr_create<int>(69);
	mylambda(foo);

	vector<ptr<int>> indirect(0);

	for (int i : range(10)) {
		indirect.add(ptr_create<int>(i));
	}

	for (cref<ptr<int>> p : reverse(indirect)) {
		LOG_INFO("%", p.get());
	}
}

void test_log() {
	LOG_INFO("% log", DIVIDE);
	auto fmt = format_string("% % %", 5, 4, "hello");
	LOG_INFO(fmt);
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

struct test_component1 {
	int a, b, c;
};

struct test_component2 {
	string name;
	int size;
};

void test_ecs() {
	LOG_INFO("% ecs", DIVIDE);
	jolly::ecs ecs;
	jolly::e_id e = ecs.create();
	ecs.add<test_component1>(e, test_component1{1, 2, 3});
	ecs.add<test_component2>(e, test_component2{"hello", 69});

	e = ecs.create();
	ecs.add<test_component1>(e, test_component1{1, 2, 3});
	ecs.add<test_component2>(e, test_component2{"world", 42});
	ecs.del<test_component2>(e);

	e = ecs.create();
	ecs.add<test_component1>(e, test_component1{4, 2, 3});
	ecs.add<test_component2>(e, test_component2{"quick", 42});

	ecs.del<test_component1>(jolly::e_id{0});

	LOG_INFO("ecs.view<test_component1>");
	for (auto [entity, component] : ecs.view<test_component1>()) {
		LOG_INFO("entity: %, a: % b: % c: %", entity._id, component->a, component->b, component->c);
	}

	LOG_INFO("ecs.view<test_component2>");
	for (auto [entity, component] : ecs.view<test_component2>()) {
		LOG_INFO("entity: %, name: %, size: %", entity._id, component->name, component->size);
	}

	LOG_INFO("ecs.group");
	for (auto [entity, components] : ecs.group<test_component1, test_component2>()) {
		auto [test1, test2] = components;
		LOG_INFO("entity: %, name: %, a: %", entity._id, test2->name, test1->a);
	}

	LOG_INFO("ecs.group");
	ecs.add<test_component2>(jolly::e_id{1}, test_component2{"brown", 44});
	for (auto [entity, components] : ecs.group<test_component1, test_component2>()) {
		auto [test1, test2] = components;

		test_component1& t1r = test1;
		LOG_INFO("entity: %, name: %, a: %", entity._id, test2->name, test1->a);
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

	test_ecs();
}
