VULKAN_PATH = /usr/include/vulkan/
CFLAGS = -std=c++17 -I$(VULKAN_PATH)
LDFLAGS = -L$(VULKAN_PATH) `pkg-config --static --libs glfw3` -lvulkan

VkBasic: VkBasic.cpp
	g++ $(CFLAGS) -o VkBasic VkBasic.cpp $(LDFLAGS) 

.PHONY: run clean

run: VkBasic
	./VkBasic

clean:
	rm -f VkBasic