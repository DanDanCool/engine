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
	enum {
		PIPELINE_TYPE_GRAPHICS
	};

	enum {
		BUFFER_TYPE_VERTEX = 1 << 0,
		BUFFER_TYPE_INDEX = 1 << 1,
		BUFFER_TYPE_STAGING = 1 << 2,
		BUFFER_TYPE_HOST = 1 << 3,
	};

	enum {
		QUEUE_TYPE_GRAPHICS,
		QUEUE_TYPE_TRANSFER,
	};

	struct framebuffer {
		math::vec2i size();
		core::ptr<void> data;
	};

	// on platforms that allow it, these should be aliased
	struct buffer {
		template<typename T>
		void data(cref<core::vector<T>> in) {
			data((u8*)in.data, in.size * sizeof(T));
		}

		void data(u8* in, u32 bytes);

		core::ptr<void> data;
	};

	struct command_buffer {
		void begin_renderpass(ref<framebuffer> fb);
		void end_renderpass();

		void bind_pipeline(pipeline_type type, cref<core::string> name);

		void bind_vertexbuffers(cref<core::vector<buffer>> vbs);
		void bind_vertexbuffer(cref<buffer> vb);
		void bind_indexbuffer(<cref<buffer> ib);

		void draw_indexed();

		// might need vector forms for multi viewport applications (XR)
		void set_viewport(math::vec2f topleft, math::vec2f size, math::vec2f depth);
		void set_scissor(math::vec2i offset, math::vec2i extent);

		core::ptr<void> data;
	};

	// synchronization object between gpu and cpu
	struct fence {
		void wait();
		void reset();
		core::ptr<void> data;
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
		void add(cref<core::string> name, pfn_render_node renderfn, cref<core::vector<core::string>> dependencies) {
			core::lock lock(busy);
			nodes[name].renderfn = renderfn;

			auto& vec = graph[name];
			if (!vec.data) {
				vec = core::vector<core::string>(0);
			}

			for (auto& dependency : dependencies) {
				vec.add(dependency);
			}
		}

		// build the graph and allocate resources
		void build() {
			core::lock lock(busy);
			// use additional input/output information to order graphs
		}

		// render
		void execute() {

		}

		// rendering commands
		core::vector<render::command_buffer> command_buffers(u32 type, u32 count, render::fence* fence = nullptr);
		render::command_buffer command_buffer(render::queue_type type, render::fence* fence = nullptr);

		bool framebuffer(u32 i, ref<render::framebuffer> fb); // swapchain framebuffer, may fail
		render::framebuffer framebuffer(cref<core::string> name); // off screen framebuffer

		render::buffer buffer(cref<core::string> name, u32 size, u32 type);
		void _present(); // internal use only

		render::fence fence(cref<core::string> name);

		void submit(cref<render::command_buffer> cmd);
		void submit(cref<core::vector<render::command_buffer>> cmds);

		void submit(cref<render::command_buffer> cmd, cref<render::fence> fence);
		void submit(cref<core::vector<render::command_buffer>> cmds, cref<render::fence> fence);

		// additional node ordering information
		void input(cref<core::string> name);
		void input(cref<core::vector<core::string>> names);
		void output(cref<core::string> name);
		void output(cref<core::vector<core::string>> names);

		core::table<core::string, pfn_render_node> nodes;
		core::table<core::string, core::vector<core::string>> graph;
		core::mutex busy;
	};
}
