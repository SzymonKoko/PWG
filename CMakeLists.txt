cmake_minimum_required(VERSION 3.15)
project(PWG)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Set C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ASSET_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)

# Collect all source files
file(GLOB_RECURSE ALL_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
)

# Group source files in Visual Studio
source_group(TREE ${PROJECT_SOURCE_DIR}/src PREFIX "src" FILES ${ALL_SOURCES})

# Find packages
find_package(OpenGL REQUIRED)

# Create a static library target with the app source code


# Add GLAD to project
add_library(glad libs/glad/src/glad.c)

target_include_directories(glad PUBLIC 
"${PROJECT_SOURCE_DIR}/libs/glad/include" # Path to GLAD Header files
)  

# Add GLFW3 to project
add_subdirectory(libs/glfw)
add_subdirectory(libs/glm)

# Add game executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES} "src/main.cpp")

# Attach header files
target_include_directories(${PROJECT_NAME} PRIVATE
	${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs
)

# Attach libraries
target_link_libraries(${PROJECT_NAME}
	PRIVATE 
		glad
		glfw
		glm
		OpenGL::GL
)

# Configure output directories for game
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Set the startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Copy assets and configs for the game
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/assets ${ASSET_OUTPUT_DIR})



