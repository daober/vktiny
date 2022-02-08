#pragma once

#include "VulkanTools.h"
#include "VulkanDevice.h"
#include "VulkanGlobals.h"


namespace vkbase {

    struct FramebufferAttachment {
        VkImage image;
        //VkDeviceMemory memory;
        VkImageView view;
        VkFormat format;
        VmaAllocation allocation;
    };

    //encapsulates a complete vulkan framebuffer
    struct VulkanFramebuffer {
        private:

        vkbase::VulkanDevice *vulkan_device;

        public:
        uint32_t width, height;
        VkFramebuffer framebuffer;
        VkRenderPass renderPass;
        VkSampler sampler;

        std::vector<vkbase::FramebufferAttachment> attachments;

		VulkanFramebuffer(vkbase::VulkanDevice* vulkan_device){
            this->vulkan_device = vulkan_device;
        }

        uint32_t addAttachment( ) {
			
        }

        ~VulkanFramebuffer(){
            for (auto& attachment : attachments){
                vmaDestroyImage( global_allocator->allocator, attachment.image, attachment.allocation );
                vkDestroyImageView(vulkan_device->logicalDevice, attachment.view, nullptr);
            }

            vkDestroySampler(vulkan_device->logicalDevice, sampler, nullptr);
            vkDestroyRenderPass(vulkan_device->logicalDevice, renderPass, nullptr);
            vkDestroyFramebuffer( vulkan_device->logicalDevice, framebuffer, nullptr);
        }

    };


}