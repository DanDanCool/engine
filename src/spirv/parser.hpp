module;

#include <core/core.h>
#include <spirv_reflect.h>

export module jolly.spirv.parser;
import core.types;
import core.string;
import core.vector;
import core.file;
import core.format;
import core.log;
import core.iterator;

export namespace jolly {
	cstr vktypename(SpvReflectFormat type) {
		switch(type) {
			case SPV_REFLECT_FORMAT_UNDEFINED: {
				return "undefined";
											   }
			case SPV_REFLECT_FORMAT_R16_UINT: {
				return "u16";
											  }
			case SPV_REFLECT_FORMAT_R16_SINT: {
				return "i16";
											  }
			case SPV_REFLECT_FORMAT_R16_SFLOAT: {
				return "f16";
												}
			case SPV_REFLECT_FORMAT_R16G16_UINT: {
				return "vec2<u16>";
												 }
			case SPV_REFLECT_FORMAT_R16G16_SINT: {
				return "vec2<i16>";
												 }
			case SPV_REFLECT_FORMAT_R16G16_SFLOAT: {
				return "vec2<f16>";
												   }
			case SPV_REFLECT_FORMAT_R16G16B16_UINT: {
				return "vec3<u16>";
													}
			case SPV_REFLECT_FORMAT_R16G16B16_SINT: {
				return "vec3<i16>";
													}
			case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT: {
				return "vec3<f16>";
													  }
			case SPV_REFLECT_FORMAT_R16G16B16A16_UINT: {
				return "vec4<u16>";
													   }
			case SPV_REFLECT_FORMAT_R16G16B16A16_SINT: {
				return "vec4<i16>";
													   }
			case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT: {
				return "vec4<f16>";
														 }
			case SPV_REFLECT_FORMAT_R32_UINT: {
				return "u32";
											  }
			case SPV_REFLECT_FORMAT_R32_SINT: {
				return "i32";
											  }
			case SPV_REFLECT_FORMAT_R32_SFLOAT: {
				return "f32";
												}

			case SPV_REFLECT_FORMAT_R32G32_UINT: {
				return "vec2<u32>";
												 }
			case SPV_REFLECT_FORMAT_R32G32_SINT: {
				return "vec2<i32>";
												 }
			case SPV_REFLECT_FORMAT_R32G32_SFLOAT: {
				return "vec2<f32>";
												   }
			case SPV_REFLECT_FORMAT_R32G32B32_UINT: {
				return "vec3<u32>";
													}
			case SPV_REFLECT_FORMAT_R32G32B32_SINT: {
				return "vec3<i32>";
													}
			case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: {
				return "vec3<f32>";
													  }
			case SPV_REFLECT_FORMAT_R32G32B32A32_UINT: {
				return "vec4<u32>";
													   }
			case SPV_REFLECT_FORMAT_R32G32B32A32_SINT: {
				return "vec4<i32>";
													   }
			case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: {
				return "vec4<f32>";
														 }
			case SPV_REFLECT_FORMAT_R64_UINT: {
				return "u64";
											  }
			case SPV_REFLECT_FORMAT_R64_SINT: {
				return "i64";
											  }
			case SPV_REFLECT_FORMAT_R64_SFLOAT: {
				return "f64";
												}
			case SPV_REFLECT_FORMAT_R64G64_UINT: {
				return "vec2<u64>";
												 }
			case SPV_REFLECT_FORMAT_R64G64_SINT: {
				return "vec2<i64>";
												 }
			case SPV_REFLECT_FORMAT_R64G64_SFLOAT: {
				return "vec2<f64>";
												   }
			case SPV_REFLECT_FORMAT_R64G64B64_UINT: {
				return "vec3<u64>";
													}
			case SPV_REFLECT_FORMAT_R64G64B64_SINT: {
				return "vec3<i64>";
													}
			case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT: {
				return "vec3<f64>";
													  }
			case SPV_REFLECT_FORMAT_R64G64B64A64_UINT: {
				return "vec4<u64>";
													   }
			case SPV_REFLECT_FORMAT_R64G64B64A64_SINT: {
				return "vec4<i64>";
													   }
			case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT: {
				return "vec4<f64>";
														 }

		}

		return "unknown";
	}

	void module_parse(ref<SpvReflectShaderModule> module) {
		u32 count = 0;
		JOLLY_CORE_ASSERT(spvReflectEnumerateInputVariables(&module, &count, NULL) == SPV_REFLECT_RESULT_SUCCESS);
		core::vector<ptr<SpvReflectInterfaceVariable>> input_vars(count);
		JOLLY_CORE_ASSERT(spvReflectEnumerateInputVariables(&module, &count, input_vars.data) == SPV_REFLECT_RESULT_SUCCESS);
		input_vars.size = count;

		LOG_INFO("spirv input variables");

		// all instanced inputs must end with '_instance'
		for (auto in : input_vars) {
			if (in->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) continue;
			LOG_INFO("% with type % at location %", in->name, vktypename(in->format), in->location);
		}

		count = 0;
		JOLLY_CORE_ASSERT(spvReflectEnumerateOutputVariables(&module, &count, NULL) == SPV_REFLECT_RESULT_SUCCESS);
		core::vector<ptr<SpvReflectInterfaceVariable>> output_vars(count);
		JOLLY_CORE_ASSERT(spvReflectEnumerateOutputVariables(&module, &count, output_vars.data) == SPV_REFLECT_RESULT_SUCCESS);
		output_vars.size = count;

		LOG_INFO("spirv output variables");
		for (auto in : output_vars) {
			if (in->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) continue;
			LOG_INFO("% with type % at location %", in->name, vktypename(in->format), in->location);
		}

		count = 0;
		JOLLY_CORE_ASSERT(spvReflectEnumerateDescriptorSets(&module, &count, NULL) == SPV_REFLECT_RESULT_SUCCESS);
		core::vector<ptr<SpvReflectDescriptorSet>> descriptor_sets(count);
		JOLLY_CORE_ASSERT(spvReflectEnumerateDescriptorSets(&module, &count, descriptor_sets.data) == SPV_REFLECT_RESULT_SUCCESS);
		descriptor_sets.size = count;

		LOG_INFO("spirv descriptor sets");
		for (auto in: descriptor_sets) {
			LOG_INFO("set % with % elements", in->set, in->binding_count);
			for (u32 i : core::range(in->binding_count)) {
				auto* descriptor_binding = in->bindings[i];
				LOG_INFO("% at binding %", descriptor_binding->name, descriptor_binding->binding);
			}
		}
	}

	void spirv_parse(cref<core::string> fname) {
		core::vector<u8> vbuf, fbuf;

		{
			auto vertex = core::fopen(core::format_string("%.vert.spv", fname), core::access::ro);
			vertex.read(vbuf);

			auto fragment = core::fopen(core::format_string("%.frag.spv", fname), core::access::ro);
			fragment.read(fbuf);
		}

		SpvReflectShaderModule vertex, fragment;
		JOLLY_CORE_ASSERT(spvReflectCreateShaderModule(vbuf.size, vbuf.data, &vertex) == SPV_REFLECT_RESULT_SUCCESS);
		JOLLY_CORE_ASSERT(spvReflectCreateShaderModule(fbuf.size, fbuf.data, &fragment) == SPV_REFLECT_RESULT_SUCCESS);

		LOG_INFO("% vertex module info", fname);
		module_parse(vertex);

		LOG_INFO("% fragment module info", fname);
		module_parse(fragment);
	}
}
