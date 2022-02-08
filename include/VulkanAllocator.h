#pragma once

#include "vk_mem_alloc.h"

#include <fstream>
#include <iostream>
#include <memory>


namespace vkbase {

	struct Buffer {

		Buffer(VmaAllocator& alloc) {
			allocator = alloc;
		}

		VkBuffer buffer;
		VmaAllocator allocator;
		VmaAllocation allocation;
		VkDescriptorBufferInfo descriptor;

		~Buffer() {
			vmaDestroyBuffer(allocator, buffer, allocation);
		}

	};

	struct Image {

		VmaAllocator allocator;

		VkImageCreateInfo imageInfo;
		VkImageView imageView;
		VkImage image;
		VmaAllocationCreateInfo allocInfo;
		VmaAllocation allocation;

	};

struct VulkanAllocator {

		VmaAllocator allocator;

		VulkanAllocator(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {

			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = physicalDevice;
			allocatorInfo.device = logicalDevice;

			if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
				throw std::runtime_error("failed to create vma allocator");
			}
		}

		VmaAllocator get_allocator() {
			return allocator;
		}

		~VulkanAllocator() {
			vmaDestroyAllocator(allocator);
		}

		//FIXME: needs a newer assimp version to fix this assimp error for clang++ compilers
		void printStats() {

			std::ofstream ofstream;

			ofstream.open("allocator_stats.json");
			if (!ofstream) {
				std::cerr << "failed to create stats file" << std::endl;
				return;
			}

			char* statsString = "stats";

			vmaBuildStatsString(allocator, &statsString, true);
			printf("%s\n", statsString);

			ofstream << statsString;
			vmaFreeStatsString(allocator, statsString);

			std::cout << "allocator stats sucessfully written" << std::endl;

		}

		//TODO: not really used for now since mem_usage can be different
		void createImage(vkbase::Image* image, VmaMemoryUsage mem_usage, VmaAllocationInfo* vma_alloc_info, VkDeviceSize size = 0) {
			
			// TODO: create image using vulkan memory allocator style
			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = mem_usage;

			if (vmaCreateImage(allocator, &image->imageInfo, &allocInfo, &image->image, &image->allocation, vma_alloc_info) != VK_SUCCESS) {
				throw std::runtime_error("failed to create vma image");
			}
		}


		void createBuffer(vkbase::Buffer* vk_buffer, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage, VmaAllocationInfo* vma_alloc_info, VkDeviceSize size = 0) {
			
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			bufferInfo.size = size;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = mem_usage;

			if (mem_usage == VMA_MEMORY_USAGE_CPU_ONLY) {
				//create host buffer
				allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
				if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vk_buffer->buffer, &vk_buffer->allocation, vma_alloc_info) != VK_SUCCESS) {
					throw std::runtime_error("failed to create staging (cpu) buffer");
				}
				//NOTE: do not forget to destroy (especially the staging) buffer afterwards
			} else {
				//create device local buffer
				allocInfo.flags = 0;
				if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vk_buffer->buffer, &vk_buffer->allocation, nullptr) != VK_SUCCESS) {
					throw std::runtime_error("failed to create device local buffer");
				}
			}
		}

	};

}
