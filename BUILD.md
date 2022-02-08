# WINDOWS BUILDING
dependencies:
cmake
Vulkan SDK from lunarG

- Vulkan SDK from lunarG (link: https://vulkan.lunarg.com/sdk/home#linux) (vulkan headers, vulkan runtime loader)

## BUILD INSTRUCTUIONS

- mkdir build && cd build
- cmake ../path/to/project/dir -G "Visual Studio 14 2015 Win64" (NOTE: Project is created on Visual Studio 16 2019, but using previous versions of visual studio should be no problem)
- open .sln in build directory, build it and copy 'assimp-vc140-mt.dll' from bin directory to the executable folder
- execute

# LINUX BUILDING (tested on UBUNTU 18.04)

following dependencies are required:

- assimp: installed via apt (libassimp-dev)
- Vulkan SDK from lunarG (link: https://vulkan.lunarg.com/sdk/home#linux) (vulkan headers, vulkan runtime loader)

- glfw3-dev (for platform agnostic window creation):
    glfw3 needs additional dependencies like:
    - xcursor-dev
    - xinerama-dev

NOTE: a recent gpu driver version is needed, since Vulkan 1.2 is used in this project.
tested with NVIDIA DRIVER Version: 390.116

there is no nouveau vulkan driver available yet.

## BUILD INSTRUCTIONS

- mkdir build && cd build
- cmake /path/to/project/dir
- make [-j4]