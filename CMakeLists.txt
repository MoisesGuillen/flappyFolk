cmake_minimum_required(VERSION 3.20)
project(flappyFolk)

set(CMAKE_CXX_STANDARD 26)

# 1. Point directly to your custom Frameworks folder
set(CMAKE_FRAMEWORK_PATH "$ENV{HOME}/Library/Frameworks")

# 2. Use find_library instead of find_package for macOS frameworks
find_library(SDL3_LIB NAMES SDL3 PATHS ${CMAKE_FRAMEWORK_PATH} REQUIRED)
find_library(SDL3_TTF_LIB NAMES SDL3_ttf PATHS ${CMAKE_FRAMEWORK_PATH} REQUIRED)

find_library(SDL3_IMAGE_LIB NAMES SDL3_image PATHS ${CMAKE_FRAMEWORK_PATH} REQUIRED)

find_library(SDL3_MIXER_LIB NAMES SDL3_mixer PATHS ${CMAKE_FRAMEWORK_PATH} REQUIRED)
add_executable(flappyBird main.cpp)

# 3. Link the found framework libraries to your target
target_link_libraries(flappyBird PRIVATE ${SDL3_LIB} ${SDL3_TTF_LIB}
        ${SDL3_IMAGE_LIB} ${SDL3_MIXER_LIB})