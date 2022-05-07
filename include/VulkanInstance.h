#pragma once

#include "VulkanTools.h"


namespace vkbase {

    struct VulkanInstance {
        VkInstance instance;

        ~VulkanInstance( ) {
            vkDestroyInstance( instance, nullptr );
        }

        std::vector<std::string> supportedInstanceExtensions;
        std::vector<const char*> enabledInstanceExtensions;

        std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

        void createInstance(){

#if defined(_WIN32)
            instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined (VK_USE_PLATFORM_ANDROID_KHR)
            instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif


            VkApplicationInfo appInfo = {};
            appInfo.apiVersion = VK_API_VERSION_1_1;
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.applicationVersion = VK_MAKE_VERSION( 0, 1, 0 );
            appInfo.pEngineName = "vktinyengine";
            appInfo.engineVersion = VK_MAKE_VERSION( 0, 1, 0 );
            appInfo.pApplicationName = "vkTinyEngine";
            appInfo.pNext = nullptr;

            VkInstanceCreateInfo instanceInfo = {};
            instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceInfo.pApplicationInfo = &appInfo;

            getInstanceExtensions();

            if (enableValidationLayers) {
                instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            instanceInfo.enabledExtensionCount = static_cast< uint32_t >(instanceExtensions.size( ));
            instanceInfo.ppEnabledExtensionNames = instanceExtensions.data( );

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

        void getInstanceExtensions() {


            uint32_t extCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
            if (extCount > 0) {
                std::vector<VkExtensionProperties> extensions(extCount);
                if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
                    for (VkExtensionProperties extension : extensions) {
                        supportedInstanceExtensions.push_back(extension.extensionName);
                    }
                }
            }

            if (enabledInstanceExtensions.size() > 0) {
                for (const char* enabledExtension : enabledInstanceExtensions) {
                    if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) == supportedInstanceExtensions.end())
                    {
                        std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
                    }
                    instanceExtensions.push_back(enabledExtension);
                }
            }

        }

    };

}