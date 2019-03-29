cmake_minimum_required(VERSION 2.8)

SET(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib")
SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")

if(WIN32)
	set(TOMGINE_BUILD_PATH_TO_GLFW_PATH PATH_TO_GLFW CACHE PATH "Path to GLFW library")
	set(TOMGINE_BUILD_PATH_TO_BOOST PATH_TO_BOOST CACHE PATH "Path to boost library")
	set(TOMGINE_BUILD_PATH_TO_GLEW PATH_TO_GLEW CACHE PATH "Path to glew library")
	include_directories(${TOMGINE_BUILD_PATH_TO_GLFW_PATH}/include)
	link_directories(${TOMGINE_BUILD_PATH_TO_GLFW_PATH}/build/src/Release)
	include_directories(${TOMGINE_BUILD_PATH_TO_BOOST})
	link_directories(${TOMGINE_BUILD_PATH_TO_BOOST}/stage/lib)
	include_directories(${TOMGINE_BUILD_PATH_TO_GLEW}/include)
	link_directories(${TOMGINE_BUILD_PATH_TO_GLEW}/lib/Release/x64)
else(WIN32)
	add_definitions("-Wall -Wl,--no-undefined -fno-strict-aliasing -pthread")
endif(WIN32)

include_directories(.)

include(opencv.cmake)

add_subdirectory(libs)
add_subdirectory(apps)

# uninstall target
configure_file(
"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
IMMEDIATE @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
