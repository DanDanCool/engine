export module core.iterator;
import core.types;

export namespace core {
	namespace iterator {
		template <typename T>
		rsequential_base {
			using type = T;
			using this_type = rsequential_base<T>;

			rsequential_base(cptr in, u32 i)
			: data(in), index(i) {}

			cref<type> operator*() const {
				return data[index];
			}

			bool operator!=(cref<this_type> other) const {
				return other.index != index || other.data != data;
			}

			cptr<type> data;
			i32 index;
		};

		template <typename T>
		wsequential_base {
			using type = T;

			rsequential_base(ptr in, u32 i)
			: data(in), index(i) {}

			ref<type> operator*() const {
				return data[index];
			}

			bool operator!=(cref<this_type> other) const {
				return other.index != index || other.data != data;
			}

			ptr<type> data;
			i32 index;
		};

		template <typename base>
		forward_base: public base {
			using this_type = forward_base<base>;
			using base::base;

			ref<this_type> operator++() {
				base::index++;
				return *this;
			}
		}

		template <typename base>
		reverse_base: public base {
			using this_type = reverse_base<base>;
			using base::base;

			ref<this_type> operator++() {
				base::index--;
				return *this;
			}
		}

		template <typename T>
		using rforward_seq = forward_base<rsequential_base<T>>;

		template <typename T>
		using wforward_seq = forward_base<wsequential_base<T>>;

		template <typename T>
		using rreverse_seq = reverse_base<rsequential_base<T>>;

		template <typename T>
		using wreverse_seq = reverse_base<wsequential_base<T>>;
	}

	// reference wrapper
	template <typename T>
	struct wref {
		using type = T;
		wref()
		: data(nullptr) {}

		wref(type& in)
		: data(&in) {}

		operator cref<type>() const {
			return data;
		}

		operator ref<type>() {
			return *data;
		}

		type* operator->() const {
			return data;
		}

		type* data;
	};

	template <typename T1, typename T2>
	struct pair {
		using this_type = pair<T1, T2>;
		pair() = default;

		pair(fwd<T1> a, fwd<T2> b)
			: x(forward_data(a)), y(forward_data(b)) {}

		pair(cref<T1> a, cref<T2> b)
			: x(a), y(b) {}

		pair(fwd<this_type> other) {
			*this = forward_data(other);
		}

		pair(cref<this_type> other) {
			*this = other;
		}

		ref<this_type> operator=(fwd<this_type> other) {
			x = forward_data(other.one);
			y = forward_data(other.two);
			return *this;
		}

		ref<this_type> operator=(cref<this_type> other) {
			x = other.one;
			y = other.two;
			return *this;
		}

		~pair() = default;

		T1 x;
		T2 y;
	};

	template <typename T>
	struct adaptor {
		using container = T;
		adaptor(cref<container> in) : data(in) {}
		cref<container> data;
	};

	template <typename T>
	struct forward: public adaptor<T> {
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
	struct reverse: public adaptor<T> {
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
		range(i32 end) : _beg(0), _end(end), _inc(1) {}
		range(i32 beg, i32 end) : _beg(beg), _end(end), _inc(0) {
			_inc = beg <= end ? 1 : -1;
		}
		range(i32 beg, i32 end, i32 inc) : _beg(beg), _end(end), _inc(inc) {}

		struct iterator {
			iterator(u32 idx, u32 inc)
			: index(idx), increment(inc) {}

			iterator& operator++() {
				index += increment;
				return *this;
			}

			bool operator!=(cref<iterator> other) const {
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

	template <typename T>
	struct enumerate: public adaptor<T> {
		using parent_type = adaptor<T>;

		enumerate(cref<T> in) : parent_type(in) {}

		template <typename iterator>
		struct wrapper {
			using this_type = wrapper<iterator>;

			wrapper(i32 i, fwd<iterator> in)
			: index(i), it(forward_data(in)) {}

			ref<this_type> operator++() {
				++index;
				++it;
				return *this;
			}

			auto operator*() const {
				using pair_type = pair<i32, wref<decltype(*it)>>;
				return pair_type(index, *it);
			}

			bool operator!=(cref<this_type> other) const {
				return other.it != it;
			}

			i32 index;
			iterator it;
		};

		auto begin() {
			return wrapper(0, parent_type::data.begin());
		}

		auto end() {
			return wrapper(0, parent_type::data.end());
		}
	};
}
