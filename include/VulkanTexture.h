#pragma once

#include <string>

#include "VulkanTools.h"
#include "vk_mem_alloc.h"


#include <gli.hpp>
#include <regex>

#include "VulkanDevice.h"
#include "VulkanInitializers.h"
#include "VulkanGlobals.h"

#include "tinygltf/tiny_gltf.h"
#include "ktx.h"
#include "ktxvulkan.h"


namespace vkbase {

	class BaseTexture {

	public:
		VmaAllocation allocation;
		VkImageView view = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;

		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkDescriptorImageInfo descriptor{};

		std::string name;
		int tex_width;
		int tex_depth;
		int tex_height;
		int tex_channels;
		int tex_arraylayers;
		int tex_miplevels;

		BaseTexture() : name(""), tex_width(0), 
			tex_depth(0), tex_height(0), tex_channels(0), 
			tex_arraylayers(0), tex_miplevels(0) {
			allocation = VK_NULL_HANDLE;

			view = VK_NULL_HANDLE;
			image = VK_NULL_HANDLE;
			sampler = VK_NULL_HANDLE;
		}

		virtual ~BaseTexture() {
			vkDestroyImageView(global_device->logicalDevice, view, nullptr);
			vkDestroySampler(global_device->logicalDevice, sampler, nullptr);
			vmaDestroyImage(global_allocator->allocator, image, allocation);
		}

		virtual void loadTextureFromFile(std::string filename, VkFormat format, 
			VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, bool forceLinear = false) = 0;


		virtual void updateDescriptor() {
			descriptor.sampler = sampler;
			descriptor.imageView = view;
			descriptor.imageLayout = imageLayout;
		}

	};


	class PNGTexture : public BaseTexture {

	public:
		virtual void loadTextureFromFile(std::string filename, VkFormat format,
			VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, bool forceLinear = false) {

		}

		//needed since tiny_gltf already reads the PNG images via stb
		virtual void createImage(tinygltf::Image& in_image, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, bool forceLinear = false) {

			//allocate it
			name = in_image.name;
			tex_width = in_image.width;
			tex_height = in_image.height;
			tex_channels = in_image.component;
			tex_miplevels = 1;
			
			VkDeviceSize size = tex_width * tex_height * tex_channels;

			VmaAllocationInfo staging_image_alloc_info = {};
			std::unique_ptr<vkbase::Buffer> staging_buffer = std::make_unique<vkbase::Buffer>(global_allocator->allocator);
			global_allocator->createBuffer(staging_buffer.get(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &staging_image_alloc_info, size);

			memcpy(staging_image_alloc_info.pMappedData, in_image.image.data(), size);

			std::vector<VkBufferImageCopy> bufferCopyRegions;
			for (uint32_t i = 0; i < tex_miplevels; i++) {
				VkBufferImageCopy bufferCopyRegion;
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = i;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex_width);
				bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex_height);
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.imageOffset = { 0, 0, 0 };
				bufferCopyRegion.bufferOffset = 0;
				bufferCopyRegion.bufferRowLength = 0;
				bufferCopyRegion.bufferImageHeight = 0;
				bufferCopyRegions.push_back(bufferCopyRegion);
			}

			VkImageCreateInfo imageCreateInfo = vkbase::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = tex_miplevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { (uint32_t)tex_width, (uint32_t)tex_height, 1 };
			imageCreateInfo.usage = imageUsageFlags;
			imageCreateInfo.flags = 0;

			if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			VmaAllocationCreateInfo imageAllocCreateInfo = {};
			imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			if (vmaCreateImage(global_allocator->allocator, &imageCreateInfo, &imageAllocCreateInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image");
			}

			// Use a separate command buffer for texture loading
			VkCommandBuffer copyCmd = global_device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkImageMemoryBarrier imgMemBarrier = vkbase::initializers::imageMemoryBarrier();
			imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgMemBarrier.subresourceRange.baseMipLevel = 0;
			imgMemBarrier.subresourceRange.levelCount = tex_miplevels;
			imgMemBarrier.subresourceRange.baseArrayLayer = 0;
			imgMemBarrier.subresourceRange.layerCount = 1;
			imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imgMemBarrier.image = image;
			imgMemBarrier.srcAccessMask = 0;
			imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgMemBarrier);


			vkCmdCopyBufferToImage(copyCmd, staging_buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bufferCopyRegions.size(), bufferCopyRegions.data());

			imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imgMemBarrier.image = image;
			imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgMemBarrier);

			global_device->flushCommandBuffer(copyCmd, copyQueue);

			//create sampler
			VkSamplerCreateInfo samplerCreateInfo = vkbase::initializers::samplerCreateInfo();
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerCreateInfo.minLod = 0.0f;
			// Max level-of-detail should match mip level count
			samplerCreateInfo.maxLod = tex_miplevels;
			// Only enable anisotropic filtering if enabled on the devicec
			samplerCreateInfo.maxAnisotropy = 16.0f;
			samplerCreateInfo.anisotropyEnable = VK_TRUE;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

			if (vkCreateSampler(global_device->logicalDevice, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS) {
				throw std::runtime_error("failed to create sampler");
			}

			//create image view
			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = format;
			viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			// Linear tiling usually won't support mip maps
			// Only set mip map count if optimal tiling is used
			viewCreateInfo.subresourceRange.levelCount = tex_miplevels;
			viewCreateInfo.image = image;

			if (vkCreateImageView(global_device->logicalDevice, &viewCreateInfo, nullptr, &view)) {
				throw std::runtime_error("failed to create image view");
			}

			// Update descriptor image info member that can be used for setting up descriptor sets
			updateDescriptor();

		}




	private:

	};



	class KtxTexture : public BaseTexture {

	public:

		virtual void loadTextureFromFile(std::string filename, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, bool forceLinear = false) override {
			ktxTexture* ktxTexture;

			ktxResult result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

			assert(KTX_SUCCESS == result);

			tex_width = ktxTexture->baseWidth;
			tex_height = ktxTexture->baseHeight;
			tex_depth = ktxTexture->baseDepth;
			tex_miplevels = ktxTexture->numLevels;

			ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
			ktx_size_t ktxTextureSize = ktxTexture_GetDataSize(ktxTexture);

			VkFormatProperties formatProperties = {};
			vkGetPhysicalDeviceFormatProperties(global_device->physicalDevice, format, &formatProperties);

			// Only use linear tiling if requested (and supported by the device)
			// Support for linear tiling is mostly limited, so prefer to use
			// optimal tiling instead
			// On most implementations linear tiling will only support a very
			// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
			VmaAllocationInfo staging_image_alloc_info = {};
			std::unique_ptr<vkbase::Buffer> staging_buffer = std::make_unique<vkbase::Buffer>(global_allocator->allocator);
			global_allocator->createBuffer(staging_buffer.get(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &staging_image_alloc_info, ktxTextureSize);

			//copy to image buffer
			memcpy(staging_image_alloc_info.pMappedData, ktxTextureData, ktxTextureSize);


			std::vector<VkBufferImageCopy> bufferCopyRegions;
			for (uint32_t i = 0; i < tex_miplevels; i++) {
				ktx_size_t offset;
				KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
				assert(result == KTX_SUCCESS);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = i;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = std::max(1u, ktxTexture->baseWidth >> i);
				bufferCopyRegion.imageExtent.height = std::max(1u, ktxTexture->baseHeight >> i);
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;

				bufferCopyRegions.push_back(bufferCopyRegion);
			}

			VkImageCreateInfo imageCreateInfo = vkbase::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = tex_miplevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { (uint32_t)tex_width, (uint32_t)tex_height, 1 };
			imageCreateInfo.usage = imageUsageFlags;

			if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			VmaAllocationCreateInfo imageAllocCreateInfo = {};
			imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			vmaCreateImage(global_allocator->allocator, &imageCreateInfo, &imageAllocCreateInfo, &image, &allocation, nullptr);

			// Use a separate command buffer for texture loading
			VkCommandBuffer copyCmd = global_device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkImageMemoryBarrier imgMemBarrier = vkbase::initializers::imageMemoryBarrier();
			imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgMemBarrier.subresourceRange.baseMipLevel = 0;
			imgMemBarrier.subresourceRange.levelCount = tex_miplevels;
			imgMemBarrier.subresourceRange.baseArrayLayer = 0;
			imgMemBarrier.subresourceRange.layerCount = 1;
			imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imgMemBarrier.image = image;
			imgMemBarrier.srcAccessMask = 0;
			imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgMemBarrier);


			vkCmdCopyBufferToImage(copyCmd, staging_buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bufferCopyRegions.size(), bufferCopyRegions.data());

			imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imgMemBarrier.image = image;
			imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgMemBarrier);

			global_device->flushCommandBuffer(copyCmd, copyQueue);

			ktxTexture_Destroy(ktxTexture);

			//create sampler
			VkSamplerCreateInfo samplerCreateInfo = vkbase::initializers::samplerCreateInfo();
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerCreateInfo.minLod = 0.0f;
			// Max level-of-detail should match mip level count
			samplerCreateInfo.maxLod = tex_miplevels;
			// Only enable anisotropic filtering if enabled on the devicec
			samplerCreateInfo.maxAnisotropy = 16.0f;
			samplerCreateInfo.anisotropyEnable = VK_TRUE;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

			if (vkCreateSampler(global_device->logicalDevice, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS) {
				throw std::runtime_error("failed to create sampler");
			}

			//create image view
			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = format;
			viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			// Linear tiling usually won't support mip maps
			// Only set mip map count if optimal tiling is used
			viewCreateInfo.subresourceRange.levelCount = tex_miplevels;
			viewCreateInfo.image = image;
			
			if (vkCreateImageView(global_device->logicalDevice, &viewCreateInfo, nullptr, &view)) {
				throw std::runtime_error("failed to create image view");
			}

			// Update descriptor image info member that can be used for setting up descriptor sets
			updateDescriptor();
		}

	public:

	};



}

