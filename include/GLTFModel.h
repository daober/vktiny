#pragma once


#include "VulkanGlobals.h"

#include "VulkanTexture.h"




namespace vkbase {

	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t materialIndex;
	};

	struct Mesh {
		std::vector<Primitive> primitives;
	};

	// A node represents an object in the glTF scene graph
	struct Node {
		Node* parent;
		std::vector<Node> children;
		Mesh mesh;
		glm::mat4 matrix;
		std::string name;
		bool visible = true;

	};

	// A glTF material stores information in e.g. the texture that is attached to it and colors
	struct Material {
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		uint32_t baseColorTextureIndex;
		uint32_t normalTextureIndex;
		std::string alphaMode = "OPAQUE";
		float alphaCutOff;
		bool doubleSided = false;
		VkDescriptorSet descriptorSet;
		VkPipeline pipeline;
	};

	// In this sample, we are only interested in the image
	struct TextureIndices {
		int32_t imageIndex;
	};

	// Single index buffer for all primitives
	struct Indices {
		int count;
		VkBuffer buffer;
		VkDeviceMemory memory;
	};
}

class GLTFBase {

public:
	GLTFBase(const std::string& filepath, VkQueue copyQueue) {};

	virtual ~GLTFBase() {};

	virtual void bind(VkCommandBuffer draw, VkPipelineLayout pipelineLayout);

	std::unique_ptr<vkbase::Buffer> vertexBuffer;
	std::unique_ptr<vkbase::Buffer> indexBuffer;

	std::vector<vkbase::TextureIndices> textures;
	std::vector<vkbase::Material> materials;
	std::vector<vkbase::Node> nodes;

	virtual VkDescriptorImageInfo getTextureDescriptor(const size_t index) = 0;

protected:
	virtual int loadTextures(tinygltf::Model& model);

	virtual int loadMaterials(tinygltf::Model& model);

	virtual int loadImages(tinygltf::Model& model) = 0;

	virtual int loadNode(const tinygltf::Node& inputnode, const tinygltf::Model& model, vkbase::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);

	virtual void drawNode(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, vkbase::Node node);

	virtual void createVertexBuffers(void);
	virtual void createIndexBuffers(void);

	tinygltf::TinyGLTF loader;

	std::string error;
	std::string warning;

	VkQueue copyQueue;

	std::string path;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vkbase::Indices bufferIndices;

	VkDevice vulkanDevice;
};


class GLTFKtxModel : public GLTFBase {

public:

	GLTFKtxModel(const std::string& filepath, VkQueue copyQueue);

	virtual ~GLTFKtxModel() override;

	VkDescriptorImageInfo getTextureDescriptor(const size_t index) override;

protected:

	int loadImages(tinygltf::Model& model) override;

	std::vector<vkbase::KtxTexture> images;

private:

};

class GLTFPngModel : public GLTFBase {

public:

	GLTFPngModel(const std::string& filepath, VkQueue copyQueue);

	virtual ~GLTFPngModel() override;

	VkDescriptorImageInfo getTextureDescriptor(const size_t index) override;

protected:

	int loadImages(tinygltf::Model& model) override;

	std::vector<vkbase::PNGTexture> images;

private:

};