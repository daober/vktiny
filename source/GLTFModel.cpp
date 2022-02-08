#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>


#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#define STB_IMAGE_IMPLEMENTATION

#define STBI_MSC_SECURE_CRT

#include "../include/GLTFModel.h"
#include <stdexcept>



bool loadImageDataFunc(tinygltf::Image* image, const int imageIndex, std::string* error, std::string* warning, int req_width, int req_height, const unsigned char* bytes, int size, void* userData)
{
	// KTX files will be handled by our own code
	if (image->uri.find_last_of(".") != std::string::npos) {
		if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx") {
			return true;
		}
	}

	return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
}

bool loadImageDataFuncEmpty(tinygltf::Image* image, const int imageIndex, std::string* error, std::string* warning, int req_width, int req_height, const unsigned char* bytes, int size, void* userData)
{
	// This function will be used for samples that don't require images to be loaded
	return true;
}


int GLTFBase::loadTextures(tinygltf::Model& model) {
	textures.resize(model.textures.size());
	for (size_t i = 0; i < model.textures.size(); i++) {
		textures.at(i).imageIndex = model.textures.at(i).source;
	}
	return 0;
}


void GLTFBase::createVertexBuffers(void) {

	VkDeviceSize vertexBufferSize = vertices.size() * sizeof(Vertex);
	VmaAllocationInfo stagingVertexBufferAllocInfo = {};

	std::unique_ptr<vkbase::Buffer> staging_buffer = std::make_unique<vkbase::Buffer>(global_allocator->allocator);
	global_allocator->createBuffer(staging_buffer.get(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingVertexBufferAllocInfo, vertexBufferSize);

	memcpy(stagingVertexBufferAllocInfo.pMappedData, vertices.data(), vertices.size() * sizeof(vertices[0]));

	vertexBuffer = std::make_unique<vkbase::Buffer>(global_allocator->allocator);
	global_allocator->createBuffer(vertexBuffer.get(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, nullptr, vertexBufferSize);

	VkBufferCopy vbCopyRegion = {};
	vbCopyRegion.srcOffset = 0;
	vbCopyRegion.dstOffset = 0;
	vbCopyRegion.size = vertexBufferSize;

	VkCommandBufferAllocateInfo cmdBufferInfo = {};
	cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferInfo.commandBufferCount = 1;
	cmdBufferInfo.commandPool = global_device->command_pool;

	VkCommandBuffer tmpCommandBuffer;
	if (vkAllocateCommandBuffers(global_device->logicalDevice, &cmdBufferInfo, &tmpCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate temp command buffer");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	//begin recording
	vkBeginCommandBuffer(tmpCommandBuffer, &beginInfo);
	vkCmdCopyBuffer(tmpCommandBuffer, staging_buffer->buffer, vertexBuffer->buffer, 1, &vbCopyRegion);
	vkEndCommandBuffer(tmpCommandBuffer);

	//now submit
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tmpCommandBuffer;

	vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(copyQueue);

	vkFreeCommandBuffers(global_device->logicalDevice, global_device->command_pool, 1, &tmpCommandBuffer);
}


void GLTFBase::createIndexBuffers(void) {

	VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
	VmaAllocationInfo stagingIndexBufferAllocInfo = {};

	//TODO: is this needed?
	bufferIndices.count = static_cast<uint32_t>(indices.size());

	std::unique_ptr<vkbase::Buffer> staging_buffer = std::make_unique<vkbase::Buffer>(global_allocator->allocator);
	global_allocator->createBuffer(staging_buffer.get(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &stagingIndexBufferAllocInfo, indexBufferSize);
	memcpy(stagingIndexBufferAllocInfo.pMappedData, indices.data(), indices.size() * sizeof(indices[0]));

	indexBuffer = std::make_unique<vkbase::Buffer>(global_allocator->allocator);
	global_allocator->createBuffer(indexBuffer.get(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, nullptr, indexBufferSize);

	//copy via command buffer
	VkBufferCopy vbCopyRegion = {};
	vbCopyRegion.srcOffset = 0;
	vbCopyRegion.dstOffset = 0;
	vbCopyRegion.size = indexBufferSize;

	VkCommandBufferAllocateInfo cmdBufferInfo = {};
	cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferInfo.commandBufferCount = 1;
	cmdBufferInfo.commandPool = global_device->command_pool;

	VkCommandBuffer tmpCommandBuffer;
	if (vkAllocateCommandBuffers(global_device->logicalDevice, &cmdBufferInfo, &tmpCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate temp command buffer");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	//begin recording
	vkBeginCommandBuffer(tmpCommandBuffer, &beginInfo);
	vkCmdCopyBuffer(tmpCommandBuffer, staging_buffer->buffer, indexBuffer->buffer, 1, &vbCopyRegion);
	vkEndCommandBuffer(tmpCommandBuffer);

	//now submit
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tmpCommandBuffer;

	vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(copyQueue);

	vkFreeCommandBuffers(global_device->logicalDevice, global_device->command_pool, 1, &tmpCommandBuffer);
}

int GLTFBase::loadMaterials(tinygltf::Model& model) {

	materials.resize(model.materials.size());

	for (size_t i = 0; i < model.materials.size(); i++) {
		tinygltf::Material gltfMaterial = model.materials[i];

		// Get the base color factor
		if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) {
			materials[i].baseColorFactor = glm::make_vec4(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		// Get base color texture index
		if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) {
			materials[i].baseColorTextureIndex = gltfMaterial.values["baseColorTexture"].TextureIndex();
		}
		// Get the normal map texture index
		if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end()) {
			materials[i].normalTextureIndex = gltfMaterial.additionalValues["normalTexture"].TextureIndex();
		}

		materials[i].alphaMode = gltfMaterial.alphaMode;
		materials[i].alphaCutOff = (float)gltfMaterial.alphaCutoff;
		materials[i].doubleSided = gltfMaterial.doubleSided;
	}

	return 0;
}

void GLTFBase::drawNode(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, vkbase::Node node) {

	if (!node.visible) {
		return;
	}

	if (node.mesh.primitives.size() > 0) {
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 nodeMatrix = node.matrix;
		vkbase::Node* currentParent = node.parent;
		while (currentParent) {
			nodeMatrix = currentParent->matrix * nodeMatrix;
			currentParent = currentParent->parent;
		}
		// Pass the final matrix to the vertex shader using push constants
		//vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
		for (vkbase::Primitive& primitive : node.mesh.primitives) {
			if (primitive.indexCount > 0) {
				vkbase::Material& material = materials[primitive.materialIndex];
				// POI: Bind the pipeline for the node's material
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
				vkCmdDrawIndexed(cmdBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			}
		}
	}
	for (auto& child : node.children) {
		drawNode(cmdBuffer, pipelineLayout, child);
	}

}

void GLTFBase::bind(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout) {

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &(vertexBuffer->buffer), offsets);
	vkCmdBindIndexBuffer(cmdBuffer, (indexBuffer->buffer), 0, VK_INDEX_TYPE_UINT32);

	for (vkbase::Node& node : nodes) {
		drawNode(cmdBuffer, pipelineLayout, node);
	}
}

int GLTFBase::loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, vkbase::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {

	vkbase::Node node{};
	node.name = inputNode.name;

	// Get the local node matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	node.matrix = glm::mat4(1.0f);

	if (inputNode.translation.size() == 3) {
		node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
	}

	if (inputNode.rotation.size() == 4) {
		glm::quat q = glm::make_quat(inputNode.rotation.data());
		node.matrix *= glm::mat4(q);
	}

	if (inputNode.scale.size() == 3) {
		node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
	}

	if (inputNode.matrix.size() == 16) {
		node.matrix = glm::make_mat4x4(inputNode.matrix.data());
	}

	//load nodes children
	if (inputNode.children.size() > 0) {
		for (size_t i = 0; i < inputNode.children.size(); i++) {
			loadNode(input.nodes[inputNode.children[i]], input, &node, indexBuffer, vertexBuffer);
		}
	}

	// If the node contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode.mesh > -1) {
		const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];

		for (size_t i = 0; i < mesh.primitives.size(); i++) {
			const tinygltf::Primitive& gltfPrimitive = mesh.primitives[i];

			uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
			uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t indexCount = 0;
			{
				const float* positionBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				const float* tangentsBuffer = nullptr;
				size_t vertexCount = 0;

				// Get buffer data for vertex positions
				if (gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*> (&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}
				// Get buffer data for vertex normals
				if (gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// POI: This sample uses normal mapping, so we also need to load the tangents from the glTF file
				if (gltfPrimitive.attributes.find("TANGENT") != gltfPrimitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					tangentsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				// Append data to model's vertex buffer
				for (size_t j = 0; j < vertexCount; j++) {
					Vertex vertex{};
					vertex.position = glm::vec4(glm::make_vec3(&positionBuffer[j * 3]), 1.0f);
					vertex.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[j * 3]) : glm::vec3(0.0f)));
					vertex.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[j * 2]) : glm::vec3(0.0f);
					vertex.color = glm::vec3(1.0f);
					vertex.tangent = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[j * 4]) : glm::vec4(0.0f);
					vertexBuffer.push_back(vertex);
				}
			}

			//indices
			{
				const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.indices];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

				indexCount += static_cast<uint32_t>(accessor.count);

				// glTF supports different component types of indices
				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					uint32_t* buf = new uint32_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					uint16_t* buf = new uint16_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					uint8_t* buf = new uint8_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return -1;
				}
			}

			vkbase::Primitive primitive{};
			primitive.firstIndex = firstIndex;
			primitive.indexCount = indexCount;
			primitive.materialIndex = gltfPrimitive.material;
			node.mesh.primitives.push_back(primitive);
		}
	}

	if (parent) {
		parent->children.push_back(node);
	}
	else {
		nodes.push_back(node);
	}

	return 0;
}


GLTFKtxModel::GLTFKtxModel(const std::string& filepath, VkQueue copyQueue) : GLTFBase(filepath, copyQueue) {

	this->copyQueue = copyQueue;
	tinygltf::Model glTFInput;

	loader.SetImageLoader(loadImageDataFunc, nullptr);

	bool fileloaded = loader.LoadASCIIFromFile(&glTFInput, &error, &warning, filepath);

	if (!warning.empty()) {
		printf("Warning: %s\n", warning.c_str());
	}

	if (!error.empty()) {
		printf("Error: %s\n", error.c_str());
	}


	std::size_t found = filepath.find_last_of("/\\");
	path = filepath.substr(0, found);

	if (fileloaded) {
		loadImages(glTFInput);
		loadMaterials(glTFInput);
		loadTextures(glTFInput);
		tinygltf::Scene& scene = glTFInput.scenes[0];
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			tinygltf::Node& node = glTFInput.nodes[scene.nodes[i]];
			loadNode(node, glTFInput, nullptr, indices, vertices);
		}
	
		createVertexBuffers();
		createIndexBuffers();
	
	}
	else {
		std::cerr << "Failed to open glTF File" << std::endl;
	}

}


VkDescriptorImageInfo GLTFKtxModel::getTextureDescriptor(const size_t index) {
	return images[index].descriptor;
}


GLTFKtxModel::~GLTFKtxModel() {
	for (auto& mat : materials) {
		vkDestroyPipeline(global_device->logicalDevice, mat.pipeline, nullptr);
	}
}


int GLTFKtxModel::loadImages(tinygltf::Model& model) {
	
	images.resize(model.images.size());
	for (size_t i = 0; i < model.images.size(); i++) {
		tinygltf::Image& image = model.images[i];
		std::string complete_path = path + '/' + image.uri;
		images[i].loadTextureFromFile(complete_path, VK_FORMAT_R8G8B8A8_UNORM, copyQueue);
	}
	return 0;
}

GLTFPngModel::GLTFPngModel(const std::string& filepath, VkQueue copyQueue) : GLTFBase(filepath, copyQueue) {

	this->copyQueue = copyQueue;
	tinygltf::Model glTFInput;

	bool fileloaded = loader.LoadASCIIFromFile(&glTFInput, &error, &warning, filepath);

	if (!warning.empty()) {
		printf("Warning: %s\n", warning.c_str());
	}

	if (!error.empty()) {
		printf("Error: %s\n", error.c_str());
	}

	std::size_t found = filepath.find_last_of("/\\");
	path = filepath.substr(0, found);

	if (fileloaded) {
		loadImages(glTFInput);
		loadMaterials(glTFInput);
		loadTextures(glTFInput);
		tinygltf::Scene& scene = glTFInput.scenes[0];
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			tinygltf::Node& node = glTFInput.nodes[scene.nodes[i]];
			loadNode(node, glTFInput, nullptr, indices, vertices);
		}

		createVertexBuffers();
		createIndexBuffers();

	}
	else {
		std::cerr << "Failed to open glTF File" << std::endl;
	}

}

GLTFPngModel::~GLTFPngModel() {
	for (auto& mat : materials) {
		vkDestroyPipeline(global_device->logicalDevice, mat.pipeline, nullptr);
	}
}


VkDescriptorImageInfo GLTFPngModel::getTextureDescriptor(const size_t index) {
	return images[index].descriptor;
}

int GLTFPngModel::loadImages(tinygltf::Model& model) {
	images.resize(model.images.size());
	for (size_t i = 0; i < model.images.size(); i++) {
		tinygltf::Image& image = model.images[i];
		std::string complete_path = path + '/' + image.uri;
		images[i].createImage(image, VK_FORMAT_R8G8B8A8_UNORM, copyQueue);
	}
	return 0;
}