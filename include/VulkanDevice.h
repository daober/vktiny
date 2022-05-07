#pragma once

#include "VulkanTools.h"
#include "VulkanInitializers.h"
#include "VulkanInitializers.h"


namespace vkbase {

    struct VulkanDevice {

        VulkanDevice( ) { }

        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memoryProps;
        VkQueue graphicsQueue;
        VkQueue presentQueue;

		VkCommandPool command_pool;

        //VulkanDevice(VkPhysicalDevice physicalDevice){
          //  this->physicalDevice = physicalDevice;
        //}

		VkCommandPool createCommandPool() {
			VkCommandPool cmdPool;

			return cmdPool;
		}

		VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false ) {
			
			VkCommandBufferAllocateInfo cmdBufferAllocateInfo = vkbase::initializers::commandBufferAllocateInfo(command_pool, level, 1);
			
			VkCommandBuffer cmdBuffer;
			vkAllocateCommandBuffers(logicalDevice, &cmdBufferAllocateInfo, &cmdBuffer);

			if (begin) {
				VkCommandBufferBeginInfo cmdBufInfo = vkbase::initializers::commandBufferBeginInfo();
				vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
			}

			return cmdBuffer;
		}


        ~VulkanDevice( ) {
            
			vkDestroyCommandPool(logicalDevice, command_pool, nullptr);
			vkDestroyDevice( logicalDevice, nullptr );

        }

        void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface){
            //get physical device
            uint32_t physicalDeviceCount = 0;

            vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, nullptr );

            if ( !physicalDeviceCount ) {
                throw std::runtime_error( "no physical device with vulkan support found" );
            }

            std::vector< VkPhysicalDevice > physicalDevices( physicalDeviceCount );
            if ( vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, physicalDevices.data( ) ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to enumerate vulkan physical device" );
            }

            for ( const VkPhysicalDevice& dev : physicalDevices ) {
                if ( isDeviceSuitable( dev, surface ) ) {
                    physicalDevice = dev;
                    break; //take the first suitable device (usually the dedicated gpu)
                }
            }

            if ( physicalDevice == VK_NULL_HANDLE ) {
                throw std::runtime_error( "failed to find a suitable GPU!" );
            }
        }

		unsigned int rateDeviceSuitability(VkPhysicalDevice device){
			unsigned int score = 0;

			//TODO: rate GPU suitability
			//if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
			//	score+=1000;	
			//}

			//geometry shader are needed
			//if (!deviceFeatures.geometryShader){
			//	return 0;
			//}
			return score;
		}

        void createLogicalDevice( VkSurfaceKHR surface ){
            QueueFamilyIndices indices = findQueueFamilies( physicalDevice, surface );

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

            float queuePriority = 1.0f;
            for ( uint32_t queueFamily : uniqueQueueFamilies ) {
                VkDeviceQueueCreateInfo queueCreateInfo = {};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.emplace_back( queueCreateInfo );
            }


            VkPhysicalDeviceFeatures deviceFeatures = {};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
			deviceFeatures.fillModeNonSolid = VK_TRUE;

            VkDeviceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.pQueueCreateInfos = queueCreateInfos.data( );
            createInfo.queueCreateInfoCount = static_cast< uint32_t >(queueCreateInfos.size( ));
            createInfo.pEnabledFeatures = &deviceFeatures;

            createInfo.enabledExtensionCount = static_cast< uint32_t >(deviceExtensions.size( ));
            createInfo.ppEnabledExtensionNames = deviceExtensions.data( );

            if ( enableValidationLayers ) {
                createInfo.enabledLayerCount = static_cast< uint32_t >(validationLayers.size( ));
                createInfo.ppEnabledLayerNames = validationLayers.data( );
            } else {
                createInfo.enabledLayerCount = 0;
            }

            if ( vkCreateDevice( physicalDevice, &createInfo, nullptr, &logicalDevice ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to create logical vkdevice" );
            }

            vkGetDeviceQueue( logicalDevice, indices.graphicsFamily, 0, &graphicsQueue );
            vkGetDeviceQueue( logicalDevice, indices.presentFamily, 0, &presentQueue );

			command_pool = createCommandPool(indices.graphicsFamily);
        }

		VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
		{
			VkCommandPoolCreateInfo cmdPoolInfo = {};
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
			cmdPoolInfo.flags = createFlags;
			VkCommandPool cmdPool;
			
			vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool);
			
			return cmdPool;
		}


        bool isDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR surface ) {
            QueueFamilyIndices indices = findQueueFamilies( device, surface );

            bool extensionsSupported = checkDeviceExtensionSupport( device );

            bool swapChainAdequate = false;
            if ( extensionsSupported ) {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport( device, surface );
                swapChainAdequate = !swapChainSupport.formats.empty( ) && !swapChainSupport.presentModes.empty( );
            }

            return (indices.isComplete( ) && extensionsSupported && swapChainAdequate);
        }

        bool checkDeviceExtensionSupport( VkPhysicalDevice device ) {

            uint32_t extensionsCount;
            vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionsCount, nullptr );

            std::vector<VkExtensionProperties> availableExtensions( extensionsCount );
            vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionsCount, availableExtensions.data( ) );

            std::set<std::string> requiredExtensions( deviceExtensions.begin( ), deviceExtensions.end( ) );

            for ( const auto& extension : availableExtensions ) {
                requiredExtensions.erase( extension.extensionName );
            }

            return requiredExtensions.empty( );
        }


		void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true)
		{
			if (commandBuffer == VK_NULL_HANDLE)
			{
				return;
			}

			vkEndCommandBuffer(commandBuffer);

			VkSubmitInfo submitInfo = vkbase::initializers::submitInfo();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			// Create fence to ensure that the command buffer has finished executing
			VkFenceCreateInfo fenceInfo = vkbase::initializers::fenceCreateInfo(0);
			VkFence fence;
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence);

			// Submit to the queue
			vkQueueSubmit(queue, 1, &submitInfo, fence);
			// Wait for the fence to signal that command buffer has finished executing
			vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

			vkDestroyFence(logicalDevice, fence, nullptr);

			if (free)
			{
				vkFreeCommandBuffers(logicalDevice, command_pool, 1, &commandBuffer);
			}
		}

    };

}

namespace vkbase {
	namespace vkDeviceInfo {

		struct Infos {
			std::string device_string = "Vulkan Physical Device: ";
			std::string vendor = "Vendor ID: ";
			std::string device_type = "Device Type: ";
			std::string max_allocation_count = "Max Allocation Count: ";
			std::string driver_version = "Driver Version: ";
			std::string api_version = "API Version: ";
			std::string memory_heap_size = "Memory Heap Size (Bytes): ";
			std::string tesselation_shader = "Tesselation Support: ";
			std::string geometry_shader = "Geometry Shader Support: ";
			std::string sparse_binding = "Sparse Binding Support: ";

			void create_basic_infos(vkbase::VulkanDevice* vulkan_device) {

				device_string.append(vulkan_device->properties.deviceName);
				vendor.append(std::to_string(vulkan_device->properties.deviceID));

				switch (vulkan_device->properties.deviceType) {
				case 0:
					device_type.append("VK_PHYSICAL_DEVICE_TYPE_OTHER ");
					break;
				case 1:
					device_type.append("VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ");
					break;
				case 2:
					device_type.append("VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ");
					break;
				case 3:
					device_type.append("VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ");
					break;
				case 4:
					device_type.append("VK_PHYSICAL_DEVICE_TYPE_CPU ");
					break;
				default:
					device_type.append("NOT FOUND");
					break;
				}

				max_allocation_count.append(std::to_string(vulkan_device->properties.limits.maxMemoryAllocationCount));
				driver_version.append(std::to_string(vulkan_device->properties.driverVersion));
				api_version.append(std::to_string(vulkan_device->properties.apiVersion));
				memory_heap_size.append(std::to_string(vulkan_device->memoryProps.memoryHeaps->size));
				
				if (vulkan_device->features.geometryShader) {
					geometry_shader.append("true");
				} else {
					geometry_shader.append("false");
				}
				if (vulkan_device->features.tessellationShader) {
					tesselation_shader.append("true");
				} else {
					tesselation_shader.append("false");
				}
				if (vulkan_device->features.sparseBinding) {
					sparse_binding.append("true");
				} else {
					sparse_binding.append("false");
				}
				
			}

		};

	}
}

