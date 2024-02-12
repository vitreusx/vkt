cmake_minimum_required(VERSION 3.18)
project(vkt LANGUAGES C CXX)

add_subdirectory(ext/tinyobjloader)
add_subdirectory(ext/spdlog)
add_subdirectory(ext/glm)
add_subdirectory(ext/stb)

set(TARGET vkt)
add_library(${TARGET})
set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)

file(GLOB SourceFiles CONFIGURE_DEPENDS
    src/${TARGET}/**.cc include/${TARGET}/**.h)
target_sources(${TARGET} PRIVATE ${SourceFiles})

target_include_directories(${TARGET}
    PUBLIC include)

target_compile_options(${TARGET}
    PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O2>)

target_compile_features(${TARGET} PUBLIC cxx_std_20)

find_package(glfw3 REQUIRED)
target_link_libraries(${TARGET} PUBLIC glfw)

find_package(Vulkan COMPONENTS glslc REQUIRED)
target_link_libraries(${TARGET} PUBLIC Vulkan::Vulkan)

set(TARGET vkx)
add_library(${TARGET})
set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)

file(GLOB SourceFiles CONFIGURE_DEPENDS
    src/${TARGET}/**.cc include/${TARGET}/**.h)
target_sources(${TARGET} PRIVATE ${SourceFiles})

target_include_directories(${TARGET}
    PUBLIC include)

target_compile_options(${TARGET}
    PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O2>)

target_link_libraries(${TARGET} PUBLIC vkt tinyobjloader spdlog::spdlog)

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

foreach(TARGET main stuff)
    add_executable(${TARGET} src/${TARGET}.cc)

    target_link_libraries(${TARGET} PRIVATE
        vkt vkx spdlog::spdlog glm::glm tinyobjloader stb)

    add_dependencies(${TARGET} shaders)

    add_custom_command(
        TARGET ${TARGET}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CompiledShadersDir}" "$<TARGET_FILE_DIR:${TARGET}>/shaders"
        COMMENT "Copying shaders next to executable")

    add_custom_command(
        TARGET ${TARGET}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_CURRENT_LIST_DIR}/assets" "$<TARGET_FILE_DIR:${TARGET}>/assets"
        COMMENT "Creating symlink to the assets."
    )
endforeach()
