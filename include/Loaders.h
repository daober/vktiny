#pragma once

#if !defined(__ANDROID__)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <tuple>

#include "../include/vk_mem_alloc.h"
#include "../include/stb_image.h"

#if defined(__ANDROID__)
#include "../wrapper/vulkan_wrapper.h"
#endif


//#include "angelscript.h"

#include <iostream>

namespace vktools {

    class Loaders {

    public:

        VkShaderModule createShaderModule( VkDevice device, const std::vector<char>& code ) {
            VkShaderModule shaderModule;

            VkShaderModuleCreateInfo moduleInfo = {};
            moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleInfo.pCode = reinterpret_cast< const uint32_t* >(code.data( ));
            moduleInfo.codeSize = code.size( );

            if ( vkCreateShaderModule( device, &moduleInfo, nullptr, &shaderModule ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to create shader module" );
            }

            return shaderModule;
        }

    };

}