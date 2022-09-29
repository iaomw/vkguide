// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

//#include <vk_types.h>
#include <vk_mesh.h>
#include "VkBootstrap.h"
//#include "vk_mem_alloc.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <deque>
#include <functional>
#include <unordered_map>

#include <fstream>
#include <iostream>

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call the function
		}

		deletors.clear();
	}
};

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh* mesh;

	Material* material;

	glm::mat4 transformMatrix;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

class VulkanEngine {
public:

	//default array of renderable objects
	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string,Mesh> _meshes;
	std::unordered_map<std::string,Material> _materials;

	vkb::Instance vkb_inst;
	VmaAllocator _allocator; //vma lib allocator

	// --- omitted ---
    VkInstance _instance; // Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
	VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface; // Vulkan window surface

	VkSwapchainKHR _swapchain; // from other articles
	// image format expected by the windowing system
	VkFormat _swapchainImageFormat;
	//array of images from the swapchain
	std::vector<VkImage> _swapchainImages;
	//array of image-views from the swapchain
	std::vector<VkImageView> _swapchainImageViews;

	VkQueue _graphicsQueue; //queue we will submit to
	uint32_t _graphicsQueueFamily; //family of that queue

	VkCommandPool _commandPool; //the command pool for our commands
	VkCommandBuffer _mainCommandBuffer; //the buffer we will record into

	VkRenderPass _renderPass;
	std::vector<VkFramebuffer> _framebuffers;

	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _meshPipeline;

	DeletionQueue _mainDeletionQueue;

	VkImageView _depthImageView;
	AllocatedImage _depthImage;

	//the format for the depth image
	VkFormat _depthFormat;

	bool _isInitialized{ false };
	int _frameNumber {0};

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	int _selectedShader{ 0 };

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private:
	void init_vulkan();
	void init_swapchain();

	void init_commands();

	void init_default_renderpass();
	void init_framebuffers();

	void init_sync_structures();
	//loads a shader module from a spir-v file. Returns false if it errors
	bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);

	void init_pipelines();

	void load_meshes();
	void upload_mesh(Mesh& mesh);
	void init_scene();

	//create material and add it to the map
	Material* create_material(VkPipeline pipeline, VkPipelineLayout layout,const std::string& name);

	//returns nullptr if it can't be found
	Material* get_material(const std::string& name);

	//returns nullptr if it can't be found
	Mesh* get_mesh(const std::string& name);

	//our draw function
	void draw_objects(VkCommandBuffer cmd,RenderObject* first, int count);
};


class PipelineBuilder {
public:

	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;

	VkPipelineDepthStencilStateCreateInfo _depthStencil;

	VkViewport _viewport;
	VkRect2D _scissor;

	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;

	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};