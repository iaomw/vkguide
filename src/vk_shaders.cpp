#include <spirv_reflect.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <vk_shaders.h>
#include <vk_initializers.h>

#include <fstream>
#include <sstream>
#include <ostream>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool vkutil::load_shader_module(VkDevice _device, const char *filePath, ShaderModule *outShaderModule)
{
	//open the file. With cursor at the end
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::cout <<"Failed to open: " << filePath << std::endl; 
		return false;
	}

	//find what the size of the file is by looking up the location of the cursor
	//because the cursor is at the end, it gives the size directly in bytes
	size_t fileSize = (size_t)file.tellg();
	 //{
		size_t _remain = fileSize % sizeof(uint32_t);
		if (_remain > 0) {
			fileSize = fileSize - _remain + sizeof(uint32_t);
		}
		size_t _count = fileSize / sizeof(uint32_t);
	//}

	//spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
	std::vector<uint32_t> buffer(_count);

	//put file cursor at beginning
	file.seekg(0);

	//load the entire file into the buffer
	file.read((char*)buffer.data(), fileSize);

	//now that the file is loaded into the buffer, we can close it
	file.close();

	//create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	//codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	//check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}

    outShaderModule->code = std::move(buffer);
	outShaderModule->module = shaderModule;
	return true;
}

// FNV-1a 32bit hashing algorithm.
constexpr uint32_t fnv1a_32(char const* s, std::size_t count)
{
	return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
}

uint32_t vkutil::hash_descriptor_layout_info(VkDescriptorSetLayoutCreateInfo* info)
{
	//we are going to put all the data into a string and then hash the string
	std::stringstream ss;

	ss << info->flags;
	ss << info->bindingCount;

	for (auto i = 0u; i < info->bindingCount; i++) {
		const VkDescriptorSetLayoutBinding &binding = info->pBindings[i];

		ss << binding.binding;
		ss << binding.descriptorCount;
		ss << binding.descriptorType;
		ss << binding.stageFlags;
	}

	auto str = ss.str();

	return fnv1a_32(str.c_str(),str.length());
}

void ShaderEffect::add_stage(ShaderModule *shaderModule, VkShaderStageFlagBits stage) 
{
    ShaderStage newStage = { shaderModule, stage };
    stages.push_back(newStage);
}

struct DescriptorSetLayoutData {
    uint32_t set_number;

    VkDescriptorSetLayoutCreateInfo create_info;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

void ShaderEffect::reflect_layout(VkDevice device, ReflectionOverrides *overrides, int overrideCount) 
{
    std::vector<DescriptorSetLayoutData> set_layouts;

    std::vector<VkPushConstantRange> constant_ranges;

    for (auto& stage : stages) {
        
        SpvReflectShaderModule moduleSPV;
        auto result = spvReflectCreateShaderModule(stage.shaderModule->code.size() * sizeof(uint32_t), stage.shaderModule->code.data(), &moduleSPV);

        uint32_t count = 0;
        result = spvReflectEnumerateDescriptorSets(&moduleSPV, &count, NULL);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectDescriptorSet*> sets(count);
        result = spvReflectEnumerateDescriptorSets(&moduleSPV, &count, sets.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        for (size_t i_set = 0; i_set < sets.size(); ++i_set) {
            const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
            DescriptorSetLayoutData layoutData = {};

            layoutData.bindings.resize(refl_set.binding_count);

            for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) {
                
                const SpvReflectDescriptorBinding& ref_binding = *(refl_set.bindings[i_binding]);
                VkDescriptorSetLayoutBinding& layout_binding = layoutData.bindings[i_binding];

                layout_binding.binding = ref_binding.binding;
                layout_binding.descriptorType = static_cast<VkDescriptorType>(ref_binding.descriptor_type);

                for (uint32_t ov=0; ov < overrideCount; ov++) {
                    if (strcmp(ref_binding.name, overrides[ov].name) == 0) {
                        layout_binding.descriptorType = overrides[ov].overridenType;
                    } 
                }

                layout_binding.descriptorCount = 1;

                for (uint32_t i_dim = 0; i_dim<ref_binding.array.dims_count; ++i_dim) {
                    layout_binding.descriptorCount *= ref_binding.array.dims[i_dim];
                }
                layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(moduleSPV.shader_stage);

                ReflectedBinding reflected;
                reflected.binding = layout_binding.binding;
                reflected.set = refl_set.set;
                reflected.type = layout_binding.descriptorType;

                bindings[ref_binding.name] = reflected;

            } // DescriptorBindings

            layoutData.set_number = refl_set.set;
            layoutData.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutData.create_info.bindingCount = refl_set.binding_count;
            layoutData.create_info.pBindings = layoutData.bindings.data();

            set_layouts.push_back(layoutData);

        } // DescriptorSets

        result = spvReflectEnumeratePushConstantBlocks(&moduleSPV, &count, NULL);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectBlockVariable*> pconstants(count);
        result = spvReflectEnumeratePushConstantBlocks(&moduleSPV, &count, pconstants.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        if (count > 0) {
            VkPushConstantRange pcs {};
            pcs.offset = pconstants[0]->offset;
            pcs.size = pconstants[0]->size;
            pcs.stageFlags = stage.stageFlag;

            constant_ranges.push_back(pcs);
        }

    } // stages

    std::array<DescriptorSetLayoutData, 4> merged_layouts;

    for (uint i = 0; i < 4; i++) {

        auto& ly = merged_layouts[i];

        ly.set_number = i;
        ly.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::unordered_map<int, VkDescriptorSetLayoutBinding> binds;

        for (auto& sl : set_layouts) {
            if (sl.set_number == i) {
                for (auto& b : sl.bindings) {
                    auto it = binds.find(b.binding);
					if (it == binds.end())
					{
						binds[b.binding] = b;
						//ly.bindings.push_back(b);
					}
					else {
						//merge flags
						binds[b.binding].stageFlags |= b.stageFlags;
					}
                } // bindings
            }
        }

        for (auto [k, v] : binds)
		{
			ly.bindings.push_back(v);
		}
		//sort the bindings, for hash purposes
		std::sort(ly.bindings.begin(), ly.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) {			
			return a.binding < b.binding;
		});


		ly.create_info.bindingCount = (uint32_t)ly.bindings.size();
		ly.create_info.pBindings = ly.bindings.data();
		ly.create_info.flags = 0;
		ly.create_info.pNext = 0;
		

		if (ly.create_info.bindingCount > 0) {
			setHashes[i] = vkutil::hash_descriptor_layout_info(&ly.create_info);
			vkCreateDescriptorSetLayout(device, &ly.create_info, nullptr, &setLayouts[i]);
		}
		else {
			setHashes[i] = 0;
			setLayouts[i] = VK_NULL_HANDLE;
		}

    } // DescriptorSetLayoutData

    //we start from just the default empty pipeline layout info
	VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();

	mesh_pipeline_layout_info.pPushConstantRanges = constant_ranges.data();
	mesh_pipeline_layout_info.pushConstantRangeCount = (uint32_t)constant_ranges.size();

	std::array<VkDescriptorSetLayout,4> compactedLayouts;
	int s = 0;
	for (int i = 0; i < 4; i++) {
		if (setLayouts[i] != VK_NULL_HANDLE) {
			compactedLayouts[s] = setLayouts[i];
			s++;
		}
	}

	mesh_pipeline_layout_info.setLayoutCount = s;
	mesh_pipeline_layout_info.pSetLayouts = compactedLayouts.data();

	
	vkCreatePipelineLayout(device, &mesh_pipeline_layout_info, nullptr, &builtLayout);

}