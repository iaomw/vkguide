#include "vk_descriptor.h"
#include <vulkan/vulkan_core.h>

void DescriptorAllocator::init(VkDevice newDevice) 
{
    device = newDevice;
}

void DescriptorAllocator::cleanup() 
{
    for (auto p : freePools) 
        vkDestroyDescriptorPool(device, p, nullptr);
    
    for (auto p : usedPools) 
        vkDestroyDescriptorPool(device, p, nullptr);
}

VkDescriptorPool createPool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags)
{
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(poolSizes.sizes.size());

    for (auto sz : poolSizes.sizes) {
		sizes.push_back({ sz.first, uint32_t(sz.second * count) });
    }

    VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = flags;
		pool_info.maxSets = count;
		pool_info.poolSizeCount = (uint32_t)sizes.size();
		pool_info.pPoolSizes = sizes.data();

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

    return descriptorPool;  
}

VkDescriptorPool DescriptorAllocator::grab_pool() 
{
	if (freePools.size() > 0) {
		VkDescriptorPool pool = freePools.back();
		freePools.pop_back();
		return pool;
	}

	return createPool(device, descriptorSizes, 1000, 0);
}

bool DescriptorAllocator::allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
{
	if (currentPool == VK_NULL_HANDLE) {
		currentPool = grab_pool();
		usedPools.push_back(currentPool);
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;

	allocInfo.pSetLayouts = &layout;
	allocInfo.descriptorPool = currentPool;
	allocInfo.descriptorSetCount = 1;

	auto allocResult =  vkAllocateDescriptorSets(device, &allocInfo, set);

	bool needReallocate = false;

	switch (allocResult) {
		case VK_SUCCESS:
			//all good, return
			return true;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			//reallocate pool
			needReallocate = true;
			break;
		default:
			//unrecoverable error
			return false;
	}

	if (needReallocate){
		//allocate a new pool and retry
		currentPool = grab_pool();
		usedPools.push_back(currentPool);

		allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);

		//if it still fails then we have big issues
		if (allocResult == VK_SUCCESS){
			return true;
		}
	}

	return false;
}

void DescriptorAllocator::reset_pools() 
{
	for (auto p : usedPools) {
		vkResetDescriptorPool(device, p, 0);
		freePools.push_back(p);
	}

	usedPools.clear();

	currentPool = VK_NULL_HANDLE;
}

void DescriptorLayoutCache::init(VkDevice newDevice) 
{
	device = newDevice;
}

void DescriptorLayoutCache::cleanup() 
{
	for (auto pair : layoutCache) {
		vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
	}
}

VkDescriptorSetLayout DescriptorLayoutCache::create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info)
{
	DescriptorLayoutInfo layoutInfo {};
	layoutInfo.bindings.reserve(info->bindingCount);

	bool isSorted = true;
	int lastBinding = -1;

	for (int i=0; i < info->bindingCount; i++) {
		layoutInfo.bindings.push_back((info->pBindings[i]));

		if (info->pBindings[i].binding > lastBinding) {
			lastBinding = info->pBindings[i].binding;
		} else {
			isSorted = false;
		}
	}

	if (!isSorted) {
		std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b ){
			return a.binding < b.binding;
		});
	}

	auto it = layoutCache.find(layoutInfo);

	if (it != layoutCache.end()) {
		return (*it).second;
	} 
	else {
		VkDescriptorSetLayout layout;
		vkCreateDescriptorSetLayout(device, info, nullptr, &layout);

		layoutCache[layoutInfo] = layout;
		return layout;
	}
}