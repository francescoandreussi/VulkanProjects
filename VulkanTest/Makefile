VULKAN_PATH = /usr/include/vulkan/
CFLAGS = -std=c++17 -I$(VULKAN_PATH)
LDFLAGS = -L$(VULKAN_PATH) `pkg-config --static --libs glfw3` -lvulkan

VkTest: VkTest.cpp
	g++ $(CFLAGS) -o VkTest VkTest.cpp $(LDFLAGS) 

.PHONY: test clean

test: VkTest
	./VkTest

clean:
	rm -f VkTest