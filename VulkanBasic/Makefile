VULKAN_INCLUDE = /usr/include/vulkan/
VULKAN_LIB = /usr/lib/x86_64-linux-gnu/
CFLAGS = -std=c++17 -I$(VULKAN_INCLUDE)
LDFLAGS = -L$(VULKAN_LIB) `pkg-config --static --libs glfw3` -lvulkan 

VkBasic: VkBasic.cpp
	g++ $(CFLAGS) -o VkBasic VkBasic.cpp $(LDFLAGS) 

.PHONY: run clean

run: VkBasic
	./VkBasic

clean:
	rm -f VkBasic