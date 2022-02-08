#include "../include/VulkanApp.h"
#include <iostream>


#if defined(__ANDROID__)
int android_main(android_app* app_state){
#else
int main( ) {
#endif
    VulkanApp* vulkan_app = new VulkanApp( );
    try {
		vulkan_app->prepare();
    } catch ( const std::exception& e ) {
        std::cerr << e.what( ) << std::endl;
        return EXIT_FAILURE;
    }
    delete vulkan_app;
    return EXIT_SUCCESS;
}
