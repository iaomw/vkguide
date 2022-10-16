#pragma once

#include "vk_descriptor.h"
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

	uint32_t hash_descriptor_layout_info(VkDescriptorSetLayoutCreateInfo* info);
}

struct ShaderEffect {

    struct ReflectionOverrides {
        const char* name;
        VkDescriptorType overridenType; 
    };

    void add_stage(ShaderModule* shaderModule, VkShaderStageFlagBits stage);
    void fill_stages(std::vector<VkPipelineShaderStageCreateInfo>& pipelineStages);

	void reflect_layout(VkDevice device, ReflectionOverrides* overrides, int overrideCount);

    struct ReflectedBinding {
        uint32_t set;
        uint32_t binding;
        VkDescriptorType type;
    };

    std::unordered_map<std::string, ReflectedBinding> bindings;
	std::array<VkDescriptorSetLayout, 4> setLayouts;
	std::array<uint32_t, 4> setHashes;

    VkPipelineLayout builtLayout;

private:
    struct ShaderStage {
        ShaderModule* shaderModule;
        VkShaderStageFlagBits stageFlag;
    };

    std::vector<ShaderStage> stages;
};

struct ShaderDesriptorBinder {

    struct BufferWriteDescriptor {
        int dstSet;
        int dstBingding;

        VkDescriptorType descriptorType;
        VkDescriptorBufferInfo bufferInfo;

        uint32_t dynamic_offset;
    };

    void bind_buffer(const char* name, const VkDescriptorBufferInfo& bufferInfo);

	void bind_dynamic_buffer(const char* name, uint32_t offset, const VkDescriptorBufferInfo& bufferInfo);

    void apply_binds(VkCommandBuffer cmd);

    void build_sets(VkDevice device, DescriptorAllocator& allocator);

    void set_shader(ShaderEffect* newShader);

    std::array<VkDescriptorSet, 4> cacheDescriptorSets;

private:

    ShaderEffect* shaders {nullptr};

    struct DynamicOffsets {
        std::array<uint32_t, 16> offsets;
        uint32_t count {0};
    };

    std::array<DynamicOffsets, 4> setOffsets;

    std::vector<BufferWriteDescriptor> bufferWrites;
};

class ShaderCache {
public:
    ShaderModule* get_shader(const std::string& path);
    
    void init(VkDevice device) { _device = device; }

private:
    VkDevice _device;
    std::unordered_map<std::string, ShaderModule> module_cache;
};