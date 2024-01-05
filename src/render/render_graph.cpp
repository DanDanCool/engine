module;

#include <core/core.h>

export module jolly.render_graph;
import core.table;
import core.vector;
import core.vector;
import core.string;
import core.lock;
import core.memory;
import math.vec;

export namespace render {
	enum class pipeline_type {
		graphics
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
		math::vec2i size();
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
			return renderpass{};
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

export namespace jolly {
	struct render_graph;
	typedef void (*pfn_render_node)(ref<render_graph> graph);

	struct render_graph {
		render_graph()
		: nodes()
		, graph()
		, busy() {
			// present all swapchains
			auto root = [](ref<render_graph> graph) {
				graph.input("swapchain")
				graph._present();
			};

			add("root", root);
		}

		~render_graph() {

		}

		// graph commands
		void add(cref<core::string> name, pfn_render_node renderfn) {
			core::lock lock(busy);
			nodes[name].renderfn = renderfn;
		}

		// build the graph and allocate resources
		void build() {
			core::lock lock(busy);
			// use additional input/output information to order graphs
		}

		// render
		void execute() {
			core::lock lock(busy);
		}

		// rendering commands
		render::command_buffer command_buffer(render::queue_type type, u32 count) {
			return render::comand_buffer{};
		}

		// swapchain framebuffer, may fail
		bool swapchain(u32 i, ref<render::framebuffer> fb) {
			return true;
		}

		// off screen framebuffer
		render::framebuffer framebuffer(cref<core::string> name) {
			return render::framebuffer{};
		}

		template<typename T>
		render::buffer buffer(cref<core::vector<T>> data, u32 type) {
			return render::buffer{};
		}

		render::pipeline pipeline(cref<core::string> name) {
			return render::pipeline{};
		}

		render::descriptor_set descriptor_set(cref<render::pipeline> pipe, u32 set = 0) {
			return render::descriptor_set{};
		}

		void submit(cref<render::command_buffer> cmd) {

		}

		void submit(cref<core::vector<render::command_buffer>> cmds) {

		}

		core::handle input(cref<core::string> name) {
			return core::handle{};
		}

		void output(cref<core::string> name, core::handle out) {

		}

		// internal use only
		void _present() {

		}

		core::table<core::string, pfn_render_node> nodes;
		core::table<core::string, core::vector<core::string>> graph;
		core::mutex busy;
	};
}
