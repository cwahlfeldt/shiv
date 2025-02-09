cmake_minimum_required(VERSION 3.15)
project(shiv)

# Use pkg-config to find SDL3
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer
    ${LIB_DIR}/SDL/include
    ${LIB_DIR}/cglm/include
    ${LIB_DIR}/stb
    ${SDL3_INCLUDE_DIRS}
)

add_executable(shiv
    src/main.c
    src/renderer/renderer.c
    examples/depth_sampler.c
)

target_link_libraries(shiv
    ${SDL3_LIBRARIES}
    m    # math library
)

add_custom_command(TARGET shiv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:shiv>/assets
)

add_custom_command(TARGET shiv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:shiv>/shaders
)