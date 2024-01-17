module;

#include <core/core.h>

export module jolly.jml;
import core.iterator;
import core.types;
import core.memory;
import core.operations;
import core.table;
import core.vector;
import core.string;

export namespace jolly {
	enum class jml_type {
		num = 1 << 0,
		str = 1 << 1,
		arr = 1 << 2,
		tbl = 1 << 3
	};

	template <typename T>
	jml_type get_jml_type() = delete;

	template<>
	jml_type get_jml_type<f64>() {
		return jml_type::num;
	}

	template<>
	jml_type get_jml_type<core::string>() {
		return jml_type::str;
	}

	ENUM_CLASS_OPERATORS(jml_type);

	struct jml_val;
	struct jml_doc;

	struct jml_tbl {
		bool operator==(cref<jml_tbl> other) const {
			if (parent && other.parent) {
				if (!(*parent == *other.parent)) return false;
			}

			if (parent || other.parent) return false;
			return name == other.name;
		}

		core::string name;
		cptr<jml_tbl> parent;
		ptr<jml_doc> data;
	};

	struct jml_doc {
		cref<jml_val> get(cref<core::string> key) const {
			jml_tbl tmp{ core::string(key.data, key.size), nullptr, nullptr };
			cref<jml_val> res = data.get(tmp);
			tmp.name.data = nullptr;
			return res;
		}

		cref<jml_val> get(cref<jml_tbl> key) const {
			return data.get(key);
		}

		core::table<jml_tbl, jml_val> data;
	};

	struct jml_val {
		template<typename T>
		T get() const {
			JOLLY_ASSERT(get_jml_type<T>() == type);
			return copy(data.get<T>());
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

		cref<jml_val> operator[](cref<core::string> key) const {
			JOLLY_ASSERT(type == jml_type::tbl);
			auto& tbl = data.get<jml_tbl>();

			jml_tbl tmp{ core::string(key.data, key.size), &tbl, nullptr };
			cref<jml_val> res = tbl.data->get(tmp);
			tmp.name.data = nullptr;
			return res;
		}

		core::any data;
		jml_type type;
	};
}

export namespace core {
	template<>
	struct operations<jolly::jml_tbl>: public operations_base<jolly::jml_tbl> {
		using type = jolly::jml_tbl;

		// based on fnv1a
		static u32 hash(cref<type> key) {
			u32 h = 0x811c9dc5;
			if (key.parent) {
				h = hash(*key.parent);
			}

			for (i32 i : range((u32)key.name.size))
				h = (h ^ key.name[i]) * 0x01000193;

			return h;
		}
	};
}
