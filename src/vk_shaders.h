﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <cstdint>
#include <vk_types.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <vector>

#include <string>
#include <unordered_map>

struct ShaderModule {
	std::vector<uint32_t> code;
	VkShaderModule module;
};
namespace vkutil {
	
	//loads a shader module from a spir-v file. Returns false if it errors	
	bool load_shader_module(VkDevice device, const char* filePath, ShaderModule* outShaderModule);
}



class VulkanEngine;
//holds all information for a given shader set for pipeline
struct ShaderEffect {

	struct ReflectionOverrides {
		const char* name;
		VkDescriptorType overridenType;
	};


	void add_stage(ShaderModule* shaderModule, VkShaderStageFlagBits stage);

	void reflect_layout(VulkanEngine* engine, ReflectionOverrides* overrides, int overrideCount);
	VkPipelineLayout builtLayout;

	struct ReflectedBinding {
		uint32_t set;
		uint32_t binding;
		VkDescriptorType type;
	};

	std::unordered_map<std::string, ReflectedBinding> bindings;
	std::array<VkDescriptorSetLayout, 4> setLayouts;

private:
	struct ShaderStage {
		ShaderModule* shaderModule;
		VkShaderStageFlagBits stage;
	};

	std::vector<ShaderStage> stages;
};

struct DescriptorBuilder {

	struct BufferWriteDescriptor {
		int dstSet;
		int dstBinding;

		VkDescriptorType descriptorType;
		VkDescriptorBufferInfo bufferInfo;

		uint32_t dynamic_offset;
	};

	void bind_buffer(const char* name, const VkDescriptorBufferInfo& bufferInfo);

	void bind_dynamic_buffer(const char* name, uint32_t offset,const VkDescriptorBufferInfo& bufferInfo);

	void apply_binds(VkDevice device, VkCommandBuffer cmd, VkDescriptorPool allocator);

	void set_shader(ShaderEffect* newShader);

	std::array<VkDescriptorSet, 4> cachedDescriptorSets;

private:
	ShaderEffect* shaders{ nullptr };
	std::vector<BufferWriteDescriptor> bufferWrites;
};