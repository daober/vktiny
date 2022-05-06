#include "../include/VulkanRenderer.h"
#include <array>


const unsigned int MAX_FRAMES_IN_FLIGHT = 2;

//defined in VulkanGlobals.h
vkbase::VulkanAllocator* global_allocator;
//global instance for every file
vkbase::VulkanDevice* global_device;


VulkanRenderer::VulkanRenderer( ) { }

VulkanRenderer::~VulkanRenderer( ) {
    std::cout << "destroying vulkan renderer instance" << std::endl;
}


void VulkanRenderer::initVulkan( ) {
	vulkan_surface = new vkbase::VulkanSurface();
	vulkan_surface->initWindow();

	vulkan_instance = new vkbase::VulkanInstance();
	vulkan_instance->createInstance();

	if (enableValidationLayers) {
		setupDebugMessenger();
	}

	submitInfo = vkbase::initializers::submitInfo();
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphores.renderComplete;
}


void VulkanRenderer::createImageViews( ) {

    image_views.resize( vulkan_swapchain->swapchainImages.size( ) );

    for ( size_t i = 0; i < vulkan_swapchain->swapchainImages.size( ); i++ ) {
        VkImageViewCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageInfo.format = vulkan_swapchain->swapchainImageFormat;
        imageInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        imageInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        imageInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageInfo.subresourceRange.baseMipLevel = 0;
        imageInfo.subresourceRange.levelCount = 1;
        imageInfo.subresourceRange.baseArrayLayer = 0;
        imageInfo.subresourceRange.layerCount = 1;

        imageInfo.image = vulkan_swapchain->swapchainImages[i];

        if ( vkCreateImageView( global_device->logicalDevice, &imageInfo, nullptr, &image_views[i] ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create image views!" );
        }
    }

}


void VulkanRenderer::createRenderpass( ) {

	std::array<VkAttachmentDescription, 2> attachments = {};
	// Color attachment
	attachments[0].format = VK_FORMAT_B8G8R8A8_UNORM;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Depth attachment
	attachments[1].format = VK_FORMAT_D32_SFLOAT;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.pResolveAttachments = nullptr;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(global_device->logicalDevice, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}


void VulkanRenderer::createFramebuffers( ) {

    //the framebuffer connects the actual image resources to the render pass
    framebuffers.resize( image_views.size( ) );

    for ( size_t i = 0; i < image_views.size( ); i++ ) {

        std::array<VkImageView, 2> attachments = { image_views[i], depth_image_view };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size( ));
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = vulkan_swapchain->swapchainExtent.width;
        framebufferInfo.height = vulkan_swapchain->swapchainExtent.height;
        framebufferInfo.layers = 1;

        if ( vkCreateFramebuffer(global_device->logicalDevice, &framebufferInfo, nullptr, &framebuffers.at( i ) ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create framebuffer" );
        }

    }

}

void VulkanRenderer::createCommandPool( ) {

    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(global_device->physicalDevice, vulkan_surface->surface );

    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if ( vkCreateCommandPool(global_device->logicalDevice, &commandPoolInfo, nullptr, &commandPool ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to create command pool" );
    }

}


void VulkanRenderer::createCommandBuffers(  ) {

	draw_cmd_buffers.resize( vulkan_swapchain->swapchainImages.size( ) );

	VkCommandBufferAllocateInfo allocInfo = vkbase::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(draw_cmd_buffers.size()));

    if ( vkAllocateCommandBuffers(global_device->logicalDevice, &allocInfo, draw_cmd_buffers.data() ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to allocate command buffers" );
    }

}

void VulkanRenderer::destoryCommandBuffers( ) {
	vkFreeCommandBuffers(global_device->logicalDevice, commandPool, static_cast<uint32_t>(draw_cmd_buffers.size()), draw_cmd_buffers.data());
}


void VulkanRenderer::createSyncObjects( ) {

    in_flight_fences.resize( MAX_FRAMES_IN_FLIGHT );

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


        if ( vkCreateSemaphore(global_device->logicalDevice, &semaphoreInfo, nullptr, &semaphores.presentComplete ) != VK_SUCCESS ||
             vkCreateSemaphore(global_device->logicalDevice, &semaphoreInfo, nullptr, &semaphores.renderComplete ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create semaphores" );
        }
        for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {

            if(  vkCreateFence(global_device->logicalDevice, &fenceInfo, nullptr, &in_flight_fences.at(i) ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to create fence" );
            }
    }

}

void VulkanRenderer::createPipelineCache( ) {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    if ( vkCreatePipelineCache(global_device->logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipeline_cache ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to create pipeline cache" );
    }
}

void VulkanRenderer::recreateSwapchain( ) {

	int width, height;
	//glfwGetFramebufferSize(vulkan_surface->window, &width, &height);
	while (width == 0 || height == 0) {
		//glfwGetFramebufferSize(vulkan_surface->window, &width, &height);
		//glfwWaitEvents();
	}

	vkDeviceWaitIdle(global_device->logicalDevice);

	cleanupSwapchain();
	createImageViews();
	
	//vulkan_swapchain = new vkbase::VulkanSwapchain(global_device->logicalDevice, vulkan_surface->window);
	vulkan_swapchain->createSwapchain(global_device->physicalDevice, vulkan_surface->surface);

	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
	createDepthStencil();
	createRenderpass();
	createPipelineCache();
	createImageViews();
	createFramebuffers();
}

void VulkanRenderer::cleanupSwapchain( ) {

   for ( size_t i = 0; i < framebuffers.size( ); i++ ) {
        vkDestroyFramebuffer(global_device->logicalDevice, framebuffers.at(i), nullptr );
   }

    vkFreeCommandBuffers(global_device->logicalDevice, commandPool, static_cast< uint32_t >(draw_cmd_buffers.size( )), draw_cmd_buffers.data( ) );

    vkDestroyRenderPass(global_device->logicalDevice, render_pass, nullptr );

    vkDestroyImageView(global_device->logicalDevice, depth_image_view, nullptr );

    for ( size_t i = 0; i < image_views.size( ); i++ ) {
        vkDestroyImageView(global_device->logicalDevice, image_views[i], nullptr );
    }

    delete vulkan_swapchain;
}

void VulkanRenderer::prepareFrame( ) {

    VkResult result = vulkan_swapchain->acquireNextImage( semaphores.presentComplete, &current_buffer );
    if ( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized ) {
        framebuffer_resized = false;
        recreateSwapchain( );
		prepare();

        return;
    } else if ( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR ) {
        throw std::runtime_error( "failed to acquire swap chain image" );
    }

}

void VulkanRenderer::submitFrame( ) {

    VkResult res = vulkan_swapchain->queuePresent(global_device->presentQueue, current_buffer, semaphores.renderComplete );
    vkQueueWaitIdle(global_device->presentQueue );
}


void VulkanRenderer::flushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, bool free ) {

    if ( commandBuffer == VK_NULL_HANDLE ) {
        return;
    }

    VkResult result = vkEndCommandBuffer( commandBuffer );

    if ( result == VK_SUCCESS ) {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

		VkFenceCreateInfo fenceInfo = {}; 
		VkFence fence;
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		if (vkCreateFence(global_device->logicalDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create fence");
		}

        vkQueueSubmit( queue, 1, &submitInfo, VK_NULL_HANDLE );
		vkWaitForFences(global_device->logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
		vkDestroyFence(global_device->logicalDevice, fence, nullptr);

        if ( free ) {
            vkFreeCommandBuffers(global_device->logicalDevice, commandPool, 1, &commandBuffer );
        }

    } else {
        std::cerr << "cannot flush command buffer" << std::endl;
    }


}

void VulkanRenderer::buildCommandBuffers( ) { /* NO IMPLEMENTATION */}


void VulkanRenderer::createDepthStencil( ) {

    depth_image.imageInfo = {};

    depth_image.imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image.imageInfo.imageType = VK_IMAGE_TYPE_2D;
    depth_image.imageInfo.extent.width = static_cast<uint32_t>(vulkan_swapchain->swapchainExtent.width);
    depth_image.imageInfo.extent.height = static_cast<uint32_t>(vulkan_swapchain->swapchainExtent.height);
    depth_image.imageInfo.extent.depth = 1;
    depth_image.imageInfo.mipLevels = 1;
    depth_image.imageInfo.arrayLayers = 1;
    depth_image.imageInfo.format = VK_FORMAT_D32_SFLOAT;
    depth_image.imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image.imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_image.imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    depth_image.imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depth_image.imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image.imageInfo.flags = 0;

    depth_image.allocInfo = {};
    depth_image.allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if ( vmaCreateImage(global_allocator->allocator, &depth_image.imageInfo, &depth_image.allocInfo, &depth_image.image, &depth_image.allocation, nullptr ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to create depth image" );
    }

    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = depth_image.image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = VK_FORMAT_D32_SFLOAT;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    if ( vkCreateImageView(global_device->logicalDevice, &imageViewInfo, nullptr, &depth_image_view ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to create depth image view" );
    }

}

VkPipelineShaderStageCreateInfo VulkanRenderer::createShader( std::string filename, VkShaderStageFlagBits stage ) {

    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
    std::vector<char> code = vkbase::Tools::readFile( filename );
    VkShaderModule shader_module = vkbase::Tools::createShaderModule(global_device->logicalDevice, code );
    shaderStage.module = shader_module;
    shaderStage.pName = "main";

    shader_modules.emplace_back( shaderStage.module );
    return shaderStage;
}


void VulkanRenderer::cleanup( ) {

    //delete basic vma allocations
    vmaDestroyImage(global_allocator->allocator, depth_image.image, depth_image.allocation );

    cleanupSwapchain( );

    vkDestroyPipelineCache(global_device->logicalDevice, pipeline_cache, nullptr );

    vkDestroySemaphore(global_device->logicalDevice, semaphores.renderComplete, nullptr );
    vkDestroySemaphore(global_device->logicalDevice, semaphores.presentComplete, nullptr );

	vkDestroyCommandPool(global_device->logicalDevice, commandPool, nullptr);

    for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i ) {
       vkDestroyFence(global_device->logicalDevice, in_flight_fences.at( i ), nullptr );
    }

	delete global_allocator;

	for (int i = 0; i < shader_modules.size(); i++) {
		vkDestroyShaderModule(global_device->logicalDevice, shader_modules.at(i), nullptr);
	}

	delete global_device;

    if ( enableValidationLayers ) {
        DestroyDebugUtilsMessengerEXT( vulkan_instance->instance, debug_messenger, nullptr );
    }

	//glfwDestroyWindow(vulkan_surface->window);
	//glfwTerminate();
	delete vulkan_surface;

	delete vulkan_instance;
}


void VulkanRenderer::prepare() {

	initVulkan();

	vulkan_surface->createSurface(&vulkan_instance->instance);
	global_device = new vkbase::VulkanDevice();
	global_device->pickPhysicalDevice(vulkan_instance->instance, vulkan_surface->surface);
	global_device->createLogicalDevice(vulkan_surface->surface);

	vkGetPhysicalDeviceFeatures(global_device->physicalDevice, &global_device->features);
	vkGetPhysicalDeviceProperties(global_device->physicalDevice, &global_device->properties);
	vkGetPhysicalDeviceMemoryProperties(global_device->physicalDevice, &global_device->memoryProps);

	vk_device_info.create_basic_infos(global_device);

	global_allocator = new vkbase::VulkanAllocator(global_device->physicalDevice, global_device->logicalDevice );

	//vulkan_swapchain = new vkbase::VulkanSwapchain(global_device->logicalDevice, vulkan_surface->window);
	vulkan_swapchain->createSwapchain(global_device->physicalDevice, vulkan_surface->surface);

	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
	createDepthStencil();
	createRenderpass();
	createPipelineCache();
	createImageViews();
	createFramebuffers();
}

void VulkanRenderer::render()
{
}


VkPipelineShaderStageCreateInfo VulkanRenderer::loadShader(std::string fileName, VkShaderStageFlagBits stage) {
	
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;

	shaderStage.module = vkbase::Tools::loadShader(fileName.c_str(), global_device->logicalDevice);
	
	shaderStage.pName = "main"; // todo : make param
	assert(shaderStage.module != VK_NULL_HANDLE);
	shader_modules.emplace_back(shaderStage.module);
	return shaderStage;

}


void VulkanRenderer::setupDebugMessenger( ) {
    if ( !enableValidationLayers ) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if ( CreateDebugUtilsMessengerEXT( vulkan_instance->instance, &createInfo, nullptr, &debug_messenger ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to set up debug messenger!" );
    }
}

/*void framebufferResizeCallback( GLFWwindow* window, int width, int height ) {
    auto app = reinterpret_cast< VulkanRenderer* >(glfwGetWindowUserPointer( window ));
    app->framebuffer_resized = true;
}*/