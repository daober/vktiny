#pragma once

#include "VulkanTools.h"


namespace vkbase {

    struct VulkanInstance {
        VkInstance instance;

        ~VulkanInstance( ) {
            vkDestroyInstance( instance, nullptr );
        }

        void createInstance(){

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;

            //glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

            VkApplicationInfo appInfo = {};
            appInfo.apiVersion = VK_API_VERSION_1_1;
            appInfo.applicationVersion = VK_MAKE_VERSION( 0, 1, 0 );
            appInfo.pEngineName = "vktinyengine";
            appInfo.engineVersion = VK_MAKE_VERSION( 0, 1, 0 );
            appInfo.pApplicationName = "vkTinyEngine";
            appInfo.pNext = nullptr;

            VkInstanceCreateInfo instanceInfo = {};
            instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceInfo.pApplicationInfo = &appInfo;

            auto extensions = getRequiredExtensions( );
            instanceInfo.enabledExtensionCount = static_cast< uint32_t >(extensions.size( ));
            instanceInfo.ppEnabledExtensionNames = extensions.data( );

            if ( enableValidationLayers ) {
                instanceInfo.enabledLayerCount = static_cast< uint32_t >(validationLayers.size( ));
                instanceInfo.ppEnabledLayerNames = validationLayers.data( );
            } else {
                instanceInfo.enabledLayerCount = 0;
            }

            if ( enableValidationLayers && !checkValidationLayerSupport( ) ) {
                throw std::runtime_error( "validation layers requested, but not available" );
            }

            if ( vkCreateInstance( &instanceInfo, nullptr, &instance ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to create vulkan instance" );
            }

        }


        std::vector<const char*> getRequiredExtensions( ) {

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            //glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
            // vector( pointer_to_first_element, pointer_to_first_element + (size_in_bytes_of_the_whole_array / size_of_one_element) )
            std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

            if ( enableValidationLayers ) {
                extensions.emplace_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
            }

            return extensions;
        }


    };

}