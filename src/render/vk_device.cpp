module;

#include <core/core.h>
#include <vulkan/vulkan.h>

export module vulkan.device;
import core.string;
import core.memory;
import core.tuple;
import core.vector;
import core.log;
import core.set;
import core.file;
import math.vec;
import vulkan.surface;
import win32.window;

export namespace jolly {
	static VKAPI_ATTR VkBool32 VKAPI_CALL vkdebugcb(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* cbdata,
		void* userdata) {
		LOG_INFO("vulkan: %", cbdata->pMessage);
		return VK_FALSE;
	}

	constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

	enum class vertex_data {
		vec2,
		vec3,
	};

	enum class vertex_input {
		vertex,
		instance
	};

	typedef core::pair<vertex_data, u32> vertex_attribute;
	typedef core::pair<core::vector<vertex_attribute>, vertex_input> binding_description;

	struct window;

	struct vk_queue {
		VkQueue queue;
		u32 family;
	};

	struct vk_gpu {
		VkPhysicalDevice physical;
		VkDevice device;
		vk_queue graphics;
		vk_queue compute;
		vk_queue transfer;
		vk_queue present;
	};

	struct vk_pipeline {
		VkRenderPass renderpass;
		VkPipelineLayout layout;
		VkPipeline pipeline;

		VkBuffer buffer;
		VkDeviceMemory memory;

		VkBuffer index;
		VkDeviceMemory index_memory;

		u32 bindings;
	};

	struct vk_cmdpool {
		vk_cmdpool() = default;
		vk_cmdpool(cref<vk_gpu> gpu, u32 family)
		: pool(VK_NULL_HANDLE)
		, free(0)
		, busy(0) {
			VkCommandPoolCreateInfo pool_info{};
			pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			pool_info.queueFamilyIndex = family;
			vkCreateCommandPool(gpu.device, &pool_info, nullptr, &pool);
		}

		// needs explicit destruction
		void vk_cmdpool::destroy(cref<vk_gpu> gpu) {
			vkDestroyCommandPool(gpu.device, pool, nullptr);
		}

		VkCommandBuffer create(cref<vk_gpu> gpu) {
			if (free.size == 0) {
				VkCommandBufferAllocateInfo alloc_info{};
				alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				alloc_info.commandPool = pool;
				alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				alloc_info.commandBufferCount = 1;

				VkCommandBuffer buffer = VK_NULL_HANDLE;
				vkAllocateCommandBuffers(gpu.device, &alloc_info, &buffer);
				return buffer;
			}

			return free[--free.size];
		}

		void destroy(VkCommandBuffer buffer, VkFence fence) {
			busy.add({buffer, fence});
		}


		void gc(cref<vk_gpu> gpu) {
			for (i32 i : core::range(busy.size, 0, -1)) {
				i = i - 1;
				auto [buffer, fence] = busy[i];
				if (vkGetFenceStatus(gpu.device, fence) == VK_SUCCESS) {
					free.add(buffer);
					busy.del(i);
				}
			}
		}

		VkCommandPool pool;
		core::vector<VkCommandBuffer> free;
		core::vector<core::pair<VkCommandBuffer, VkFence>> busy;
	};

	struct vk_device {
		vk_device() = default;
		vk_device(cref<core::string> name, math::vec2i sz = { 1280, 720 })
		: _window()
		, _instance(VK_NULL_HANDLE)
		, _debugmsg(VK_NULL_HANDLE)
		, _pipeline{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE}
		, _cmdpool()
		, _gpus(0)
		, _gpu(U32_MAX)
		, _frame(0) {
			_window = core::ptr_create<window>(name, sz);
			_window->create("test", { 480, 480 });

			// enable validation layers
			core::vector<cstr> enabled_layers = {
				"VK_LAYER_KHRONOS_validation"
			};

			u32 count = 0;
			vkEnumerateInstanceLayerProperties(&count, nullptr);
			core::vector<VkLayerProperties> layers(count);
			vkEnumerateInstanceLayerProperties(&count, layers.data);
			layers.size = count;

			for (auto name : enabled_layers) {
				bool found = false;
				for (auto& layer : layers) {
					if (core::cmpeq(name, layer.layerName)) {
						found = true;
						break;
					}
				}

				JOLLY_ASSERT(found, "validation layer not found");
			}

			VkApplicationInfo app_info{};
			app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pApplicationName = "jolly";
			app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			app_info.pEngineName = "jolly_engine";
			app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			app_info.apiVersion = VK_API_VERSION_1_0;

			// enable extensions
			count = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
			core::vector<VkExtensionProperties> extensions(count);
			vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data);
			extensions.size = count;

			LOG_INFO("vkextensions");
			for (auto& extension : extensions) {
				LOG_INFO("extension: %", extension.extensionName);
			}

			core::vector<cstr> enabled_extensions = _window->vk_extensions(extensions);
			enabled_extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			VkDebugUtilsMessengerCreateInfoEXT msg_info{};
			msg_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			msg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			msg_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			msg_info.pfnUserCallback = vkdebugcb;
			msg_info.pUserData = nullptr;

			// create instance
			VkInstanceCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			info.pApplicationInfo = &app_info;
			info.enabledExtensionCount = enabled_extensions.size;
			info.ppEnabledExtensionNames = enabled_extensions.data;
			info.enabledLayerCount = enabled_layers.size;
			info.ppEnabledLayerNames = enabled_layers.data;
			info.pNext = &msg_info;

			vkCreateInstance(&info, nullptr, &_instance);

			// create debug messenger
			auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
			fn(_instance, &msg_info, nullptr, &_debugmsg);

			_window->vk_init(*this);

			// create gpus
			count = 0;
			vkEnumeratePhysicalDevices(_instance, &count, nullptr);
			core::vector<VkPhysicalDevice> devices(count);
			vkEnumeratePhysicalDevices(_instance, &count, devices.data);
			devices.size = count;

			auto device_suitable = [](ref<vk_gpu> gpu, VkSurfaceKHR surface) {
				VkPhysicalDeviceProperties props{};
				vkGetPhysicalDeviceProperties(gpu.physical, &props);

				VkPhysicalDeviceFeatures features{};
				vkGetPhysicalDeviceFeatures(gpu.physical, &features);

				// queue info
				u32 count = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(gpu.physical, &count, nullptr);
				core::vector<VkQueueFamilyProperties> queues(count);
				vkGetPhysicalDeviceQueueFamilyProperties(gpu.physical, &count, queues.data);
				queues.size = count;

				u32 graphics = U32_MAX;
				u32 present = U32_MAX;
				u32 transfer = U32_MAX;
				u32 compute = U32_MAX;

				for (int i : core::range(queues.size)) {
					auto& queue = queues[i];

					VkBool32 present_support = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(gpu.physical, i, surface, &present_support);
					if (present_support) {
						present = i;
					}

					if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
						graphics = i;
					}

					if (queue.queueFlags & VK_QUEUE_TRANSFER_BIT) {
						transfer = i;
					}

					if (queue.queueFlags & VK_QUEUE_COMPUTE_BIT) {
						compute = i;
					}
				}

				core::set<u32> indices = { graphics, present, transfer, compute };
				core::vector<VkDeviceQueueCreateInfo> queue_info(indices.size);
				for (u32 i : indices) {
					if (i == U32_MAX) continue;
					float priority = 1;
					auto& info = queue_info.add();
					info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					info.queueFamilyIndex = i;
					info.queueCount = 1;
					info.pQueuePriorities = &priority;
				}

				// check for extensions
				core::vector<cstr> extensions = {
					VK_KHR_SWAPCHAIN_EXTENSION_NAME
				};

				count = 0;
				vkEnumerateDeviceExtensionProperties(gpu.physical, nullptr, &count, nullptr);
				core::vector<VkExtensionProperties> available(count);
				vkEnumerateDeviceExtensionProperties(gpu.physical, nullptr, &count, available.data);
				available.size = count;

				bool all_extensions = true;
				for (cstr name : extensions) {
					bool found = false;
					for (auto& extension : available) {
						if (core::cmpeq(name, extension.extensionName)) {
							found = true;
							break;
						}
					}

					all_extensions &= found;
					if (!all_extensions) break;
				}

				// check swapchain support
				bool swapchain = true;
				count = 0;
				vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.physical, surface, &count, nullptr);
				swapchain = swapchain && count != 0;
				count = 0;
				vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.physical, surface, &count, nullptr);
				swapchain = swapchain && count != 0;

				// create logical device
				VkPhysicalDeviceFeatures enabled{};
				VkDeviceCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				info.pQueueCreateInfos = queue_info.data;
				info.queueCreateInfoCount = queue_info.size;
				info.pEnabledFeatures = &enabled;
				if (all_extensions) {
					info.ppEnabledExtensionNames = extensions.data;
					info.enabledExtensionCount = extensions.size;
				}
				vkCreateDevice(gpu.physical, &info, nullptr, &gpu.device);

				if (graphics != U32_MAX) {
					vkGetDeviceQueue(gpu.device, graphics, 0, &gpu.graphics.queue);
					gpu.graphics.family = graphics;
				}

				if (present != U32_MAX) {
					vkGetDeviceQueue(gpu.device, present, 0, &gpu.present.queue);
					gpu.present.family = present;
				}

				gpu.transfer = gpu.graphics;
				if (transfer != U32_MAX) {
					vkGetDeviceQueue(gpu.device, transfer, 0, &gpu.transfer.queue);
					gpu.transfer.family = transfer;
				}

				if (compute != U32_MAX) {
					vkGetDeviceQueue(gpu.device, compute, 0, &gpu.compute.queue);
					gpu.compute.family = compute;
				}

				// decide if this device is suitable
				bool all_queues = graphics != U32_MAX && present != U32_MAX;
				bool discrete = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
				return discrete && all_queues && all_extensions && swapchain;
			};

			VkSurfaceKHR surface = _window->vk_main_swapchain().surface;
			for (auto device : devices) {
				ref<vk_gpu> gpu = _gpus.add();
				gpu.physical = device;
				if (device_suitable(gpu, surface) && _gpu == U32_MAX) {
					_gpu = _gpus.size - 1;
				}
			}

			JOLLY_ASSERT(_gpu != U32_MAX, "could not find suitable gpu");

			_window->vk_swapchain_create();
			core::vector<binding_description> bindings{
				binding_description( core::vector<vertex_attribute>{ {vertex_data::vec2, 0}, {vertex_data::vec3, 1} }, vertex_input::vertex ),
			};

			auto& gpu = main_gpu();
			_cmdpool = vk_cmdpool(gpu, gpu.graphics.family);

			pipeline("../assets/shaders/quad", bindings);
		}

		~vk_device() {
			auto& gpu = main_gpu();
			vkDeviceWaitIdle(gpu.device);

			_cmdpool.destroy(gpu);

			vkDestroyBuffer(gpu.device, _pipeline.buffer, nullptr);
			vkFreeMemory(gpu.device, _pipeline.memory, nullptr);

			vkDestroyBuffer(gpu.device, _pipeline.index, nullptr);
			vkFreeMemory(gpu.device, _pipeline.index_memory, nullptr);

			vkDestroyPipelineLayout(gpu.device, _pipeline.layout, nullptr);
			vkDestroyRenderPass(gpu.device, _pipeline.renderpass, nullptr);
			vkDestroyPipeline(gpu.device, _pipeline.pipeline, nullptr);

			_window->vk_term();

			for (auto& gpu : _gpus) {
				vkDestroyDevice(gpu.device, nullptr);
			}

			auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
			fn(_instance, _debugmsg, nullptr);
			vkDestroyInstance(_instance, nullptr);

			_pipeline.layout = VK_NULL_HANDLE;
			_debugmsg = VK_NULL_HANDLE;
			_instance = VK_NULL_HANDLE;
		}

		void pipeline(cref<core::string> fname, cref<core::vector<binding_description>> layout) {
			auto& gpu = main_gpu();

			auto fvertex = core::fopen(core::format_string("%.vert.spv", fname).data, core::access::ro);
			auto vbuf = fvertex.read();

			VkShaderModuleCreateInfo vinfo{};
			vinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vinfo.codeSize = vbuf.size;
			vinfo.pCode = (u32*)vbuf.data;

			auto ffragment = core::fopen(core::format_string("%.frag.spv", fname).data, core::access::ro);
			auto fbuf = ffragment.read();

			VkShaderModuleCreateInfo finfo{};
			finfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			finfo.codeSize = fbuf.size;
			finfo.pCode = (u32*)fbuf.data;

			VkShaderModule vmodule = VK_NULL_HANDLE, fmodule = VK_NULL_HANDLE;
			vkCreateShaderModule(gpu.device, &vinfo, nullptr, &vmodule);
			vkCreateShaderModule(gpu.device, &finfo, nullptr, &fmodule);

			VkPipelineShaderStageCreateInfo vstage{};
			vstage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vstage.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vstage.module = vmodule;
			vstage.pName = "main";

			VkPipelineShaderStageCreateInfo fstage{};
			fstage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fstage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fstage.module = fmodule;
			fstage.pName = "main";

			VkPipelineShaderStageCreateInfo shader_info[] = { vstage, fstage };

			core::vector<VkDynamicState> dynamic = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamic_info{};
			dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamic_info.dynamicStateCount = dynamic.size;
			dynamic_info.pDynamicStates = dynamic.data;

			auto parse_layout = [](cref<core::vector<binding_description>> layout) {
				auto vk_vertex_data_format = [](vertex_data data) {
					using pair = core::pair<VkFormat, u32>;
					switch (data) {
						case vertex_data::vec2:
							return pair{ VK_FORMAT_R32G32_SFLOAT, sizeof(math::vec2f) };
						case vertex_data::vec3:
							return pair{ VK_FORMAT_R32G32B32_SFLOAT, sizeof(math::vec3f) };
					}

					return pair{ VK_FORMAT_UNDEFINED, 0 };
				};

				auto vk_vertex_input_rate = [](vertex_input input) {
					switch (input) {
						case vertex_input::vertex:
							return VK_VERTEX_INPUT_RATE_VERTEX;
						case vertex_input::instance:
							return VK_VERTEX_INPUT_RATE_INSTANCE;
					}

					// probably wrong but whatever
					return VK_VERTEX_INPUT_RATE_VERTEX;
				};

				core::vector<VkVertexInputBindingDescription> bindings(0);
				core::vector<VkVertexInputAttributeDescription> attributes(0);
				for (u32 i : core::range(layout.size)) {
					u32 stride = 0;
					for (auto [data, location] : layout[i].one) {
						auto [format, size] = vk_vertex_data_format(data);
						auto& attribute = attributes.add();
						attribute.binding = i;
						attribute.location = location;
						attribute.format = format;
						attribute.offset = stride;
						stride += size;
					}
					auto& binding = bindings.add();
					binding.binding = i;
					binding.stride = stride;
					binding.inputRate = vk_vertex_input_rate(layout[i].two);
				}

				return core::pair{ bindings, attributes };
			};

			auto [ bindings, attributes ] = parse_layout(layout);
			VkPipelineVertexInputStateCreateInfo vertex_info{};
			vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertex_info.vertexBindingDescriptionCount = bindings.size;
			vertex_info.pVertexBindingDescriptions = bindings.data;
			vertex_info.vertexAttributeDescriptionCount = attributes.size;
			vertex_info.pVertexAttributeDescriptions = attributes.data;
			_pipeline.bindings = bindings.size;

			VkPipelineInputAssemblyStateCreateInfo assembly_info{};
			assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			assembly_info.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewport_info{};
			viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewport_info.viewportCount = 1;
			viewport_info.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterization_info{};
			rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterization_info.depthClampEnable = VK_FALSE;
			rasterization_info.rasterizerDiscardEnable = VK_FALSE;
			rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
			rasterization_info.lineWidth = 1;
			rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterization_info.depthBiasEnable = VK_FALSE;
			rasterization_info.depthBiasConstantFactor = 0;
			rasterization_info.depthBiasClamp = 0;
			rasterization_info.depthBiasSlopeFactor = 0;

			VkPipelineMultisampleStateCreateInfo multisample_info{};
			multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisample_info.sampleShadingEnable = VK_FALSE;
			multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisample_info.minSampleShading = 1;
			multisample_info.pSampleMask = nullptr;
			multisample_info.alphaToCoverageEnable = VK_FALSE;
			multisample_info.alphaToOneEnable = VK_FALSE;

			VkPipelineDepthStencilStateCreateInfo depthstencil_info{};
			depthstencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

			VkPipelineColorBlendAttachmentState colorblend{};
			colorblend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorblend.blendEnable = VK_FALSE;
			colorblend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			colorblend.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorblend.colorBlendOp = VK_BLEND_OP_ADD;
			colorblend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorblend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorblend.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo colorblend_info{};
			colorblend_info.sType= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorblend_info.logicOpEnable = VK_FALSE;
			colorblend_info.logicOp = VK_LOGIC_OP_COPY;
			colorblend_info.attachmentCount = 1;
			colorblend_info.pAttachments = &colorblend;
			colorblend_info.blendConstants[0] = 0;
			colorblend_info.blendConstants[1] = 0;
			colorblend_info.blendConstants[2] = 0;
			colorblend_info.blendConstants[3] = 0;

			VkPipelineLayoutCreateInfo layout_info{};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 0;
			layout_info.pSetLayouts = nullptr;
			layout_info.pushConstantRangeCount = 0;
			layout_info.pPushConstantRanges = nullptr;

			vkCreatePipelineLayout(gpu.device, &layout_info, nullptr, &_pipeline.layout);

			VkAttachmentDescription color_attachment{};
			color_attachment.format = _window->surface->format;
			color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference color_attachmentref{};
			color_attachmentref.attachment = 0;
			color_attachmentref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachmentref;

			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkRenderPassCreateInfo renderpass_info{};
			renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderpass_info.attachmentCount = 1;
			renderpass_info.pAttachments = &color_attachment;
			renderpass_info.subpassCount = 1;
			renderpass_info.pSubpasses = &subpass;
			renderpass_info.dependencyCount = 1;
			renderpass_info.pDependencies = &dependency;

			vkCreateRenderPass(gpu.device, &renderpass_info, nullptr, &_pipeline.renderpass);

			VkGraphicsPipelineCreateInfo pipeline_info{};
			pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_info.stageCount = 2;
			pipeline_info.pStages = shader_info;
			pipeline_info.pVertexInputState = &vertex_info;
			pipeline_info.pInputAssemblyState = &assembly_info;
			pipeline_info.pViewportState = &viewport_info;
			pipeline_info.pRasterizationState = &rasterization_info;
			pipeline_info.pMultisampleState = &multisample_info;
			pipeline_info.pDepthStencilState = nullptr;
			pipeline_info.pColorBlendState = &colorblend_info;
			pipeline_info.pDynamicState = &dynamic_info;
			pipeline_info.layout = _pipeline.layout;
			pipeline_info.renderPass = _pipeline.renderpass;
			pipeline_info.subpass = 0;
			pipeline_info.basePipelineHandle = nullptr;
			pipeline_info.basePipelineIndex = -1;

			vkCreateGraphicsPipelines(gpu.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_pipeline.pipeline);

			for (u32 id : _window->windows.keys()) {
				vk_framebuffer_create_cb(_window.get(), id, win_event::create);
			}

			_window->add_cb(win_event::create, vk_framebuffer_create_cb);

			{
				auto create_buffer = [](cref<vk_gpu> gpu, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
					VkBufferCreateInfo buffer_info{};
					buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					buffer_info.size = size;
					buffer_info.usage = usage;
					buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

					VkBuffer buffer = VK_NULL_HANDLE;
					vkCreateBuffer(gpu.device, &buffer_info, nullptr, &buffer);

					VkMemoryRequirements mem_requirements{};
					vkGetBufferMemoryRequirements(gpu.device, buffer, &mem_requirements);

					VkPhysicalDeviceMemoryProperties mem_properties{};
					vkGetPhysicalDeviceMemoryProperties(gpu.physical, &mem_properties);

					u32 type_index = U32_MAX;
					for (i32 i : core::range(mem_properties.memoryTypeCount)) {
						bool type_match = mem_requirements.memoryTypeBits & (1 << i);
						bool property_match = (mem_properties.memoryTypes[i].propertyFlags & properties) == properties;
						if (type_match && property_match) {
							type_index = i;
							break;
						}
					}

					VkMemoryAllocateInfo alloc_info{};
					alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					alloc_info.allocationSize = mem_requirements.size;
					alloc_info.memoryTypeIndex = type_index;

					VkDeviceMemory memory = VK_NULL_HANDLE;
					vkAllocateMemory(gpu.device, &alloc_info, nullptr, &memory);
					vkBindBufferMemory(gpu.device, buffer, memory, 0);

					return core::pair { buffer, memory };
				};

				core::vector<f32> vertices = {
					-0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
					+0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
					+0.5f, +0.5f, 0.0f, 0.0f, 1.0f,
					-0.5f, +0.5f, 1.0f, 1.0f, 1.0f
				};

				core::vector<u32> indices = {
					0, 1, 2, 2, 3, 0
				};

				u32 vdata_size = vertices.size * sizeof(f32);
				auto [ vstaging_buffer, vstaging_memory ] = create_buffer(gpu, vdata_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				void* gpu_memory = nullptr;
				vkMapMemory(gpu.device, vstaging_memory, 0, vdata_size, 0, &gpu_memory);
				core::copy8((u8*)vertices.data, (u8*)gpu_memory, vdata_size);
				vkUnmapMemory(gpu.device, vstaging_memory);

				u32 idata_size = indices.size * sizeof(u32);
				auto [ istaging_buffer, istaging_memory ] = create_buffer(gpu, idata_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				vkMapMemory(gpu.device, istaging_memory, 0, idata_size, 0, &gpu_memory);
				core::copy8((u8*)indices.data, (u8*)gpu_memory, idata_size);
				vkUnmapMemory(gpu.device, istaging_memory);

				auto [ vertex_buffer, vertex_memory ] = create_buffer(gpu, vdata_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				auto [ index_buffer, index_memory ] = create_buffer(gpu, idata_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				VkCommandBufferAllocateInfo alloc_info{};
				alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				alloc_info.commandPool = _cmdpool.pool;
				alloc_info.commandBufferCount = 1;

				VkCommandBuffer cmdbuf = VK_NULL_HANDLE;
				vkAllocateCommandBuffers(gpu.device, &alloc_info, &cmdbuf);

				VkCommandBufferBeginInfo begin_info{};
				begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(cmdbuf, &begin_info);

				VkBufferCopy copy_region{};
				copy_region.srcOffset = 0;
				copy_region.dstOffset = 0;
				copy_region.size = vdata_size;
				vkCmdCopyBuffer(cmdbuf, vstaging_buffer, vertex_buffer, 1, &copy_region);

				copy_region.size = idata_size;
				vkCmdCopyBuffer(cmdbuf, istaging_buffer, index_buffer, 1, &copy_region);

				vkEndCommandBuffer(cmdbuf);

				VkSubmitInfo submit_info{};
				submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.commandBufferCount = 1;
				submit_info.pCommandBuffers = &cmdbuf;

				vkQueueSubmit(gpu.graphics.queue, 1, &submit_info, VK_NULL_HANDLE);
				vkQueueWaitIdle(gpu.graphics.queue);

				vkFreeCommandBuffers(gpu.device, _cmdpool.pool, 1, &cmdbuf);
				vkDestroyBuffer(gpu.device, vstaging_buffer, nullptr);
				vkFreeMemory(gpu.device, vstaging_memory, nullptr);
				vkDestroyBuffer(gpu.device, istaging_buffer, nullptr);
				vkFreeMemory(gpu.device, istaging_memory, nullptr);

				_pipeline.buffer = vertex_buffer;
				_pipeline.memory = vertex_memory;
				_pipeline.index = index_buffer;
				_pipeline.index_memory = index_memory;
			}

			vkDestroyShaderModule(gpu.device, vmodule, nullptr);
			vkDestroyShaderModule(gpu.device, fmodule, nullptr);
		}

		ref<vk_gpu> main_gpu() const {
			return _gpus[_gpu];
		}

		void step(f32 ms) {
			_window->step(ms);

			auto& gpu = main_gpu();
			auto& surface = _window->surface.get();

			vkWaitForFences(gpu.device, 1, &surface.fences[_frame], VK_TRUE, U64_MAX);

			_cmdpool.gc(gpu);

			core::vector<VkCommandBuffer> cmdbufs(0);
			core::vector<VkSemaphore> wait_semaphores(0);
			core::vector<VkSemaphore> signal_semaphores(0);
			core::vector<VkPipelineStageFlags> wait_stages(0);
			core::vector<VkSwapchainKHR> swapchains(0);
			core::vector<u32> image_indices(0);
			for (u32 i : _window->windows.keys()) {
				auto& swapchain = surface.surfaces[i];
				if (!swapchain.busy.tryacquire()) {
					continue;
				}

				VkCommandBuffer cmdbuf = _cmdpool.create(gpu);
				vkResetCommandBuffer(cmdbuf, 0);

				VkCommandBufferBeginInfo begin_info{};
				begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.flags = 0;
				begin_info.pInheritanceInfo = nullptr;
				vkBeginCommandBuffer(cmdbuf, &begin_info);

				u32 image_index = U32_MAX;
				vkAcquireNextImageKHR(gpu.device, swapchain.swapchain, U64_MAX, swapchain.image_available[_frame], VK_NULL_HANDLE, &image_index);
				VkRenderPassBeginInfo renderpass_info{};
				renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderpass_info.renderPass = _pipeline.renderpass;
				renderpass_info.framebuffer = swapchain.framebuffers[image_index];
				renderpass_info.renderArea.offset = { 0, 0 };
				renderpass_info.renderArea.extent = swapchain.extent;

				VkClearValue clear = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
				renderpass_info.clearValueCount = 1;
				renderpass_info.pClearValues = &clear;

				vkCmdBeginRenderPass(cmdbuf, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.pipeline);

				VkBuffer vertex_buffers[] = { _pipeline.buffer };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(cmdbuf, 0, 1, vertex_buffers, offsets);
				vkCmdBindIndexBuffer(cmdbuf, _pipeline.index, 0, VK_INDEX_TYPE_UINT32);

				VkViewport viewport{};
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = (float)swapchain.extent.width;
				viewport.height = (float)swapchain.extent.height;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(cmdbuf, 0, 1, &viewport);

				VkRect2D scissor{};
				scissor.offset = { 0, 0 };
				scissor.extent = swapchain.extent;
				vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

				vkCmdDrawIndexed(cmdbuf, 6, 1, 0, 0, 0);
				vkCmdEndRenderPass(cmdbuf);
				vkEndCommandBuffer(cmdbuf);

				cmdbufs.add(cmdbuf);
				wait_semaphores.add(swapchain.image_available[_frame]);
				signal_semaphores.add(swapchain.render_finished[_frame]);
				wait_stages.add(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
				swapchains.add(swapchain.swapchain);
				image_indices.add(image_index);

				_cmdpool.destroy(cmdbuf, surface.fences[_frame]);
				swapchain.busy.release();
			}

			if (!swapchains.size) return;

			VkSubmitInfo submit_info{};
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.waitSemaphoreCount = wait_semaphores.size;
			submit_info.pWaitSemaphores = wait_semaphores.data;
			submit_info.pWaitDstStageMask = wait_stages.data;
			submit_info.commandBufferCount = cmdbufs.size;
			submit_info.pCommandBuffers = cmdbufs.data;
			submit_info.signalSemaphoreCount = signal_semaphores.size;
			submit_info.pSignalSemaphores = signal_semaphores.data;

			vkResetFences(gpu.device, 1, &surface.fences[_frame]);
			vkQueueSubmit(gpu.graphics.queue, 1, &submit_info, surface.fences[_frame]);

			VkPresentInfoKHR present_info{};
			present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			present_info.waitSemaphoreCount = signal_semaphores.size;
			present_info.pWaitSemaphores = signal_semaphores.data;
			present_info.swapchainCount = swapchains.size;
			present_info.pSwapchains = swapchains.data;
			present_info.pImageIndices = image_indices.data;
			present_info.pResults = nullptr;

			vkQueuePresentKHR(gpu.present.queue, &present_info);

			_frame = (_frame + 1) % MAX_FRAMES_IN_FLIGHT;
		}

		core::ptr<window> _window;
		VkInstance _instance;
		VkDebugUtilsMessengerEXT _debugmsg;

		vk_pipeline _pipeline;
		vk_cmdpool _cmdpool;

		core::vector<vk_gpu> _gpus;
		u32 _gpu; // gpu we use to present and render
		u32 _frame;
	};
}
