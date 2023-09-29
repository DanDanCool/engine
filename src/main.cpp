#include "core/vector.h"
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
	vector<int> v;

	int a = {};

	for (u32 i = 0; i < 32; i++) {
		v.add(i);
	}

	{
		vector<foostruct> scopedv;
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

	table<string, int> mytable;
	mytable["hello"] = 69;
	mytable["world"] = 4;

	std::cout << mytable["hello"] << std::endl;

	mytable.del("world");

	std::cout << mytable["world"] << std::endl;
}
