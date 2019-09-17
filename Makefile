

VkTest: VkTest.cpp
	g++ -o VkTest VkTest.cpp

.PHONY: test clean

test: VkTest
	./VkTest

clean:
	rm -f VkTest