#pragma once

#include "VulkanTools.h"
#include "imgui/imgui.h"


void mouse_pos_callback( GLFWwindow* window, double xpos, double ypos );
void mouse_button_callback( GLFWwindow* window, int button, int action, int mods );
void framebufferResizeCallback( GLFWwindow* window, int width, int height );


namespace vkbase {

struct VulkanSurface {
        GLFWwindow* window;
        VkSurfaceKHR surface;
		VkInstance* inst;

        void createSurface( VkInstance* instance ) {

			inst = instance;

            if ( glfwCreateWindowSurface( *inst, window, nullptr, &surface ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to create window surface" );
            }
        }

        void initWindow( ) {

            glfwInit( );

            glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
            glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

            window = glfwCreateWindow( WIDTH, HEIGHT, "VkTinyEngine", nullptr, nullptr );
            glfwSetWindowUserPointer( window, this );
            glfwSetFramebufferSizeCallback( window, framebufferResizeCallback );
            glfwSetCursorPosCallback( window, mouse_pos_callback );
            //glfwSetMouseButtonCallback( window, mouse_button_callback );

            glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
        }

		~VulkanSurface() {
			vkDestroySurfaceKHR(*inst, surface, nullptr);
		}

    };


}