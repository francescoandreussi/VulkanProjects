VULKAN_INCLUDE = /usr/include/vulkan/
STB_INCLUDE = /usr/lib/stb
VULKAN_LIB = /usr/lib/x86_64-linux-gnu/
GLSLPATH = /home/eldomo/vulkan-tools-1.1.121.0~rc2/glslang/bin
CFLAGS = -std=c++17 -I$(VULKAN_INCLUDE) -I$(STB_INCLUDE)
LDFLAGS = -L$(VULKAN_LIB) `pkg-config --static --libs glfw3` -lvulkan -lX11

VkWarp: VkWarp.cpp
	#$(GLSLPATH)/glslangValidator -h
	$(GLSLPATH)/glslangValidator -o shaders/vert.spv -V shaders/shader.vert
	$(GLSLPATH)/glslangValidator -o shaders/frag.spv -V shaders/shader.frag
	g++ $(CFLAGS) -o vkWarp VkWarp.cpp $(LDFLAGS)

.PHONY: run clean

run: VkWarp
	./vkWarp

runSW: VkWarp
	./vkWarp textures/SimpleWarpUVMS.png textures/SimpleWarpUVLS.png

runSWI: VkWarp
	./vkWarp textures/SimpleWarpUVIntensityMS.png textures/SimpleWarpUVIntensityLS.png

runWarp: VkWarp
	./vkWarp textures/WarpUVMS.png textures/WarpUVLS.png full

capture: VkWarp
	./vkWarp capture

captureSW: VkWarp
	./vkWarp capture textures/SimpleWarpUVMS.png textures/SimpleWarpUVLS.png

captureSWI: VkWarp
	./vkWarp capture textures/SimpleWarpUVIntensityMS.png textures/SimpleWarpUVIntensityLS.png

captureWarp: VkWarp
	./vkWarp capture textures/WarpUVMS.png textures/WarpUVLS.png full

clean:
	rm -f vkWarp
	rm -f shaders/vert.spv
	rm -f shaders/frag.spv