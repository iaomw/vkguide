#include <vk_pipeline.h>

#include <vk_initializers.h>

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
	_vertexInputInfo = vkinit::vertex_input_state_create_info();
    //connect the pipeline builder vertex input info to the one we get from Vertex
	_vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	_vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();

	_vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	_vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexDescription.bindings.size();


	//make viewport state from our stored viewport and scissor.
		//at the moment we wont support multiple viewports or scissors
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.pViewports = &_viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &_scissor;

	//setup dummy color blending. We arent using transparent objects yet
	//the blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &_colorBlendAttachment;

	//build the actual pipeline
	//we now use all of the info structs we have been writing into into this one to create the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;

	pipelineInfo.stageCount = (uint32_t)_shaderStages.size();
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &_depthStencil;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;


	std::vector<VkDynamicState> dynamicStates;
	dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
	dynamicState.pDynamicStates = dynamicStates.data();
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();

	pipelineInfo.pDynamicState = &dynamicState;

	//its easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(
		device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		//LOG_FATAL("Failed to build graphics pipeline");
		return VK_NULL_HANDLE;
	}
	else
	{
		return newPipeline;
	}
}

void ShaderEffect::fill_stages(std::vector<VkPipelineShaderStageCreateInfo>& pipelineStages)
{
	for (auto& s : stages)
	{
		pipelineStages.push_back(vkinit::pipeline_shader_stage_create_info(s.stageFlag, s.shaderModule->module));
	}
}

void PipelineBuilder::setShaderEffect(ShaderEffect* effect)
{
	_shaderStages.clear();
	effect->fill_stages(_shaderStages);

	_pipelineLayout = effect->builtLayout;
}

void PipelineBuilder::clear_vertex_input()
{
	_vertexInputInfo.pVertexAttributeDescriptions = nullptr;
	_vertexInputInfo.vertexAttributeDescriptionCount = 0;

	_vertexInputInfo.pVertexBindingDescriptions = nullptr;
	_vertexInputInfo.vertexBindingDescriptionCount = 0;
}

VkPipeline ComputePipelineBuilder::build_pipeline(VkDevice device)
{
	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;

	pipelineInfo.stage = _shaderStage;
	pipelineInfo.layout = _pipelineLayout;


	VkPipeline newPipeline;
	if (vkCreateComputePipelines(
		device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		//LOG_FATAL("Failed to build compute pipeline");
		return VK_NULL_HANDLE;
	}
	else
	{
		return newPipeline;
	}
}