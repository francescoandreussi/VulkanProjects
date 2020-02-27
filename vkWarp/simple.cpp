// A simple program that computes the square root of a number
#include <cmath>
#include <iostream>
#include <string>
#include "VkWarpConfig.h"

#ifdef USE_MYMATH
#   include "libs.h"
#endif

int main(int argc, char* argv[])
{
  if (argc < 2) {
      // report version
    std::cout << argv[0] << " Version " << vkWarp_VERSION_MAJOR << "." << vkWarp_VERSION_MINOR << std::endl;
    std::cout << "Usage: " << argv[0] << " number" << std::endl;
    return 1;
  }

  // convert input to double
  const double inputValue = std::stod(argv[1]);

  // calculate square root
#ifdef USE_MYMATH
    std::cout << "USE_MYMATH is ON" << std::endl;
    const double outputValue = mysqrt(inputValue);
#else
    std::cout << "USE_MYMATH is OFF" << std::endl;
    const double outputValue = sqrt(inputValue);
#endif
  std::cout << "The square root of " << inputValue << " is " << outputValue
            << std::endl;
  return 0;
}
