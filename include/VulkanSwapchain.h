#pragma once


#include "VulkanTools.h"


namespace vkbase {

    struct VulkanSwapchain {

        VkSwapchainKHR swapchain;
        VkDevice device;
        std::vector<VkImage> swapchainImages;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
		uint32_t imgui_image_count;

        VulkanSwapchain(VkDevice dev) {
            device = dev;
        }


        ~VulkanSwapchain( ) {
            vkDestroySwapchainKHR( device, swapchain, nullptr );
        }


        void createSwapchain( VkPhysicalDevice physicalDevice, VkInstance inst, VkSurfaceKHR surface ) {

            SwapChainSupportDetails swapChainSupport = querySwapChainSupport( physicalDevice, surface );

            VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
            VkPresentModeKHR presentMode = chooseSwapPresentMode( swapChainSupport.presentModes );
            VkExtent2D extent = chooseSwapExtent( swapChainSupport.surfaceCapabilites );

            uint32_t imageCount = swapChainSupport.surfaceCapabilites.minImageCount + 1;
			imgui_image_count = imageCount;

            if ( swapChainSupport.surfaceCapabilites.maxImageCount > 0 && imageCount > swapChainSupport.surfaceCapabilites.maxImageCount ) {
                imageCount = swapChainSupport.surfaceCapabilites.maxImageCount;
            }

            VkSwapchainCreateInfoKHR swapInfo = {};
            swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapInfo.clipped = VK_FALSE;
            swapInfo.surface = surface;

            swapInfo.minImageCount = imageCount;
            swapInfo.imageFormat = surfaceFormat.format;
            swapInfo.imageColorSpace = surfaceFormat.colorSpace;
            swapInfo.imageExtent = extent;
            swapInfo.imageArrayLayers = 1;
            swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies( physicalDevice, surface );
            uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

            if ( indices.graphicsFamily != indices.presentFamily ) {
                swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapInfo.queueFamilyIndexCount = 2;
                swapInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                swapInfo.queueFamilyIndexCount = 0; // Optional
                swapInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            swapInfo.preTransform = swapChainSupport.surfaceCapabilites.currentTransform;
            swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapInfo.presentMode = presentMode;
            swapInfo.clipped = VK_TRUE;
            swapInfo.oldSwapchain = VK_NULL_HANDLE;


            if ( vkCreateSwapchainKHR( device, &swapInfo, nullptr, &swapchain ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to create swapchain" );
            }

            //acquire images from swapchain
            vkGetSwapchainImagesKHR( device, swapchain, &imageCount, nullptr );
            swapchainImages.resize( imageCount );
            vkGetSwapchainImagesKHR( device, swapchain, &imageCount, swapchainImages.data( ) );

            swapchainImageFormat = surfaceFormat.format;
            swapchainExtent = extent;

        }

        VkResult acquireNextImage( VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex ) {
            VkResult result = vkAcquireNextImageKHR( device, swapchain, UINT64_MAX, presentCompleteSemaphore, (VkFence) nullptr, imageIndex );


            return result;
        }


        VkResult queuePresent( VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE ) {

            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchain;
            presentInfo.pImageIndices = &imageIndex;

            if ( waitSemaphore != VK_NULL_HANDLE ) {
                presentInfo.pWaitSemaphores = &waitSemaphore;
                presentInfo.waitSemaphoreCount = 1;
            }
            return vkQueuePresentKHR( queue, &presentInfo );
        }

    private:

        VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats ) {

            if ( availableFormats.size( ) == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED ) {
                return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
            }

            for ( const VkSurfaceFormatKHR& format : availableFormats ) {
                if ( format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
                    return format;
                }
            }
            return availableFormats[0];
        }

        VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) {

            for ( const auto& availablePresentMode : availablePresentModes ) {
                if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
                    return availablePresentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }


        VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR & capabilities ) {

            if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max( ) ) {
                return capabilities.currentExtent;
            } else {
                int width, height;

                //glfwGetFramebufferSize( window, &width, &height );

                VkExtent2D actualExtent = { WIDTH, HEIGHT };

                actualExtent.width = static_cast< uint32_t >(width);
                actualExtent.height = static_cast< uint32_t >(height);

                return actualExtent;
            }

        }

    };
}