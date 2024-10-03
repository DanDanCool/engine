module;

#include <core/core.h>
#include <initializer_list>

export module jolly.jml;
import core.iterator;
import core.types;
import core.memory;
import core.operations;
import core.table;
import core.vector;
import core.string;
import core.file;

export namespace jolly {
	enum class jml_type {
		unk = 0,
		num = 1 << 0,
		str = 1 << 1,
		boolean = 1 << 2,
		arr = 1 << 3,
		tbl = 1 << 4
	};

	ENUM_CLASS_OPERATORS(jml_type);
	struct jml_val;
	struct jml_doc;

	struct jml_tbl {
		bool operator==(cref<jml_tbl> other) const {
			if (parent && other.parent) {
				if (!(*parent == *other.parent)) return false;
			} else if (parent || other.parent) {
				return false;
			}

			return name == other.name;
		}

		core::strv name;
		cptr<jml_tbl> parent;
		ptr<jml_doc> data;
	};

	template <typename T>
	jml_type get_jml_type() {
		return jml_type::unk;
	}

	template<>
	jml_type get_jml_type<f64>() {
		return jml_type::num;
	}

	template<>
	jml_type get_jml_type<core::string>() {
		return jml_type::str;
	}

	struct jml_val;

	template<>
	jml_type get_jml_type<core::vector<jml_val>>() {
		return jml_type::arr;
	}

	template<>
	jml_type get_jml_type<jml_tbl>() {
		return jml_type::tbl;
	}

	template<>
	jml_type get_jml_type<bool>() {
		return jml_type::boolean;
	}

	struct jml_doc {
		jml_doc() = default;

		cref<jml_val> get(core::strv key) const {
			jml_tbl tmp{ key, nullptr, nullptr };
			cref<jml_val> res = data.get(tmp);
			return res;
		}

		cref<jml_val> get(cref<jml_tbl> key) const {
			return data.get(key);
		}

		ref<jml_val> operator[](cref<jml_tbl> key) {
			return data[key];
		}

		ref<jml_val> operator[](core::strv key);

		cref<jml_val> operator[](cref<jml_tbl> key) const {
			return get(key);
		}

		cref<jml_val> operator[](core::strv key) const {
			return get(key);
		}

		core::table<jml_tbl, jml_val> data;
	};

	struct jml_val {
		jml_val() = default;

		template<typename T>
		jml_val(fwd<T> in)
		: data(), type() {
			*this = forward_data(in);
		}

		jml_val(fwd<jml_val> in)
		: data(), type() {
			*this = forward_data(in);
		}

		template<typename T>
		ref<jml_val> operator=(fwd<T> in) {
			if (data.data) {
				data.core::any::~any();
			}

			data = core::mem_create<T>(forward_data(in));
			type = get_jml_type<T>();

			return *this;
		}

		ref<jml_val> operator=(fwd<jml_val> in) {
			data = forward_data(in.data);
			type = in.type;
			return *this;
		}

		template<typename T>
		cref<T> raw() const {
			JOLLY_ASSERT(get_jml_type<T>() == type);
			return data.get<T>();
		}

		template<typename T>
		T get() const {
			return core::copy(raw<T>());
		}

		template<typename T>
		T get(u32 idx) const {
			JOLLY_ASSERT((get_jml_type<T>() & type) == type);
			JOLLY_ASSERT((type & jml_type::arr) == jml_type::arr);
			auto& vec = data.get<core::vector<jml_val>>();
			return vec[idx].get<T>();
		}

		u32 size() const {
			JOLLY_ASSERT((type & jml_type::arr) == jml_type::arr);
			auto& vec = data.get<core::vector<jml_val>>();
			return vec.size;
		}

		auto begin() const {
			JOLLY_ASSERT((type & jml_type::arr) == jml_type::arr);
			auto& vec = data.get<core::vector<jml_val>>();
			return vec.begin();
		}

		auto end() const {
			JOLLY_ASSERT((type & jml_type::arr) == jml_type::arr);
			auto& vec = data.get<core::vector<jml_val>>();
			return vec.end();
		}

		ref<jml_val> operator[](core::strv key) {
			JOLLY_ASSERT(type == jml_type::tbl);
			auto& tbl = data.get<jml_tbl>();

			jml_tbl tmp{ key, &tbl, nullptr };
			ref<jml_doc> doc = *tbl.data;
			ref<jml_val> res = doc[tmp];

			if (res.type == jml_type::unk) {
				res = jml_tbl{ key, &tbl, &doc };
			}

			return res;
		}

		core::any data;
		jml_type type;
	};

	ref<jml_val> jml_doc::operator[](core::strv key) {
		jml_tbl tmp{ key, nullptr, nullptr };
		ref<jml_val> res = data[tmp];

		if (res.type == jml_type::unk) {
			res = jml_tbl{ key, nullptr, this };
		}

		return res;
	}

	template<typename T>
	core::vector<jml_val> jml_vector_impl(std::initializer_list<T> args) {
		core::vector<jml_val> res(0);
		for (auto& item : args) {
			res.add(jml_val(core::copy(item)));
		}

		return res;
	}

	core::vector<jml_val> jml_vector(std::initializer_list<core::string> args) {
		return jml_vector_impl<core::string>(args);
	}

	core::vector<jml_val> jml_vector(std::initializer_list<f64> args) {
		return jml_vector_impl<f64>(args);
	}

	void jml_dump(cref<jml_doc> data, ref<core::file> f);
}

export namespace core {
	template<>
	struct op_hash<jolly::jml_tbl> {
		using type = jolly::jml_tbl;

		// based on fnv1a
		static u32 hash(cref<type> key) {
			u32 h = 0x811c9dc5;
			if (key.parent) {
				h = hash(*key.parent);
			}

			for (i32 i : range(key.name.size))
				h = (h ^ key.name[i]) * 0x01000193;

			return h;
		}
	};
}
