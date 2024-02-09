cmake_minimum_required(VERSION 3.18)
project(vkt LANGUAGES C CXX)

set(TARGET example)
add_executable(${TARGET} src/main.cc)

target_compile_options(${TARGET}
    PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O2>)

target_compile_features(${TARGET} PRIVATE cxx_std_20)

find_package(glfw3 REQUIRED)
target_link_libraries(${TARGET} PRIVATE glfw)

find_package(Vulkan COMPONENTS glslc REQUIRED)
target_link_libraries(${TARGET} PRIVATE Vulkan::Vulkan)

add_subdirectory(ext/spdlog)
target_link_libraries(${TARGET} PRIVATE spdlog::spdlog)

function(add_spirv_shader INPUT_FILE OUTPUT_FILE)
    cmake_path(GET OUTPUT_FILE PARENT_PATH OUTPUT_PARENT)
    add_custom_command(
        OUTPUT "${OUTPUT_FILE}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_PARENT}"
        COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${INPUT_FILE}" -o "${OUTPUT_FILE}"
        MAIN_DEPENDENCY "${INPUT_FILE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endfunction()

set(ShadersDir "${CMAKE_CURRENT_LIST_DIR}/shaders")

file(GLOB_RECURSE ShaderFiles
    CONFIGURE_DEPENDS
    RELATIVE "${ShadersDir}"
    "${ShadersDir}/**")

set(CompiledShaders)

set(CompiledShadersDir "${CMAKE_CURRENT_BINARY_DIR}/shaders")

foreach(ShaderFile IN LISTS ShaderFiles)
    set(CompiledShader "${CompiledShadersDir}/${ShaderFile}.spv")
    add_spirv_shader("${ShadersDir}/${ShaderFile}" "${CompiledShader}")
    list(APPEND CompiledShaders "${CompiledShader}")
endforeach()

add_custom_target(shaders DEPENDS "${CompiledShaders}")
add_dependencies(${TARGET} shaders)

add_custom_command(
    TARGET ${TARGET}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CompiledShadersDir}" "$<TARGET_FILE_DIR:${TARGET}>/shaders"
    COMMENT "Copying shaders next to executable")