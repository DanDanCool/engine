module;

#include <core/core.h>

module jolly.jml;
import core.format;

namespace jolly {
	using doc_hierarchy = core::table<jml_tbl, core::vector<cptr<jml_tbl>>>;
	template<typename T>
	u64 min_space_required() = delete;

	template<>
	u64 min_space_required<bool>() {
		return sizeof("false") - 1;
	}

	template<>
	u64 min_space_required<f64>() {
		constexpr u64 sign = 1;
		constexpr u64 mantissa = 20;
		constexpr u64 dot = 1;
		constexpr u64 exp = 3 + 1; // 'e' + 3 digits
		return sign + mantissa + dot + exp;
	}

	template<typename T>
	void format_safe(cref<T> arg, ref<core::file> f) {
		if (f.data.rem() < min_space_required<T>()) {
			f.write();
		}

		core::format(arg, f.data);
	}

	void dump_item(cref<jml_val> val, cref<jml_doc> data, cref<doc_hierarchy> hierarchy, ref<core::file> f) {
		switch(val.type) {
			case jml_type::boolean: {
				format_safe(val.raw<bool>(), f);
				break;
			}
			case jml_type::num: {
				format_safe(val.raw<f64>(), f);
				break;
			}
			case jml_type::str: {
				f.write('"');
				f.write(val.raw<core::string>());
				f.write('"');
				break;
			}
			case jml_type::arr: {
				f.write('[');
				for (auto& child : val) {
					dump_item(child, data, hierarchy, f);
					f.write(',');
				}
				f.write(']');
				break;
			}
			case jml_type::tbl: {
				f.write('{');
				for (auto child : hierarchy[val.raw<jml_tbl>()]) {
					f.write(child->name);
					f.write('=');
					dump_item(data.get(*child), data, hierarchy, f);
					f.write(',');
				}
				f.write('}');
				break;
			}
		}
	}

	void jml_dump(cref<jml_doc> data, ref<core::file> f) {
		doc_hierarchy hierarchy;
		core::vector<cptr<jml_tbl>> root(0);
		for (auto& key : data.data.keys()) {
			if (!key.parent) {
				root.add(&key);
				continue;
			}

			auto& v = hierarchy[*key.parent];
			if (!v.reserve) {
				v = core::vector<cptr<jml_tbl>>(0);
			}

			v.add(&key);
		}

		for (auto key : root) {
			f.write(key->name);
			f.write('=');
			dump_item(data.get(*key), data, hierarchy, f);
			f.write('\n');
		}
	}
}
