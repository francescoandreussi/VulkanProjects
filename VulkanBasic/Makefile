VULKAN_INCLUDE = /usr/include/vulkan/
STB_INCLUDE = /usr/lib/stb
VULKAN_LIB = /usr/lib/x86_64-linux-gnu/
GLSLPATH = /home/eldomo/vulkan-tools-1.1.121.0~rc2/glslang/bin
CFLAGS = -std=c++17 -I$(VULKAN_INCLUDE) -I$(STB_INCLUDE)
LDFLAGS = -L$(VULKAN_LIB) `pkg-config --static --libs glfw3` -lvulkan 

VkBasic: VkBasic.cpp
	#$(GLSLPATH)/glslangValidator -h
	$(GLSLPATH)/glslangValidator -o shaders/vert.spv -V shaders/shader.vert
	$(GLSLPATH)/glslangValidator -o shaders/frag.spv -V shaders/shader.frag
	g++ $(CFLAGS) -o VkBasic VkBasic.cpp $(LDFLAGS) 

.PHONY: run clean

run: VkBasic
	./VkBasic

clean:
	rm -f VkBasic
	rm -f shaders/vert.spv
	rm -f shaders/frag.spv