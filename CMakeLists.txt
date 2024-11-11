cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0091 NEW)
project(vkgalaxy VERSION 1.0)

# add fsanitize
if(NOT WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined")
endif()

# set output dir
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# find source files:
file(GLOB_RECURSE vkgalaxy_src CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")
add_executable(${PROJECT_NAME} ${vkgalaxy_src})

# find include diretories and libraries
if(WIN32)
    find_package(GLFW3 3.3 REQUIRED)
else()
    find_package(glfw3 3.3 REQUIRED)
endif()

find_package(Vulkan REQUIRED)

include_directories("src/" ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} glfw ${Vulkan_LIBRARIES})

# set working directory:
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor") # set dpi awareness (only works on windows)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
else()
    set_property(TARGET ${PROJECT_NAME} PROPERTY WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
endif()

# compile shaders:
file(GLOB_RECURSE vkgalaxy_shader_src CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/assets/shaders/*")

if(WIN32)
    add_custom_target(vkgalaxy_shaders ALL DEPENDS ${vkgalaxy_shader_src} COMMAND "${CMAKE_SOURCE_DIR}/shader_compile.bat")
else()
    add_custom_target(vkgalaxy_shaders ALL DEPENDS ${vkgalaxy_shader_src} COMMAND sh "${CMAKE_SOURCE_DIR}/shader_compile.sh")
endif()
