cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

project(VkTinyRenderer)

include_directories(external)
include_directories(external/glm-0.9.9.1/glm)
include_directories(external/gli-master/gli)

add_subdirectory(external/glfw-3.2.1)

find_package(Vulkan REQUIRED)

set(
  SOURCES source/main.cpp 
  include/VulkanRenderer.h 
  source/VulkanRenderer.cpp 
  source/VulkanApp.cpp 
  source/VMA_IMPL.cpp
  external/tinygltf/json.hpp
  external/tinygltf/stb_image.h 
  external/tinygltf/tiny_gltf.h
  #external/ktx/ktx.h
  #external/ktx/ktxvulkan.h
  include/vk_mem_alloc.h 
  include/VulkanInitializers.h 
  include/VulkanCamera.h 
  source/VulkanCamera.cpp 
  include/VulkanInstance.h 
  include/VulkanSurface.h 
  include/VulkanDevice.h 
  include/VulkanApp.h
  include/VulkanGlobals.h 
  include/VulkanAllocator.h
  include/VulkanSwapchain.h 
  include/VulkanFramebuffer.h 
  include/VulkanTools.h 
  include/VulkanTexture.h 
  include/GLTFModel.h
  source/GLTFModel.cpp
  external/imgui/imgui.cpp 
  external/imgui/imgui.h 
  external/imgui/imgui_draw.cpp 
  external/imgui/imgui_demo.cpp 
  external/imgui/imgui_draw.cpp 
  external/imgui/imgui_internal.h 
  external/imgui/imgui_widgets.cpp 
  external/imgui/imgui_impl_glfw.h 
  external/imgui/imgui_impl_glfw.cpp 
  external/imgui/imgui_impl_vulkan.h 
  external/imgui/imgui_impl_vulkan.cpp
)

IF (WIN32 OR UNIX)
include_directories(${GLFW_INCLUDE_DIR} ${Vulkan_INCLUDE_DIR})
include_directories(external/ktx)

add_executable(${PROJECT_NAME} ${SOURCES})

set(KTX_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/ktx/ktx.lib)

target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} ${Vulkan_LIBRARIES} ${KTX_LIB})
ELSE()
ENDIF()

file(COPY assets DESTINATION .)
file(COPY shaders DESTINATION .)
