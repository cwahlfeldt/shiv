cmake_minimum_required(VERSION 3.10)
project(shiv LANGUAGES C)

find_library(FLECS_LIB
    NAMES flecs libflecs
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/flecs/build
)

find_library(SDL_LIB
    NAMES SDL3 libSDL3
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL/build
)

find_library(CGLM_LIB
    NAMES cglm libcglm
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm/build
)

# Include directories
include_directories(
    ${LIB_DIR}/SDL/include
    ${LIB_DIR}/cglm/include
    ${LIB_DIR}/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(shiv
    src/main.c
    src/renderer/renderer.c
    src/systems/depth_sampler.c
)

target_link_libraries(shiv
    ${SDL_LIB}
    ${CGLM_LIB}
    ${FLECS_LIB}
    m
)

target_include_directories(shiv
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/flecs/include
)

add_custom_command(TARGET shiv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:shiv>/assets
)

add_custom_command(TARGET shiv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:shiv>/shaders
)
