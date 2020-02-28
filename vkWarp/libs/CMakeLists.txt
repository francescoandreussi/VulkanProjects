add_library(libs mysqrt.cpp)
target_include_directories(libs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Allows installation of application in the program folder of the computer (needs admin privileges)
install(TARGETS libs DESTINATION lib)
install(FILES libs.h DESTINATION include)