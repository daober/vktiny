#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "VulkanInitializers.h"

#include <fstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <set>
#include <iostream>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#define DEFAULT_FENCE_TIMEOUT 100000000000
#define PI 3.14


#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


const int WIDTH = 1680;
const int HEIGHT = 1050;



struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR surfaceCapabilites;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


const std::vector<const char*> deviceExtensions = {
    "VK_KHR_swapchain"
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};


inline VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger ) {
    auto func = ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr ) {
        return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

inline void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator ) {
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr ) {
        func( instance, debugMessenger, pAllocator );
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData ) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}



inline bool checkValidationLayerSupport( ) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data( ) );

    for ( const char* layerName : validationLayers ) {
        bool layerFound = false;

        for ( VkLayerProperties layer : availableLayers ) {
            if ( strcmp( layer.layerName, layerName ) == 0 ) {
                layerFound = true;
                break;
            }
        }
        if ( !layerFound ) {
            return false;
        }
    }
    return true;
}


inline SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface ) {

    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.surfaceCapabilites );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );

    if ( formatCount != 0 ) {
        details.formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data( ) );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );

    if ( presentModeCount != 0 ) {
        details.presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data( ) );
    }

    return details;
}


struct QueueFamilyIndices {
    uint32_t graphicsFamily = -1;
    uint32_t presentFamily = -1;

    bool isComplete( ) {
        if ( graphicsFamily >= 0 && presentFamily >= 0 ) {
            return true;
        } else {
return false;
		}
	}
};

inline QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {

	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}




namespace vkbase {


	namespace Tools {

		static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
			VkShaderModule shaderModule;

			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
			moduleInfo.codeSize = code.size();

			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module");
			}

			return shaderModule;
		}

		static std::vector<char> readFile(const std::string& filename) {
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				throw std::runtime_error("failed to open shader file!");
			}

			size_t fileSize = (size_t)file.tellg();
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);

			file.close();

			return buffer;
		}


		inline VkShaderModule loadShader(const char* fileName, VkDevice device)
		{
			std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

			if (is.is_open())
			{
				size_t size = is.tellg();
				is.seekg(0, std::ios::beg);
				char* shaderCode = new char[size];
				is.read(shaderCode, size);
				is.close();

				assert(size > 0);

				VkShaderModule shaderModule;
				VkShaderModuleCreateInfo moduleCreateInfo{};
				moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				moduleCreateInfo.codeSize = size;
				moduleCreateInfo.pCode = (uint32_t*)shaderCode;

				if (vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule) != VK_SUCCESS){
					std::cerr << "failed to create shader module" << std::endl;
				}

				 delete[] shaderCode;

				 return shaderModule;
			 }
			 else
			 {
				 std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << std::endl;
				 return VK_NULL_HANDLE;
			 }
		 }


		  inline void setImageLayout(
			 VkCommandBuffer cmdbuffer,
			 VkImage image,
			 VkImageLayout oldImageLayout,
			 VkImageLayout newImageLayout,
			 VkImageSubresourceRange subresourceRange,
			 VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			 VkPipelineStageFlags dstStageMask= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
		 {
			 // Create an image barrier object
			 VkImageMemoryBarrier imageMemoryBarrier = vkbase::initializers::imageMemoryBarrier();
			 imageMemoryBarrier.oldLayout = oldImageLayout;
			 imageMemoryBarrier.newLayout = newImageLayout;
			 imageMemoryBarrier.image = image;
			 imageMemoryBarrier.subresourceRange = subresourceRange;

			 // Source layouts (old)
			 // Source access mask controls actions that have to be finished on the old layout
			 // before it will be transitioned to the new layout
			 switch (oldImageLayout)
			 {
			 case VK_IMAGE_LAYOUT_UNDEFINED:
				 // Image layout is undefined (or does not matter)
				 // Only valid as initial layout
				 // No flags required, listed only for completeness
				 imageMemoryBarrier.srcAccessMask = 0;
				 break;

			 case VK_IMAGE_LAYOUT_PREINITIALIZED:
				 // Image is preinitialized
				 // Only valid as initial layout for linear images, preserves memory contents
				 // Make sure host writes have been finished
				 imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				 // Image is a color attachment
				 // Make sure any writes to the color buffer have been finished
				 imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				 // Image is a depth/stencil attachment
				 // Make sure any writes to the depth/stencil buffer have been finished
				 imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				 // Image is a transfer source 
				 // Make sure any reads from the image have been finished
				 imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				 // Image is a transfer destination
				 // Make sure any writes to the image have been finished
				 imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				 // Image is read by a shader
				 // Make sure any shader reads from the image have been finished
				 imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				 break;
			 default:
				 // Other source layouts aren't handled (yet)
				 break;
			 }

			 // Target layouts (new)
			 // Destination access mask controls the dependency for the new image layout
			 switch (newImageLayout)
			 {
			 case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				 // Image will be used as a transfer destination
				 // Make sure any writes to the image have been finished
				 imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				 // Image will be used as a transfer source
				 // Make sure any reads from the image have been finished
				 imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				 // Image will be used as a color attachment
				 // Make sure any writes to the color buffer have been finished
				 imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				 // Image layout will be used as a depth/stencil attachment
				 // Make sure any writes to depth/stencil buffer have been finished
				 imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				 break;

			 case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				 // Image will be read in a shader (sampler, input attachment)
				 // Make sure any writes to the image have been finished
				 if (imageMemoryBarrier.srcAccessMask == 0)
				 {
					 imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				 }
				 imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				 break;
			 default:
				 // Other source layouts aren't handled (yet)
				 break;
			 }

			 // Put barrier inside setup command buffer
			 vkCmdPipelineBarrier(
				 cmdbuffer,
				 srcStageMask,
				 dstStageMask,
				 0,
				 0, nullptr,
				 0, nullptr,
				 1, &imageMemoryBarrier);
		 }

		 // Fixed sub resource on first mip level and layer
		 inline void setImageLayout(
			 VkCommandBuffer cmdbuffer,
			 VkImage image,
			 VkImageAspectFlags aspectMask,
			 VkImageLayout oldImageLayout,
			 VkImageLayout newImageLayout,
			 VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			 VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
		 {
			 VkImageSubresourceRange subresourceRange = {};
			 subresourceRange.aspectMask = aspectMask;
			 subresourceRange.baseMipLevel = 0;
			 subresourceRange.levelCount = 1;
			 subresourceRange.layerCount = 1;
			 setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
		 }


		 inline void insertImageMemoryBarrier(
			 VkCommandBuffer cmdbuffer,
			 VkImage image,
			 VkAccessFlags srcAccessMask,
			 VkAccessFlags dstAccessMask,
			 VkImageLayout oldImageLayout,
			 VkImageLayout newImageLayout,
			 VkPipelineStageFlags srcStageMask,
			 VkPipelineStageFlags dstStageMask,
			 VkImageSubresourceRange subresourceRange)
		 {
			 VkImageMemoryBarrier imageMemoryBarrier = vkbase::initializers::imageMemoryBarrier();
			 imageMemoryBarrier.srcAccessMask = srcAccessMask;
			 imageMemoryBarrier.dstAccessMask = dstAccessMask;
			 imageMemoryBarrier.oldLayout = oldImageLayout;
			 imageMemoryBarrier.newLayout = newImageLayout;
			 imageMemoryBarrier.image = image;
			 imageMemoryBarrier.subresourceRange = subresourceRange;

			 vkCmdPipelineBarrier(
				 cmdbuffer,
				 srcStageMask,
				 dstStageMask,
				 0,
				 0, nullptr,
				 0, nullptr,
				 1, &imageMemoryBarrier);
		 }

    };

}