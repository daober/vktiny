#pragma once

#include "VulkanAllocator.h"
#include "VulkanCamera.h"
#include "VulkanDevice.h"



struct uboVS {
    glm::mat4 model;
	glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 lightPos = glm::vec4(0.0f, 2.5f, 0.0f, 1.0f);
    glm::vec3 viewPos;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
    glm::vec4 tangent;
};


extern vkbase::VulkanAllocator* global_allocator;
extern vkbase::VulkanDevice* global_device;

extern Camera* global_camera;

extern vkbase::Buffer* global_uniform_buffer;

extern VmaAllocationInfo uniform_allocation_info;

extern uboVS transform_matrices;

extern float rotation_amount;
extern bool wireframe_mode;