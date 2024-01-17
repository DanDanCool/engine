module;

#include <core/core.h>

export module jolly.render_graph;
import core.types;
import render.primitives;
import core.table;
import core.vector;
import core.vector;
import core.string;
import core.lock;
import core.memory;
import core.tuple;
import math.vec;

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
				graph.input("present");
				graph._present();
			};

			add("root", root);
		}

		~render_graph() {

		}

		// graph commands
		void add(cref<core::string> name, pfn_render_node renderfn) {
			core::lock lock(busy);
			nodes.set(name, forward_data(renderfn));
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
		core::vector<render::command_buffer> command_buffer(render::queue_type type, u32 count) {
			return core::vector<render::command_buffer>(0);
		}

		// grab window 'i' swapchain framebuffer, may fail
		bool swapchain(u32 i, ref<render::framebuffer> fb) {
			return true;
		}

		u32 windows() const {
			return 0;
		}

		// off screen framebuffer
		render::framebuffer framebuffer(cref<core::string> name) {
			return render::framebuffer{};
		}

		template<typename T>
		render::buffer buffer(cref<core::vector<T>> data, render::buffer_type type) {
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
