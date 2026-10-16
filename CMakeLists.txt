cmake_minimum_required(VERSION 3.16)
project(OpenGLDemo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

file(GLOB CPP_FILES CONFIGURE_DEPENDS "labs/*.cpp")

# 2. Loop through each file and create an executable
foreach(SOURCE_FILE ${CPP_FILES})
    # Extract filename without extension (e.g. "donut" from "donut.cpp")
    get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)

    # Add executable named after the file
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    # Link libraries to the executable
    target_link_libraries(${TARGET_NAME} PRIVATE
        PkgConfig::EPOXY
        glfw
        OpenGL::GL
        glm::glm
    )
endforeach()
