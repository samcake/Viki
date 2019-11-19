project(viki)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11 " )
set(EXECUTABLE_OUTPUT_PATH ./${CMAKE_BUILD_TYPE})



add_executable(${PROJECT_NAME} viki.cpp)
add_subdirectory(externals/volk volk)
add_subdirectory(externals/glfw glfw)


target_link_libraries(${PROJECT_NAME} PRIVATE volk_headers glfw)

#target_link_libraries (${PROJECT_NAME} glfw)


find_package(Vulkan REQUIRED)
#find_package(glfw3 3.3 REQUIRED)
#
if (VULKAN_FOUND)
   message(STATUS "Found Vulkan, Including and Linking now")
    include_directories(${Vulkan_INCLUDE_DIRS})   
    set(GLSL_VALIDATOR "${Vulkan_INCLUDE_DIRS}/../bin/glslangValidator.exe")

#else()
#    set(GLSL_VALIDATOR "/Users/sam/Dev/vulkansdk-macos-1.1.121.0/macOS/bin/glslangValidator")
endif (VULKAN_FOUND)

message(STATUS "${GLSL_VALIDATOR}")

file(GLOB_RECURSE GLSL_SOURCE_FILES_VERT
    "shaders/*.vert.glsl*"
    )

file(GLOB_RECURSE GLSL_SOURCE_FILES_FRAG
    "shaders/*.frag.glsl*"
    )

set (SPIRV_DIR "${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/shaders")

foreach(GLSL ${GLSL_SOURCE_FILES_VERT})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${SPIRV_DIR}/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SPIRV_DIR}"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
foreach(GLSL ${GLSL_SOURCE_FILES_FRAG})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${SPIRV_DIR}/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SPIRV_DIR}"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_dependencies(viki Shaders)

