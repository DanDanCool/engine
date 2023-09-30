#include "core/vector.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/table.h"
#include <iostream>

using namespace core;

struct foostruct {
	foostruct(cref<int> in) : data(in) {}
	~foostruct() {
		std::cout << "foostruct destructor " << data << std::endl;
	}
	int data;
};

int main() {
	{
		vector<int> v(0);

		int a = {};

		for (u32 i = 0; i < 32; i++) {
			v.add(i);
		}
	}

	{
		vector<foostruct> scopedv(0);
		for (int i = 0; i < 10; i++) {
			scopedv.add(foostruct(i));
		}

		for (auto x : reverse(scopedv)) {
			std::cout << x.data << std::endl;
		}
	}

	{
		string s("hello world!");
		for (auto x : forward(s)) {
			std::cout << x << std::endl;
		}
	}

	{
		table<i32, i32> mytable;
		for (i32 x : range(9999)) {
			mytable[x] = x * 2;
		}

		i64 sum = 0;
		for (i32 x : range(9999)) {
			sum += mytable[x];
		}
		std::cout << sum << std::endl;
	}

	{
		ptr<string> scope("hello world");
		ptr<int> a(4);
		std::cout << scope->data << std::endl;
		std::cout << a.ref() << std::endl;

		vector<ptr<int>> indirect(0);

		for (int i : range(20)) {
			indirect.add(ptr<int>(i));
		}

		for (cref<ptr<int>> p : reverse(indirect)) {
			std::cout << p.ref() << std::endl;
		}
	}
}
