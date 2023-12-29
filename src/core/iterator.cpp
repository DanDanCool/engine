module;

#include "core.h"

export module core.iterator;

export namespace core {
	template <typename T>
	struct view_base {
		using container = T;
		view_base(cref<container> in) : data(in) {}
		cref<container> data;
	};

	template <typename T>
	struct forward : public view_base<T> {
		forward(cref<T> in) : view_base(in) {}

		auto begin() const {
			return data.begin();
		}

		auto end() const {
			return data.end();
		}
	};

	template <typename T>
	struct reverse : public view_base<T> {
		reverse(cref<T> in) : view_base(in) {}

		auto begin() const {
			return data.rbegin();
		}

		auto end() const {
			return data.rend();
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
			: index(_idx), increment(_inc) {}

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
