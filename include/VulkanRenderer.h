#pragma once

#include "../include/VulkanInstance.h"
#include "../include/VulkanSurface.h"
#include "../include/VulkanSwapchain.h"
#include "../include/VulkanDevice.h"
#include "../include/VulkanAllocator.h"
#include "../include/VulkanFramebuffer.h"


class VulkanRenderer {

public:

    VulkanRenderer( );
    virtual ~VulkanRenderer( );

	void prepareFrame();
	void submitFrame();

public:
    void initVulkan( );
    virtual void cleanup( );
    virtual void renderLoop( ) = 0;
	virtual void prepare();
	virtual void render() = 0;

	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

private:
    void createImageViews( );
    void createRenderpass( );

    void createFramebuffers( );
    void createCommandPool( );
    void createCommandBuffers( );
    void destoryCommandBuffers( );
    void createSyncObjects( );
    void createPipelineCache( );
    void recreateSwapchain( );
    void cleanupSwapchain( );

    void createDepthStencil( );



private:

    //end the command buffer, submit it to the queue and free it
    virtual void flushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, bool free );
    virtual void buildCommandBuffers( );

    VkPipelineShaderStageCreateInfo createShader(std::string filename, VkShaderStageFlagBits stage );

    VkSurfaceCapabilitiesKHR surfaceCapabilites;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    void setupDebugMessenger( );

	virtual void preparePipelines() = 0;


protected:

	struct {
		// Swap chain image presentation
		VkSemaphore presentComplete;
		// Command buffer submission and execution
		VkSemaphore renderComplete;
	} semaphores;

	// Contains command buffers and semaphores to be presented to the queue
	VkSubmitInfo submitInfo;

	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    vkbase::VulkanInstance* vulkan_instance;
    vkbase::VulkanSurface* vulkan_surface;

    vkbase::VulkanSwapchain* vulkan_swapchain;

    vkbase::Image depth_image;
    VkImageView depth_image_view;
    VkRenderPass render_pass;
    VkPipelineCache pipeline_cache;

    std::vector<VkImageView> image_views;
    std::vector<VkFramebuffer> framebuffers;

    VkCommandPool commandPool;

	// Command buffers used for rendering
    std::vector<VkCommandBuffer> draw_cmd_buffers;

    std::vector<VkFence> in_flight_fences;

    // List of shader modules created (stored for cleanup)
    std::vector<VkShaderModule> shader_modules;

    VkDebugUtilsMessengerEXT debug_messenger;

    uint32_t current_buffer = 0;

	vkbase::vkDeviceInfo::Infos vk_device_info;

    VkPipelineLayout pipelineLayout;

    struct DescriptorSetLayouts {
        VkDescriptorSetLayout matrices;
        VkDescriptorSetLayout textures;
    } descriptorSetLayouts;


    //VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    


public:
    bool framebuffer_resized = false;
};