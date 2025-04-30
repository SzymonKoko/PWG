cmake_minimum_required(VERSION 3.14)
project(PWG)

# Ustaw C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Określ katalogi źródeł
set(SOURCES
    src/Application.cpp
	src/window.cpp
	src/main.cpp
)

set(HEADERS
    src/window.h
	src/Application.h
)

# GLAD
add_library(glad
    libs/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    libs/glad/include  # Ścieżka do nagłówków GLAD
)

# Dodaj plik wykonywalny
add_executable(PWG ${SOURCES} ${HEADERS})

# Dołącz katalogi nagłówków
target_include_directories(PWG PRIVATE
    include
    libs/glad/include
    libs/glm
    libs/glfw/include
)

# GLFW + OpenGL
find_package(OpenGL REQUIRED)

# Dodanie bibliotek GLAD i GLFW (zakładam, że masz źródła)
add_subdirectory(libs/glfw)


# Podłącz biblioteki
target_link_libraries(PWG
    glad
    glfw
    OpenGL::GL
)

