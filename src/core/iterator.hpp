module;

#include "core.h"

export module core.iterator;

export namespace core {
	template <typename T>
	struct adaptor {
		using container = T;
		adaptor(cref<container> in) : data(in) {}
		cref<container> data;
	};

	template <typename T>
	struct forward : public adaptor<T> {
		using parent_type = adaptor<T>;
		forward(cref<T> in) : parent_type(in) {}

		auto begin() const {
			return parent_type::data.begin();
		}

		auto end() const {
			return parent_type::data.end();
		}
	};

	template <typename T>
	struct reverse : public adaptor<T> {
		using parent_type = adaptor<T>;
		reverse(cref<T> in) : parent_type(in) {}

		auto begin() const {
			return parent_type::data.rbegin();
		}

		auto end() const {
			return parent_type::data.rend();
		}
	};

	struct range {
		range(u32 end) : _beg(0), _end(end), _inc(1) {}
		range(u32 beg, u32 end) : _beg(beg), _end(end), _inc(0) {
			_inc = beg <= end ? 1 : -1;
		}
		range(u32 beg, u32 end, u32 inc) : _beg(beg), _end(end), _inc(inc) {}

		struct iterator {
			iterator(u32 idx, u32 inc)
			: index(idx), increment(inc) {}

			iterator& operator++() {
				index += increment;
				return *this;
			}

			bool operator !=(cref<iterator> other) const {
				return index != other.index;
			}

			i32 operator*() const {
				return index;
			}

			i32 index, increment;
		};

		iterator begin() {
			return iterator(_beg, _inc);
		}

		iterator end() {
			return iterator(_end, 0);
		}

		i32 _beg, _end, _inc;
	};

	// python style enumerate
	struct enumerate {

	};
}
