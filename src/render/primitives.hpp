module;

#include <core/core.h>

export module render.primitives;
import math.vec;
import core.memory;
import core.tuple;
import core.string;
import core.vector;

export namespace render {
	enum class pipeline_type {
		graphics,
		compute
	};

	enum class buffer_type {
		vertex = 1 << 0,
		index = 1 << 1,
		staging = 1 << 2,
		host = 1 << 3,
	};

	enum class queue_type {
		graphics,
		transfer,
	};

	ENUM_CLASS_OPERATORS(pipeline_type);
	ENUM_CLASS_OPERATORS(buffer_type);
	ENUM_CLASS_OPERATORS(queue_type);

	struct framebuffer {
		math::vec2i size() {
			return math::vec2i{0, 0};
		}
		core::handle data;
	};

	// on platforms that allow it, these should be aliased
	struct buffer {
		core::handle data;
	};

	struct renderpass {
		core::handle data;
	};

	struct pipeline {
		render::renderpass renderpass() {
			return render::renderpass{};
		}

		core::handle data;
	};

	struct descriptor_set {
		using pair_type = core::pair<core::string, core::handle>;
		void update(cref<core::vector<pair_type>> writes) {

		}

		core::handle data;
	};

	struct command_buffer {
		void begin(cref<renderpass> pass, cref<framebuffer> fb) {

		}

		void end() {

		}

		void descriptor_set(cref<core::vector<descriptor_set>> sets) {

		}

		void pipeline(pipeline_type type, cref<pipeline> pipe) {

		}

		// might need vector forms for multi viewport (XR)
		void viewport(math::vec2f topleft, math::vec2f size, math::vec2f depth) {

		}

		void scissor(math::vec2i offset, math::vec2i extent) {

		}

		void draw(cref<core::vector<buffer>> vb, cref<buffer> ib) {

		}

		void draw(cref<buffer> vb, cref<buffer> ib) {

		}

		core::handle data;
	};

	// synchronization object between gpu and cpu
	struct fence {
		void wait() {

		}

		void reset() {

		}
		core::handle data;
	};
}
