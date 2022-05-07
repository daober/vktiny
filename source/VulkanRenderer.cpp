#include "../include/VulkanRenderer.h"
#include <array>
#if defined(_WIN32)
#include <ShellScalingApi.h>
#endif

#include "../include/keycodes.h"

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
	
	vulkan_swapchain = new vkbase::VulkanSwapchain(global_device->logicalDevice);
	vulkan_swapchain->createSwapchain(global_device->physicalDevice, vulkan_instance->instance, vulkan_surface->surface);

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

#if defined(_WIN32)
// Win32 : Sets up a console window and redirects standard output to it
void VulkanRenderer::setupConsole(std::string title)
{
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    FILE* stream;
    freopen_s(&stream, "CONIN$", "r", stdin);
    freopen_s(&stream, "CONOUT$", "w+", stdout);
    freopen_s(&stream, "CONOUT$", "w+", stderr);
    SetConsoleTitle(TEXT(title.c_str()));
}

void VulkanRenderer::setupDPIAwareness()
{
    typedef HRESULT* (__stdcall* SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

    HMODULE shCore = LoadLibraryA("Shcore.dll");
    if (shCore)
    {
        SetProcessDpiAwarenessFunc setProcessDpiAwareness =
            (SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

        if (setProcessDpiAwareness != nullptr)
        {
            setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        }

        FreeLibrary(shCore);
    }
}


HWND VulkanRenderer::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)
{
	this->windowInstance = hinstance;

	WNDCLASSEX wndClass;
	std::string name = "vulkan";

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndproc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hinstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = name.c_str();
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClass))
	{
		std::cout << "Could not register window class!\n";
		fflush(stdout);
		exit(1);
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	
	bool fullscreen = false;
	if (fullscreen)
	{
		if ((width != (uint32_t)screenWidth) && (height != (uint32_t)screenHeight))
		{
			DEVMODE dmScreenSettings;
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
			dmScreenSettings.dmSize = sizeof(dmScreenSettings);
			dmScreenSettings.dmPelsWidth = width;
			dmScreenSettings.dmPelsHeight = height;
			dmScreenSettings.dmBitsPerPel = 32;
			dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{
					//settings.fullscreen = false;
				}
				else
				{
					return nullptr;
				}
			}
			screenWidth = width;
			screenHeight = height;
		}

	}

	DWORD dwExStyle;
	DWORD dwStyle;

	if (fullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}

	RECT windowRect;
	windowRect.left = 0L;
	windowRect.top = 0L;
	windowRect.right = fullscreen ? (long)screenWidth : (long)width;
	windowRect.bottom = fullscreen ? (long)screenHeight : (long)height;

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	std::string windowTitle = "Generic";
	window = CreateWindowEx(0,
		name.c_str(),
		windowTitle.c_str(),
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hinstance,
		NULL);

	if (!fullscreen)
	{
		// Center on screen
		uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
		uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
		SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	if (!window)
	{
		printf("Could not create window!\n");
		fflush(stdout);
		return nullptr;
	}

	ShowWindow(window, SW_SHOW);
	SetForegroundWindow(window);
	SetFocus(window);

	return window;
}

void VulkanRenderer::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		prepared = false;
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		ValidateRect(window, NULL);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case KEY_P:
			paused = !paused;
			break;
		case KEY_F1:
			//if (settings.overlay) {
			//	UIOverlay.visible = !UIOverlay.visible;
			//}
			break;
		case KEY_ESCAPE:
			PostQuitMessage(0);
			break;
		}

		/*if (camera.type == Camera::firstperson)
		{
			switch (wParam)
			{
			case KEY_W:
				camera.keys.up = true;
				break;
			case KEY_S:
				camera.keys.down = true;
				break;
			case KEY_A:
				camera.keys.left = true;
				break;
			case KEY_D:
				camera.keys.right = true;
				break;
			}
		}*/

		//keyPressed((uint32_t)wParam);
		break;
	case WM_KEYUP:
		/*if (camera.type == Camera::firstperson)
		{
			switch (wParam)
			{
			case KEY_W:
				camera.keys.up = false;
				break;
			case KEY_S:
				camera.keys.down = false;
				break;
			case KEY_A:
				camera.keys.left = false;
				break;
			case KEY_D:
				camera.keys.right = false;
				break;
			}
		}*/
		break;
	case WM_LBUTTONDOWN:
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		//mouseButtons.left = true;
		break;
	case WM_RBUTTONDOWN:
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		//mouseButtons.right = true;
		break;
	case WM_MBUTTONDOWN:
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		//mouseButtons.middle = true;
		break;
	case WM_LBUTTONUP:
		//mouseButtons.left = false;
		break;
	case WM_RBUTTONUP:
		//mouseButtons.right = false;
		break;
	case WM_MBUTTONUP:
		//mouseButtons.middle = false;
		break;
	case WM_MOUSEWHEEL:
	{
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		//camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f));
		//viewUpdated = true;
		break;
	}
	case WM_MOUSEMOVE:
	{
		//handleMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_SIZE:
		if ((prepared) && (wParam != SIZE_MINIMIZED))
		{
			/*if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
			{
				destWidth = LOWORD(lParam);
				destHeight = HIWORD(lParam);
				windowResize();
			}*/
		}
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
		minMaxInfo->ptMinTrackSize.x = 64;
		minMaxInfo->ptMinTrackSize.y = 64;
		break;
	}
	case WM_ENTERSIZEMOVE:
		//resizing = true;
		break;
	case WM_EXITSIZEMOVE:
		//resizing = false;
		break;
	}
}

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
int32_t VulkanRenderer::handleAppInput(android_app* app, AInputEvent* event)
{
	VulkanExampleBase* vulkanExample = reinterpret_cast<VulkanExampleBase*>(app->userData);
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	{
		int32_t eventSource = AInputEvent_getSource(event);
		switch (eventSource) {
		case AINPUT_SOURCE_JOYSTICK: {
			// Left thumbstick
			vulkanExample->gamePadState.axisLeft.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);
			vulkanExample->gamePadState.axisLeft.y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0);
			// Right thumbstick
			vulkanExample->gamePadState.axisRight.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0);
			vulkanExample->gamePadState.axisRight.y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0);
			break;
			}

		case AINPUT_SOURCE_TOUCHSCREEN: {
			int32_t action = AMotionEvent_getAction(event);

			switch (action) {
			case AMOTION_EVENT_ACTION_UP: {
				vulkanExample->lastTapTime = AMotionEvent_getEventTime(event);
				vulkanExample->touchPos.x = AMotionEvent_getX(event, 0);
				vulkanExample->touchPos.y = AMotionEvent_getY(event, 0);
				vulkanExample->touchTimer = 0.0;
				vulkanExample->touchDown = false;
				vulkanExample->camera.keys.up = false;

				// Detect single tap
				int64_t eventTime = AMotionEvent_getEventTime(event);
				int64_t downTime = AMotionEvent_getDownTime(event);
				if (eventTime - downTime <= vks::android::TAP_TIMEOUT) {
					float deadZone = (160.f / vks::android::screenDensity) * vks::android::TAP_SLOP * vks::android::TAP_SLOP;
					float x = AMotionEvent_getX(event, 0) - vulkanExample->touchPos.x;
					float y = AMotionEvent_getY(event, 0) - vulkanExample->touchPos.y;
					if ((x * x + y * y) < deadZone) {
						vulkanExample->mouseButtons.left = true;
					}
				};

				return 1;
				break;
			}
			case AMOTION_EVENT_ACTION_DOWN: {
				// Detect double tap
				int64_t eventTime = AMotionEvent_getEventTime(event);
				if (eventTime - vulkanExample->lastTapTime <= vks::android::DOUBLE_TAP_TIMEOUT) {
					float deadZone = (160.f / vks::android::screenDensity) * vks::android::DOUBLE_TAP_SLOP * vks::android::DOUBLE_TAP_SLOP;
					float x = AMotionEvent_getX(event, 0) - vulkanExample->touchPos.x;
					float y = AMotionEvent_getY(event, 0) - vulkanExample->touchPos.y;
					if ((x * x + y * y) < deadZone) {
						vulkanExample->keyPressed(TOUCH_DOUBLE_TAP);
						vulkanExample->touchDown = false;
					}
				}
				else {
					vulkanExample->touchDown = true;
				}
				vulkanExample->touchPos.x = AMotionEvent_getX(event, 0);
				vulkanExample->touchPos.y = AMotionEvent_getY(event, 0);
				vulkanExample->mousePos.x = AMotionEvent_getX(event, 0);
				vulkanExample->mousePos.y = AMotionEvent_getY(event, 0);
				break;
			}
			case AMOTION_EVENT_ACTION_MOVE: {
				bool handled = false;
				if (vulkanExample->settings.overlay) {
					ImGuiIO& io = ImGui::GetIO();
					handled = io.WantCaptureMouse;
				}
				if (!handled) {
					int32_t eventX = AMotionEvent_getX(event, 0);
					int32_t eventY = AMotionEvent_getY(event, 0);

					float deltaX = (float)(vulkanExample->touchPos.y - eventY) * vulkanExample->camera.rotationSpeed * 0.5f;
					float deltaY = (float)(vulkanExample->touchPos.x - eventX) * vulkanExample->camera.rotationSpeed * 0.5f;

					vulkanExample->camera.rotate(glm::vec3(deltaX, 0.0f, 0.0f));
					vulkanExample->camera.rotate(glm::vec3(0.0f, -deltaY, 0.0f));

					vulkanExample->viewChanged();

					vulkanExample->touchPos.x = eventX;
					vulkanExample->touchPos.y = eventY;
				}
				break;
			}
			default:
				return 1;
				break;
			}
		}

									  return 1;
		}
}

void VulkanRenderer::handleAppCommand(android_app* app, int32_t cmd)
{
	assert(app->userData != NULL);
	VulkanExampleBase* vulkanExample = reinterpret_cast<VulkanExampleBase*>(app->userData);
	switch (cmd)
	{
	case APP_CMD_SAVE_STATE:
		LOGD("APP_CMD_SAVE_STATE");
		/*
		vulkanExample->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)vulkanExample->app->savedState) = vulkanExample->state;
		vulkanExample->app->savedStateSize = sizeof(struct saved_state);
		*/
		break;
	case APP_CMD_INIT_WINDOW:
		LOGD("APP_CMD_INIT_WINDOW");
		if (androidApp->window != NULL)
		{
			if (vulkanExample->initVulkan()) {
				vulkanExample->prepare();
				assert(vulkanExample->prepared);
			}
			else {
				LOGE("Could not initialize Vulkan, exiting!");
				androidApp->destroyRequested = 1;
			}
		}
		else
		{
			LOGE("No window assigned!");
		}
		break;
	case APP_CMD_LOST_FOCUS:
		LOGD("APP_CMD_LOST_FOCUS");
		vulkanExample->focused = false;
		break;
	case APP_CMD_GAINED_FOCUS:
		LOGD("APP_CMD_GAINED_FOCUS");
		vulkanExample->focused = true;
		break;
	case APP_CMD_TERM_WINDOW:
		// Window is hidden or closed, clean up resources
		LOGD("APP_CMD_TERM_WINDOW");
		if (vulkanExample->prepared) {
			vulkanExample->swapChain.cleanup();
		}
		break;
	}
}
#endif

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

#if defined(_WIN32)
    setupConsole("hello");
    setupDPIAwareness();
    setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
    handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    static int32_t handleAppInput(struct android_app* app, AInputEvent * event);
    static void handleAppCommand(android_app * app, int32_t cmd);
#endif

	global_device = new vkbase::VulkanDevice();
	global_device->pickPhysicalDevice(vulkan_instance->instance);
    global_device->createLogicalDevice();

	global_allocator = new vkbase::VulkanAllocator(global_device->physicalDevice, global_device->logicalDevice );
	vulkan_swapchain = new vkbase::VulkanSwapchain(global_device->logicalDevice);

    vulkan_surface = new vkbase::VulkanSurface(vulkan_instance->instance);
#if defined(_WIN32)
    vulkan_surface->initSurface(windowInstance, window);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    swapChain.initSurface(androidApp->window);
    vulkan_surface->initSurface(androidApp->window);
#endif

	createCommandPool();
    vulkan_swapchain->createSwapchain(global_device->physicalDevice, vulkan_instance->instance, vulkan_surface->surface);
    //create swapchain
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