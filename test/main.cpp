#include <core/core.h>
#include <iostream>
#include <string>

import core.types;
import core.vector;
import core.memory;
import core.string;
import core.table;
import core.thread;
import core.lock;
import core.log;
import core.set;
import core.atom;
import core.format;
import core.iterator;
import core.file;

import jolly.jml;
import jolly.ecs;
import jolly.spirv.parser;

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
	mem<basicstruct> myptr = mem_create<basicstruct>(1, 2, 3, 4);

	auto coroutine = [](ref<thread>, mem<void>&& in) -> int {
		mem<int> args = in.cast<int>();

		for (int x : range(args.get())) {
			LOG_INFO("%", x);
		}

		return 0;
	};

	auto args = mem_create<int>(5);
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

	auto coroutine = [](ref<thread>, mem<void>&& in) {
		mem<my_data> args = in.cast<my_data>();

		for (int x : range(10000)) {
			u32 val = args->counter.get(memory_order_relaxed);
			while (!args->counter.cmpxchg(val, val + 1, memory_order_release, memory_order_relaxed));
		}

		return 0;
	};

	vector<thread> threads(10);
	for (int i : range(10)) {
		threads[i] = thread(coroutine, mem_create<my_data>(counter).cast<void>());
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
	mem<string> scope = mem_create<string>("hello world");
	mem<int> a = mem_create<int>(4);
	LOG_INFO("%, %", scope->data, a.get());

	auto mylambda = [](cref<mem<int>> in) {
		LOG_INFO("%", in.get());
	};

	mem<int> foo = mem_create<int>(69);
	mylambda(foo);

	vector<mem<int>> indirect(0);

	for (int i : range(10)) {
		indirect.add(mem_create<int>(i));
	}

	for (cref<mem<int>> p : reverse(indirect)) {
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

void test_convert() {
	LOG_INFO("% string conversion", DIVIDE);
	i64 i = stoi("543");
	i64 i_neg = stoi("-543");
	i64 i_score = stoi("876_999");

	LOG_INFO("test int % test int", i);
	LOG_INFO("test int % test int", i_neg);
	LOG_INFO("test int % test int", i_score);

	f64 f = stod("5.0");
	f64 f_dot = stod("5.");
	f64 f_neg = stod("-8432.234");
	f64 f_exp = stod("334_4.3e10");
	f64 f_nexp = stod("-3.45_321e-66");
	f64 f_nexp2 = -3.45321e-66;
	f64 f_nexp3 = std::stod("-3.45321e-66");
	f64 f_npow10 = stod("21e-5");
	f64 f_underscore_monster = stod("123_456_789.420_69_69e43");
	f64 f_underscore_monster2 = 123456789.4206969e43;

	LOG_INFO("test double % test double", f);
	LOG_INFO("test double % test double", f_dot);
	LOG_INFO("test double % test double", f_neg);
	LOG_INFO("test double % test double", f_exp);
	LOG_INFO("test double % test double", f_nexp);
	LOG_INFO("test double % test double", f_nexp2);
	LOG_INFO("test double % test double", f_npow10);
	LOG_INFO("test double % test double", f_underscore_monster);
	std::cout << f_nexp << std::endl;

	f32 f32t = stof("5.0");
	f32 f32t_dot = stof("5.");
	f32 f32t_neg = stof("-8432.234");
	f32 stdtest = std::stof("-8432.234");
	f32 f32t_exp = stof("3344.3e10");
	f32 f32t_nexp = stof("-3.45321e-66");
	f32 f32t_nexp2 = -3.45321e-66;
	f32 f32t_npow10 = stof("21e-5");

	LOG_INFO("test float % test float", f32t);
	LOG_INFO("test float % test float", f32t_dot);
	LOG_INFO("test float % test float", f32t_neg);
	LOG_INFO("test float % test float", f32t_exp);
	LOG_INFO("test float % test float", f32t_nexp);
	LOG_INFO("test float % test float", f32t_nexp2);
	LOG_INFO("test float % test float", f32t_npow10);

	LOG_INFO("f32 %", 420.69f);
}

void test_jml() {
	LOG_INFO("% test jml", DIVIDE);

	core::string s1("test");
	core::string s2("test");

	LOG_INFO("% equal", s1 == s2);

	jolly::jml_tbl k1{ "test", nullptr, nullptr };
	jolly::jml_tbl k2{ "test", nullptr, nullptr };

	jolly::jml_tbl k3{ "test", &k1, nullptr };
	jolly::jml_tbl k4{ "test", &k2, nullptr };

	LOG_INFO("% equal", k1 == k2);
	LOG_INFO("% equal", k4 == k3);

	jolly::jml_doc doc;
	doc["test"]["test"]["test"]["test"]["test"]["test"] = 3.0;

	doc["test"]["mystring"] = core::string("hello");

	f64 very_nested = doc["test"]["test"]["test"]["test"]["test"]["test"].get<f64>();
	LOG_INFO("very_nested is %", very_nested);

	core::string mystring = doc["test"]["mystring"].get<core::string>();
	LOG_INFO("mystring is %", mystring);

	doc["vector_f64"] = jolly::jml_vector({ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 });
	for (auto& val : doc["vector_f64"]) {
		LOG_INFO("val is %", val.get<f64>());
	}

	doc["mybool"] = true;
	LOG_INFO("bool is %", doc["mybool"].get<bool>());

	doc["vector_str"] = jolly::jml_vector({ "hello", "world", "test123" });
	for (auto& val : doc["vector_str"]) {
		LOG_INFO("val is %", val.get<core::string>());
	}

	auto f = fopen("dump.jml", core::access::wo);
	jml_dump(doc, f);
}

void test_spirv() {
	jolly::spirv_parse("../assets/shaders/texture");
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
	test_convert();
	test_jml();
	test_spirv();
}
