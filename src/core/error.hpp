module;

#include "core.h"

export module core.error;
import core.types;
import core.string;

export namespace core {
	struct error {
		virtual ~error();
		virtual string message();
	};

	template <typename T, typename E>
	struct result : public monad<result<T, E>> {
		using type = T;

		result(T _value)
		: value(_value), err(none_option) {}

		result(E _err)
		: value(), err(_err) {}

		type get() const {
			JOLLY_CORE_ASSERT(!err);
			return value;
		}

		type get_or(type other) const {
			if (!other) {
				return value;
			}
			return other;
		}

		operator bool() {
			return !err;
		}

		type value;
		option<E> err;
	};
}
