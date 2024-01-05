module;

#include <core/core.h>

export module jolly.ui;
import core.string;
import math.vec;

export namespace jolly {
	struct ui_context;

	typedef void (*pfn_ui_render)(ref<ui_context> ui, f32 ms);
	struct ui_component {
		math::vec2i pos;
		math::vec2i size;
		math::vec3f color;

		pfn_ui_render fn;
	};

	// ui.grid(4, 2);
	// if (ui.button("hello")) do something
	struct ui_context {
		void grid();
		void justify();
		bool button(cref<core::string> name);
	};

	struct ui_defaults {
		math::vec4f background; // window backgrounds

		// ui elements like buttons, sliders
		math::vec4f element_foreground;
		math::vec4f element_background;
		math::vec4f element_hover;
		math::vec4f element_select;

		math::vec4f text_color;
		math::vec4f text_select;
		core::string text_font;
	};
};
