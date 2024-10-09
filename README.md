# VkStarterProject

This repository contains setup code for making Vulkan applications, along with some simple usage examples. 
The contents follow almost the entire https://vulkan-tutorial.com/ (including compute shaders), but the code is
structured in a more modular way, models are loaded in gltf format and ImGui is integrated to provide interactive UI. 
A lot of advice from https://vkguide.dev/ is also incorporated, in particular usage of [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) 
to do initialization, usage of [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) and a deletion queue.

### Cloning

This repository contains submodules and should be cloned recursively:

	git clone --recursive https://github.com/Langwedocjusz/VkStarterProject <TargetDir>

### Dependencies
This is a cmake project, so make sure cmake is installed and added to your path. Also make sure you have [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) 
installed, as this is needed not only for the actual application, but also to provide the glslc shader compiler. This project uses Vulkan 1.3 so make sure 
your graphics drivers support that.

### Aditional setup

After cloning the repository make sure to run the two bundled python scripts:

	./scripts/DownloadAssets.py
	./scripts/CompileShaders.py

The first one will download models and textures used by the examples, and the second one will compile the provided shader 
source files into the bytecode SPIR-V format consumed by the actual application.

### Building

To build you need to call cmake to generate appropriate buildsystem files. For your convenience there are two scripts provided to do that
at the top level of the repository: 

`Build.sh` - meant to be used on Linux, will generate makefiles and build the project using make, 
`WinGenerateProjects.bat` - meant to be used on Windows, will generate a Visual Studio solution instead.
