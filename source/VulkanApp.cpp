#include "../include/VulkanApp.h"

#include <glm/gtc/matrix_inverse.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>


#define VERTEX_BUFFER_BIND_ID 0

bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;

//define external from globals (move it to vulkan initializers?)
Camera* global_camera;

float rotation_amount_x = 273.0f;
float rotation_amount_y = 325.0f;
float rotation_amount_z = 258.0f;

bool wireframe_mode = false;

vkbase::Buffer* global_uniform_buffer;

VmaAllocationInfo uniform_allocation_info = {};

uboVS transform_matrices;



float degreesToRadians(float angleDegrees) {
	return (angleDegrees * PI / 180.0);
}



VulkanApp::VulkanApp( ) : VulkanRenderer() {
	global_camera = new Camera();
}

VulkanApp::~VulkanApp( ) {

#ifndef NDEBUG
	//only stats printing if in debug mode
	global_allocator->printStats();
#endif

    std::cout << "destroying vulkan app instance" << std::endl;

	if (model != nullptr) {
		delete model;
		model = nullptr;
	}


	vmaDestroyBuffer(global_allocator->allocator, global_uniform_buffer->buffer, global_uniform_buffer->allocation);

	vkDestroyDescriptorPool(global_device->logicalDevice, descriptorPool, nullptr);

	ImGui_ImplVulkan_Shutdown();

	vkDestroyPipelineLayout(global_device->logicalDevice, pipelineLayout, nullptr);

	vkDestroyPipeline(global_device->logicalDevice, pipeline, nullptr);

	vkDestroyDescriptorSetLayout(global_device->logicalDevice, descriptorSetLayouts.matrices, nullptr);
	vkDestroyDescriptorSetLayout(global_device->logicalDevice, descriptorSetLayouts.textures, nullptr);

	cleanup();
}


void VulkanApp::renderLoop() {


	transform_matrices.projection = glm::perspective(glm::radians(60.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 256.0f);
	transform_matrices.projection[1][1] *= -1.0f;

	//while (!glfwWindowShouldClose(vulkan_surface->window)) {
	//	float currentFrame = glfwGetTime();
	//	deltaTime = currentFrame - lastFrame;
	//	lastFrame = currentFrame;

	//	glfwPollEvents();
	//	processInput(vulkan_surface->window);

		render();

		//transform_matrices.view *= glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
		transform_matrices.view = global_camera->GetViewMatrix();
		transform_matrices.viewPos = global_camera->Position;

		transform_matrices.model = glm::mat4(1.0f);
		transform_matrices.model = glm::translate(transform_matrices.model, glm::vec3(0.0f, 0.0f, 0.0f));
		//transform_matrices.model = glm::rotate(transform_matrices.model, degreesToRadians(180.0f), glm::vec3(0.0, 0.0f, 1.0f));
		transform_matrices.model = glm::scale(transform_matrices.model, glm::vec3(0.01f, 0.01f, 0.01f));

		memcpy(uniform_allocation_info.pMappedData, &transform_matrices, sizeof(uboVS));

		//bool show_demo_window = true;
		//bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		//renderUIFrame();
		//createApplicationInfoWindow();

		//ImGui::Render();

		buildCommandBuffers();
	//}
	vkDeviceWaitIdle(global_device->logicalDevice);
}


/*void VulkanApp::createControlInfoWindow() {

	ImGui::Begin("Help", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | 
								  ImGuiWindowFlags_NoBackground |ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | 
								  ImGuiWindowFlags_NoScrollbar  | ImGuiInputTextFlags_NoHorizontalScroll );

	ImGui::SetWindowSize(ImVec2(450.0f, 20.0f));
	ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
	ImVec4 text_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	ImGui::SetWindowFontScale(1.0f);
	ImGui::TextColored(text_color, "INFO: use 'LEFT CTRL' to manipulate the camera view");

	ImGui::End();
}*/


void VulkanApp::createDescriptorSetLayout() {

	//descriptor set for passing matrices
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = 0;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { layoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(global_device->logicalDevice, &layoutInfo, nullptr, &descriptorSetLayouts.matrices) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout");
	}


	//descriptor set layout for passing material textures
	//color
	VkDescriptorSetLayoutBinding samplerBinding = {};
	samplerBinding.binding = 0;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.descriptorCount = 1;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//normal
	VkDescriptorSetLayoutBinding normalBinding = {};
	normalBinding.binding = 1;
	normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalBinding.descriptorCount = 1;
	normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> materialBinding = { samplerBinding, normalBinding };

	layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = materialBinding.size();
	layoutInfo.pBindings = materialBinding.data();
	
	
	if (vkCreateDescriptorSetLayout(global_device->logicalDevice, &layoutInfo, nullptr, &descriptorSetLayouts.textures) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout");
	}

}

void VulkanApp::createDescriptorSets() {

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayouts.matrices;

	if (int result = vkAllocateDescriptorSets(global_device->logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		std::cout << result << std::endl;
		throw std::runtime_error("failed to allocate descriptor sets");
	}

	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = global_uniform_buffer->buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(uboVS);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(global_device->logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	{
		for (auto& material : model->materials) {

			VkDescriptorSetAllocateInfo allocMaterialInfo = {};
			allocMaterialInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocMaterialInfo.descriptorPool = descriptorPool;
			allocMaterialInfo.pSetLayouts = &descriptorSetLayouts.textures;
			allocMaterialInfo.descriptorSetCount = 1;
			if (int result = vkAllocateDescriptorSets(global_device->logicalDevice, &allocMaterialInfo, &material.descriptorSet) != VK_SUCCESS) {
				std::cout << result << std::endl;
				throw std::runtime_error("failed to allocate descriptor sets");
			}

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

			VkDescriptorImageInfo colorInfo = model->getTextureDescriptor(material.baseColorTextureIndex);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = material.descriptorSet;
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[0].pImageInfo = &colorInfo;

			VkDescriptorImageInfo normalInfo = model->getTextureDescriptor(material.normalTextureIndex);

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = material.descriptorSet;
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].pImageInfo = &normalInfo;

			vkUpdateDescriptorSets(global_device->logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

}

/*void VulkanApp::createApplicationInfoWindow() {

		ImGui::Begin("Device Details");

		ImGui::SetWindowSize(ImVec2(300.0f, 200.0f));
		ImGui::SetWindowPos(ImVec2(20.0f, 20.0f));

		ImGui::TextWrapped(vk_device_info.device_string.c_str());

		if ( ImGui::CollapsingHeader("GPU details")) {
			ImGui::TextWrapped(vk_device_info.vendor.c_str());
			ImGui::TextWrapped(vk_device_info.device_type.c_str());
			ImGui::TextWrapped(vk_device_info.driver_version.c_str());
			ImGui::TextWrapped(vk_device_info.api_version.c_str());
			ImGui::TextWrapped(vk_device_info.memory_heap_size.c_str());
			
			if (ImGui::CollapsingHeader("Device Limits")) {
				ImGui::TextWrapped(vk_device_info.max_allocation_count.c_str());
			}

			if (ImGui::CollapsingHeader("GPU Shader Infos")) {
				ImGui::TextWrapped(vk_device_info.tesselation_shader.c_str());
				ImGui::TextWrapped(vk_device_info.geometry_shader.c_str());
				ImGui::TextWrapped(vk_device_info.sparse_binding.c_str());
			}
		}

		ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
}*/


void VulkanApp::render() {
	draw();
}


void VulkanApp::loadAssets() {
	//load model
	global_uniform_buffer = new vkbase::Buffer(global_allocator->allocator);
	global_allocator->createBuffer(global_uniform_buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &uniform_allocation_info, sizeof(uboVS));


	model = new GLTFKtxModel("../assets/sponza/sponza.gltf", global_device->graphicsQueue);
	//model = new GLTFPngModel("../assets/bistro_exterior/bistro_exterior.gltf", global_device->graphicsQueue);
	//model = new GLTFPngModel("../assets/bistro_interior/bistro_interior.gltf", global_device->graphicsQueue);
}


void VulkanApp::preparePipelines() {
	
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vkbase::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState = vkbase::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = vkbase::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = vkbase::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = vkbase::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = vkbase::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = vkbase::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicState = vkbase::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkVertexInputBindingDescription vertexInputBinding = vkbase::initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);

	// Attribute descriptions
	// Describes memory layout and shader positions
	// Vertex
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
		vkbase::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),
		vkbase::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),
		vkbase::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)),
		vkbase::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),
		vkbase::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)),
	};

	//mesh rendering pipeline
	shaderStages[0] = loadShader("../shaders/mesh_basic.vspv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("../shaders/mesh_basic.fspv", VK_SHADER_STAGE_FRAGMENT_BIT);
	//shaderStages[0] = loadShader("../shaders/tbn.vspv", VK_SHADER_STAGE_VERTEX_BIT);
	//shaderStages[1] = loadShader("../shaders/tbn.fspv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = vkbase::initializers::pipelineCreateInfo(pipelineLayout, render_pass, 0);
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();

	VkPipelineVertexInputStateCreateInfo vertexInputState = vkbase::initializers::pipelineVertexInputStateCreateInfo();
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

	pipelineCreateInfo.pVertexInputState = &vertexInputState;

	//we need a pipeline for each material
	for (auto& material : model->materials) {

		// For double sided materials, culling will be disabled
		rasterizationState.cullMode = material.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;

		if (vkCreateGraphicsPipelines(global_device->logicalDevice, pipeline_cache, 1, &pipelineCreateInfo, nullptr, &material.pipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline");
		}
	}


}


void VulkanApp::buildCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBufferInfo = vkbase::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = VkClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vkbase::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = render_pass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = vulkan_swapchain->swapchainExtent.width;
	renderPassBeginInfo.renderArea.extent.height = vulkan_swapchain->swapchainExtent.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	VkViewport viewport = vkbase::initializers::viewport((float)WIDTH, (float)HEIGHT, 0.0f, 1.0f);
	VkRect2D scissor = vkbase::initializers::rect2D(WIDTH, HEIGHT, 0, 0);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i) {
		renderPassBeginInfo.framebuffer = framebuffers[i];

		vkBeginCommandBuffer(draw_cmd_buffers[i], &cmdBufferInfo);
		
		vkCmdBeginRenderPass(draw_cmd_buffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			model->bind(draw_cmd_buffers[i], pipelineLayout);

			// Record Imgui Draw Data and draw funcs into command buffer
			//renderUIDrawData(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		if (vkEndCommandBuffer(draw_cmd_buffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to end command buffer!");
		}
	}
}


void VulkanApp::prepare() {
	VulkanRenderer::prepare();

	//initUI();

	loadAssets();

	createDescriptorPool();
	createDescriptorSetLayout();
	createDescriptorSets();
	createPipelineLayout();

	preparePipelines();
	//prepareUI();
	//renderUIFrame();

	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//createApplicationInfoWindow();
	//ImGui::Render();

	renderLoop();
}

void VulkanApp::createPipelineLayout() {
	std::array<VkDescriptorSetLayout, 2> setLayouts = { descriptorSetLayouts.matrices, descriptorSetLayouts.textures };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = setLayouts.size();
	pipelineLayoutInfo.pSetLayouts = setLayouts.data();

	if (vkCreatePipelineLayout(global_device->logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}


void VulkanApp::createDescriptorPool() {
	std::vector<VkDescriptorPoolSize> poolSizes = {};
	
	VkDescriptorPoolSize poolSize;
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = vulkan_swapchain->swapchainImages.size();
	poolSizes.push_back(poolSize);

	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = vulkan_swapchain->swapchainImages.size() * model->materials.size() * 2;
	poolSizes.push_back(poolSize);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.maxSets = vulkan_swapchain->swapchainImages.size() * model->materials.size() + 1;
	poolInfo.pPoolSizes = poolSizes.data();

	if (vkCreateDescriptorPool(global_device->logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool");
	}
}


void VulkanApp::draw() {
	VulkanRenderer::prepareFrame();

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &draw_cmd_buffers[current_buffer];

	vkQueueSubmit(global_device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	VulkanRenderer::submitFrame();
}


/*void VulkanApp::renderUIFrame() {
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}*/


/*void VulkanApp::initUI() {
	//setup dear imgui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForVulkan(vulkan_surface->window, true);
}*/


/*void VulkanApp::prepareUI() {

	std::vector<VkDescriptorPoolSize> poolSizes = {
	vkbase::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
	vkbase::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo = vkbase::initializers::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 2);

	if (vkCreateDescriptorPool(global_device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pools");
	}

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vulkan_instance->instance;
	init_info.PhysicalDevice = global_device->physicalDevice;
	init_info.Device = global_device->logicalDevice;
	init_info.QueueFamily = findQueueFamilies(global_device->physicalDevice, vulkan_surface->surface).graphicsFamily;
	init_info.Queue = global_device->graphicsQueue;
	init_info.PipelineCache = pipeline_cache;
	init_info.DescriptorPool = descriptorPool;
	init_info.MinImageCount = vulkan_swapchain->imgui_image_count;
	init_info.ImageCount = vulkan_swapchain->imgui_image_count;
	ImGui_ImplVulkan_Init(&init_info, render_pass);

	// Upload Fonts
	{
		// Use any command queue
		VkCommandPool ui_command_pool = commandPool;
		VkCommandBuffer ui_command_buffer = draw_cmd_buffers[current_buffer];

		vkResetCommandPool(global_device->logicalDevice, ui_command_pool, 0);
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(ui_command_buffer, &begin_info);

		ImGui_ImplVulkan_CreateFontsTexture(ui_command_buffer);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &ui_command_buffer;
		vkEndCommandBuffer(ui_command_buffer);
		vkQueueSubmit(global_device->graphicsQueue, 1, &end_info, VK_NULL_HANDLE);
		vkDeviceWaitIdle(global_device->logicalDevice);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

}*/


/*void VulkanApp::renderUIDrawData(VkCommandBuffer draw_cmd_buffer) {
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), draw_cmd_buffer);
}*/

/*void VulkanApp::processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		global_camera->ProcessKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		global_camera->ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		global_camera->ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		global_camera->ProcessKeyboard(RIGHT, deltaTime);
	}
		
}*/


void mouse_pos_callback(GLFWwindow* window, double xpos, double ypos) {

		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		//float yoffset = ypos - lastY;
		float yoffset = lastY - ypos;

		lastX = xpos;
		lastY = ypos;

		global_camera->ProcessMouseMovement(xoffset, yoffset);
}