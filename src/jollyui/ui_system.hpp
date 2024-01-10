module;

#include <core/core.h>

export module jolly.ui_system;
import core.log;
import core.lock;
import core.iterator;
import jolly.system;
import jolly.ui;
import jolly.engine;
import jolly.components;
import jolly.render_graph;
import jolly.render_thread;
import jolly.ecs;

export namespace jolly {
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

			auto state = core::wview_create(engine::instance().get_ecs());
			auto e = state->create();

			ui_component component{
				math::vec2i{ 0, 0 },
				math::vec2i{ 1, 1 },
				math::vec3f{ 0.2f, 0.8f, 0.2f },
				ui_render
			};

			state->add<ui_component>(e, component);
		}

		virtual void term() {
			// maybe delete the entities we created
		}

		virtual void step(f32 ms) {
			auto state = core::rview_create(engine::instance().get_ecs());

			for (auto [entity, component] : core::rview_create(state->view<quad_component>())) {
				LOG_INFO("quad_component: e", entity.id());
			}

			auto renderui = [](ref<render_graph> graph) {
				// other_node -> graph.output("main")
				auto resource = graph.input("main"); // specify dependency on 'main' resource

				// graph creates a default renderpass, pipeline, and framebuffers for presentation purposes
				// it can be accessed by calling respective functions without arguments

				core::vector<render::command_buffer> cmds = graph.command_buffer(render::queue_type::graphics, graph.windows());
				for (i32 i : core::range(graph.windows())) {
					// graph.framebuffer("main"); // creates an offscreen framebuffer
					render::framebuffer fb;
					 // gets swapchain framebuffer
					if (!graph.swapchain(i, fb)) {
						continue; // likely that swapchain is being resized
					}

					auto& cmd = cmds[i];

					auto vb_data = core::vector<f32>();
					auto ib_data = core::vector<i32>();

					// staging buffers, resource transitions should be automatic
					// resources often need several instances to deal with multiple frames
					// probably do one allocation per frame
					render::buffer_type flags = render::buffer_type::vertex;
					auto vb = graph.buffer(vb_data, flags);

					flags = render::buffer_type::index;
					auto ib = graph.buffer(ib_data, flags);

					// pipelines loaded by name
					auto pipeline = graph.pipeline("name");

					// automatically depend on semaphores
					// resource should contain semaphore objects
					// graph.renderpass() gives default renderpass
					// each command buffer is associated with one renderpass/subpass
					// recording multiple render passes in a buffer is not possible
					cmd.begin(pipeline.renderpass(), fb);

					// always bind pipeline first, pipeline data will be used to verify correctness of later commands
					cmd.pipeline(render::pipeline_type::graphics, pipeline);

					auto [x, y] = fb.size();
					cmd.viewport(math::vec2f{ 0, 0 }, math::vec2f{ (f32)x, (f32)y }, math::vec2f{ 0.0f, 1.0f });
					cmd.scissor(math::vec2i{ 0, 0 }, math::vec2i{ x, y });

					// get set based off of index
					auto set = graph.descriptor_set(pipeline);

					auto lighting_buffer = graph.buffer(vb_data, flags);
					auto transform_buffer = graph.buffer(vb_data, flags);
					// update set using names instead of binding number
					set.update({ {"lighting", lighting_buffer.data}, {"transforms", transform_buffer.data} });

					cmd.descriptor_set({ set });

					// cmd.compute(compute);
					// cmd.draw({color, uv, specular}, ib);
					cmd.draw(vb, ib);

					// renderpass automatically ended when going out of scope
				}

				graph.submit(cmds);
				graph.output("present", resource);
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
