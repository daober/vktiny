#pragma once

#include "VulkanTools.h"
#include "imgui/imgui.h"


//void mouse_pos_callback( GLFWwindow* window, double xpos, double ypos );
//void mouse_button_callback( GLFWwindow* window, int button, int action, int mods );
//void framebufferResizeCallback( GLFWwindow* window, int width, int height );

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#endif

//#if defined(VK_USE_PLATFORM_WIN32_KHR)




namespace vkbase {

struct VulkanSurface {
        VkSurfaceKHR surface = nullptr;
		VkInstance inst = nullptr;

        VulkanSurface(VkInstance instance){
            inst = instance;
            surface = nullptr;
        }


#if defined(VK_USE_PLATFORM_WIN32_KHR)
        void initSurface(void* platformHandle, void* platformWindow)
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        void initSurface(ANativeWindow* window)
#endif
        {
            VkResult err = VK_SUCCESS;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
            VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
            surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
            surfaceCreateInfo.hwnd = (HWND)platformWindow;

            err = vkCreateWin32SurfaceKHR(inst, &surfaceCreateInfo, nullptr, &surface);

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
            VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
            surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
            surfaceCreateInfo.window = window;

            err = vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
#endif

            if (err != VK_SUCCESS) {
                throw std::runtime_error("Could not create surface!");
            }




        }

        void initWindow( ) {

            /*glfwInit( );

            glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
            glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

            window = glfwCreateWindow( WIDTH, HEIGHT, "VkTinyEngine", nullptr, nullptr );
            glfwSetWindowUserPointer( window, this );
            glfwSetFramebufferSizeCallback( window, framebufferResizeCallback );
            glfwSetCursorPosCallback( window, mouse_pos_callback );
            //glfwSetMouseButtonCallback( window, mouse_button_callback );

            glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );*/
        }

		~VulkanSurface() {
			vkDestroySurfaceKHR(inst, surface, nullptr);
		}

    };


}

//#endif