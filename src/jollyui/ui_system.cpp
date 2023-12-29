module;

#include <core/core.h>

export module jolly.ui_system;
import core.log;
import jolly.system;
import jolly.ui;
import jolly.engine;
import jolly.components;
import jolly.render_graph;
import jolly.render_thread;

namespace jolly {
	struct ui_system : public system {
		ui_system() {
			LOG_INFO("UI system");
		}

		~ui_system() {
			LOG_INFO("~UI system");
		}

		virtual void init() {
			auto ui_render = [](ref<ui_context> ui, f32 ms) {
				if (ui.button("press me!")) {
					LOG_INFO("button pressed");
				}
			};

			auto& state = engine::instance().get_ecs();
			auto e = state.create();
			state.add<ui_component>(e, ui_component{
					math::vec2f{ 0, 0 },
					math::vec2f{ 1, 1 },
					math::vec3f{ 0.2f, 0.8f, 0.2f },
					ui_render});
		}

		virtual void term() {
			// maybe delete the entities we created
		}


		virtual void term();

		virtual void step(f32 ms);

		virtual void step(f32 ms) {
			auto& state = engine::instance().get_ecs();

			for (auto& [entity, component] : state.view<quad_component>()) {
				//LOG_INFO("quad_component: e", entity.id());
			}

			auto renderui = [](ref<render_graph> graph) {
				// other_node -> graph.output("main")
				// input/output specifiers allow for more dynamic dependencies, for more dynamic graphs
				graph.input("main"); // specify dependency on 'main' resource

				auto fence = graph.fence("render_finished"); // limit amount of frames we work on
				fence.wait();

				core::vector<render::command_buffer> cmds = graph.command_buffer(render::queue_type::graphics, windows.size);
				for (i32 i : core::range(windows)) {
					// graph.framebuffer("main"); // creates an offscreen framebuffer
					render::framebuffer fb;
					 // gets swapchain framebuffer
					if (!graph.framebuffer(i, fb)) {
						continue; // likely that swapchain is being resized
					}

					auto& cmd = cmds[i];

					// resources often need several instances to deal with multiple frames
					// probably do one allocation per frame
					// auto& vb = graph.vertexbuffer("obj_model", HOST_MEMORY);
					render::buffer_type flags = render::buffer_type::vertex | render::buffer_type::staging;
					u32 vb_size = 0; // put a limit here
					auto vb = graph.buffer(core::format_string("swapvb%", i).data, size, flags);
					vb.data(vertices);

					flags = render::buffer_type::index | render::buffer_type::staging;
					u32 ib_size = 0;
					auto ib = graph.buffer(core::format_string("swapib%", i).data, size, flags);
					ib.data(indices);

					// automatically depend on semaphores
					// resource should contain semaphore objects
					cmd.begin_renderpass(fb);
					cmd.bind_pipeline(render::pipeline_type::graphics, "quad");
					// cmd.bind_vertexbuffers({ a, b, c });
					cmd.bind_vertexbuffer(vb);
					cmd.bind_indexbuffer(ib);

					auto [x, y] = fb.size();
					cmd.set_viewport(math::vec2f{ 0, 0 }, math::vec2f{ x, y }, math::vec2f{ 0.0f, 1.0f });
					cmd.set_scissor(math::vec2i{ 0, 0 }, math::vec2i{ x, y });

					cmd.draw_indexed();
					cmd.end_renderpass();
				}

				fence.reset();
				graph.submit(cmds, fence);

				graph.output("swapchain");
			};

			auto& render = (ref<render_thread>)engine::instance().get("render");
			auto& graph = render.graph();
			graph.add("ui", renderui);
		}

		ref<ui_defaults> defaults();
		void update_defaults();

		ui_defaults _defaults;
	};
}
