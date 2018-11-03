# Pull submodules
execute_process(COMMAND git submodule update --init --depth 1)

# GLM is header only
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# Libraries
add_subdirectory(libgl3w)
# Override glfw default options
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(glfw)

add_library(imgui STATIC "")
target_sources(imgui
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui/examples/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui/examples/imgui_impl_opengl3.cpp
)
target_include_directories(imgui
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/imgui
    ${CMAKE_CURRENT_LIST_DIR}/imgui/examples
)
target_link_libraries(imgui
    PUBLIC
    ${OPENGL_LIBRARIES}
    libgl3w
    glfw
)

add_library(librocket STATIC "")
target_sources(librocket
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/rocket/lib/device.c
    ${CMAKE_CURRENT_LIST_DIR}/rocket/lib/track.c
)
target_include_directories(librocket
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/rocket/lib
)
if(MSVC)
    target_link_libraries(librocket
        PRIVATE
        ws2_32.lib
    )
endif()

add_library(bass INTERFACE)
target_include_directories(bass
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/bass
)
if (MSVC)
    target_link_libraries(bass
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/bass/lib/bass.lib
    )
elseif (APPLE)
    target_link_libraries(bass
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/bass/lib/libbass.dylib
    )
else()
    target_link_libraries(bass
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/bass/lib/libbass.so
    )
endif()
