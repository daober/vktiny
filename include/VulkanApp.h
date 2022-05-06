#include "VulkanRenderer.h"
#include "VulkanCamera.h"
#include "VulkanTexture.h"
#include "VulkanGlobals.h"

#include "GLTFModel.h"


class VulkanApp : public VulkanRenderer {

public:

    VulkanApp( );

    ~VulkanApp( );

	void loadAssets();

    void render( ) override;

	void prepare() override;

	void preparePipelines() override;

	void buildCommandBuffers() override;

	void renderLoop() override;

	void createPipelineLayout();

	void createDescriptorPool();

	void createDescriptorSetLayout();

	void createDescriptorSets();

	//void createApplicationInfoWindow();

	void draw();

	//void renderUIFrame();

	//void initUI();
	//void prepareUI();
	//void renderUIDrawData(VkCommandBuffer draw_cmd_buffer);

	//void processInput(GLFWwindow* window);


private:

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool;

	VmaAllocationInfo uniform_allocation_info = {};
	uboVS transform_matrices;

	float deltaTime = 0.0f;
	float lastFrame = 0.0f;

	GLTFKtxModel* model;
	//GLTFPngModel* model;
};