cmake_minimum_required(VERSION 3.15)
project(PWG)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Set C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#Fetching ImGui from github
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)

FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
	spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)

FetchContent_MakeAvailable(spdlog)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ASSET_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)

# Collect all source files
file(GLOB_RECURSE ALL_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
	"${PROJECT_SOURCE_DIR}/src/*.inl"
)

# Set all imgui sources
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Group source files in Visual Studio
source_group(TREE ${PROJECT_SOURCE_DIR}/src PREFIX "src" FILES ${ALL_SOURCES})

# Find packages
find_package(OpenGL REQUIRED)

# Add GLAD to project
add_library(glad libs/glad/src/glad.c)

target_include_directories(glad PUBLIC 
	"${PROJECT_SOURCE_DIR}/libs/glad/include" # Path to GLAD Header files
)  

# Add stb_image to project
add_library(stb_image STATIC libs/stb/stb_image.cpp)

target_include_directories(stb_image PUBLIC 
    "${PROJECT_SOURCE_DIR}/libs/stb" # Path to stb_image Header file
)

#Add entt to project
add_library(entt INTERFACE)
target_include_directories(entt INTERFACE 
    "${PROJECT_SOURCE_DIR}/libs/entt" # Path to entt Header file
) 

# Add GLFW3 to project
add_subdirectory(libs/glfw)
add_subdirectory(libs/glm)

# Add game executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${IMGUI_SOURCES} "src/main.cpp")

# Attach header files
target_include_directories(${PROJECT_NAME} PRIVATE
	${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs
	${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Attach libraries
target_link_libraries(${PROJECT_NAME}
	PRIVATE 
		glad
		glfw
		glm
		stb_image
		entt
		OpenGL::GL
		spdlog::spdlog
)

# Configure output directories for game
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Includes natvis to VS Project
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_VISUALIZER_FILES
    "${imgui_SOURCE_DIR}/misc/natvis/imgui.natvis"
)

# Set the startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Copy assets and configs for the game
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/assets ${ASSET_OUTPUT_DIR})



