cmake_minimum_required(VERSION 3.15)
project(raylib_cimgui_lua)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(FetchContent)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Fetch raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 4.0
    GIT_SHALLOW TRUE
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(raygui)

# Fetch cimgui with a stable tag
# FetchContent_Declare(
#     cimgui
#     GIT_REPOSITORY https://github.com/cimgui/cimgui.git
#     GIT_TAG 1.92.1 # Matches ImGui 1.92.1 updated.
# )
# FetchContent_MakeAvailable(cimgui)

# Define cimgui sources
# set(IMGUI_SOURCES
#     ${cimgui_SOURCE_DIR}/cimgui.cpp
#     ${cimgui_SOURCE_DIR}/imgui/imgui.cpp
#     ${cimgui_SOURCE_DIR}/imgui/imgui_draw.cpp
#     ${cimgui_SOURCE_DIR}/imgui/imgui_widgets.cpp
#     ${cimgui_SOURCE_DIR}/imgui/imgui_demo.cpp
#     ${cimgui_SOURCE_DIR}/imgui/imgui_tables.cpp
#     ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
#     ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
# )

# Build cimgui as a static library with a custom name
# add_library(custom_cimgui STATIC ${IMGUI_SOURCES})

# Set include directories for custom_cimgui
# target_include_directories(custom_cimgui PUBLIC
#     ${cimgui_SOURCE_DIR}                            # cimgui
#     ${cimgui_SOURCE_DIR}/imgui                      # imgui
#     ${raylib_SOURCE_DIR}/src/external/glfw/include  # glfw
# )

# Set compile definitions for custom_cimgui (no CIMGUI_DEFINE_ENUMS_AND_STRUCTS)
# target_compile_definitions(custom_cimgui PUBLIC
#     IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
#     IMGUI_IMPL_API=extern\ \"C\"
# )

# Link custom_cimgui with GLFW and OpenGL
# target_link_libraries(custom_cimgui PUBLIC glfw OpenGL::GL) # needed library

#================================================
# Lua
#================================================
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lua)

# Lua doesn't have a CMake build, so we manually compile it
# List Lua source files (core and libraries, excluding lua.c and luac.c)
set(LUA_SRC
    ${lua_SOURCE_DIR}/lapi.c
    ${lua_SOURCE_DIR}/lauxlib.c
    ${lua_SOURCE_DIR}/lbaselib.c
    ${lua_SOURCE_DIR}/lcode.c
    ${lua_SOURCE_DIR}/lcorolib.c
    ${lua_SOURCE_DIR}/lctype.c
    ${lua_SOURCE_DIR}/ldblib.c
    ${lua_SOURCE_DIR}/ldebug.c
    ${lua_SOURCE_DIR}/ldo.c
    ${lua_SOURCE_DIR}/ldump.c
    ${lua_SOURCE_DIR}/lfunc.c
    ${lua_SOURCE_DIR}/lgc.c
    ${lua_SOURCE_DIR}/linit.c
    ${lua_SOURCE_DIR}/liolib.c
    ${lua_SOURCE_DIR}/llex.c
    ${lua_SOURCE_DIR}/lmathlib.c
    ${lua_SOURCE_DIR}/lmem.c
    ${lua_SOURCE_DIR}/loadlib.c
    ${lua_SOURCE_DIR}/lobject.c
    ${lua_SOURCE_DIR}/lopcodes.c
    ${lua_SOURCE_DIR}/loslib.c
    ${lua_SOURCE_DIR}/lparser.c
    ${lua_SOURCE_DIR}/lstate.c
    ${lua_SOURCE_DIR}/lstring.c
    ${lua_SOURCE_DIR}/lstrlib.c
    ${lua_SOURCE_DIR}/ltable.c
    ${lua_SOURCE_DIR}/ltablib.c
    ${lua_SOURCE_DIR}/ltm.c
    ${lua_SOURCE_DIR}/lundump.c
    ${lua_SOURCE_DIR}/lvm.c
    ${lua_SOURCE_DIR}/lzio.c
    ${lua_SOURCE_DIR}/lutf8lib.c
)

# Create a static Lua library
add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR})

#================================================
# ENET
#================================================
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/zpl-c/enet.git
    GIT_TAG v2.6.5
    GIT_SHALLOW TRUE
)
set(ENET_TEST  OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(enet)

FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG v4.1.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(flecs)

FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6
    GIT_PROGRESS   TRUE
)
set(CGLM_SHARED OFF CACHE BOOL "" FORCE)
set(CGLM_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cglm)

# FetchContent_Declare(
#     stb
#     GIT_REPOSITORY https://github.com/nothings/stb.git
#     GIT_TAG        master
#     EXCLUDE_FROM_ALL #this disable build?
#     GIT_SHALLOW TRUE
#     USES_TERMINAL_DOWNLOAD TRUE
# )
# FetchContent_MakeAvailable(stb)


#================================================
# Application
#================================================
set(EXPORT_APP ON) #ON OFF bool
if(${EXPORT_APP})
    message(STATUS "EXPORT APP")
    set(APP_NAME ril)
    set(SRC_FILES
        # src/module_cimgui.c                             # cimgui
        # src/module_lua.c                                # lua
        # src/module_enet.c                               # enet
        # src/module_raylib.c                             # raylib
        # src/drawcube.c                             # raylib
    )

    add_executable(${APP_NAME}
        icon.rc
        # ${SRC_FILES}
        # src/main.c


        # examples/flecs_id.c
        # examples/example_window.c
        # examples/rl_flecs_test01.c
        # examples/rl_flecs_test02.c
        # examples/rl_flecs_test03.c
        # examples/rl_flecs_test02.c
        # examples/raylib3d_flecs_transform_hierarchy07.c
        # examples/raylib3d_flecs_transform_hierarchy08.c
        examples/rl_flecs_camera3d.c 
        # examples/raylib_raycast_picking01.c 
        
        # src/example_raygui.c
        # src/example_flecs.c
        # src/raylib_flecs.c
        # src/raylib_raycast.c
    )

    # Link application with custom_cimgui
    target_link_libraries(${APP_NAME} PRIVATE 
        raylib                                          # raylib
        # custom_cimgui                                 # cimgui
        # lua                                             # lua
        flecs                                           # flecs
        cglm                                            # cglm
    )

    # Include directories for the application
    target_include_directories(${APP_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include                       # include
        ${lua_SOURCE_DIR}                                   # lua
        # ${cimgui_SOURCE_DIR}                              # imgui
        # ${cimgui_SOURCE_DIR}/imgui                        # imgui
        ${raylib_SOURCE_DIR}/src                            # raylib include
        ${raylib_SOURCE_DIR}/src/external/glfw/include      # glfw
        # ${enet_SOURCE_DIR}/include                        # enet
        # ${stb_SOURCE_DIR}                                 # stb
        ${raygui_SOURCE_DIR}/src                            # raygui
        ${cglm_SOURCE_DIR}/include                          # clgm
    )

    # Set compile definitions for the application
    # this will enable for cimgui windows and graphics defines
    target_compile_definitions(${APP_NAME} PRIVATE
        # CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1       # cimgui
        # IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1      # imgui
        # CIMGUI_USE_GLFW=1                       # cimgui glfw
        # CIMGUI_USE_OPENGL3=1                    # cimgui opengl 330
        # ENET_IMPLEMENTATION=1                   #enet
    )

    # Windows-specific settings
    if(WIN32)
        # ws2_32 # network
        target_link_libraries(${APP_NAME} PRIVATE ws2_32 gdi32 user32 shell32)
        # target_link_options(${APP_NAME} PRIVATE -static-libgcc)

        target_link_options(${APP_NAME} PRIVATE
            -static-libgcc
            -static-libstdc++  # Uncomment if C++ code is used
            -static           # Avoid full static linking to prevent issues with system libraries
        )
    endif()
endif()

set(EXPORT_FLECS_APP ON)

if(${EXPORT_FLECS_APP})
    message(STATUS "EXPORT FLECS APP")
    set(APP_FLECS_NAME rlf)

    add_executable(${APP_FLECS_NAME}
        src/main.c
    )

    # Link application with custom_cimgui
    target_link_libraries(${APP_FLECS_NAME} PRIVATE 
        raylib                                          # raylib
        # lua                                             # lua
        flecs                                           # flecs
        cglm                                            # cglm
    )

    # Include directories for the application
    target_include_directories(${APP_FLECS_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include                       # include
        ${lua_SOURCE_DIR}                                   # lua
        ${raylib_SOURCE_DIR}/src                            # raylib include
        ${raylib_SOURCE_DIR}/src/external/glfw/include      # glfw
        # ${enet_SOURCE_DIR}/include                        # enet
        # ${stb_SOURCE_DIR}                                 # stb
        ${raygui_SOURCE_DIR}/src                            # raygui
        ${cglm_SOURCE_DIR}/include                          # clgm
    )
    # Windows-specific settings
    if(WIN32)
        # ws2_32 # network
        target_link_libraries(${APP_FLECS_NAME} PRIVATE ws2_32 gdi32 user32 shell32)
        # target_link_options(${APP_NAME} PRIVATE -static-libgcc)

        target_link_options(${APP_FLECS_NAME} PRIVATE
            -static-libgcc
            -static-libstdc++  # Uncomment if C++ code is used
            -static           # Avoid full static linking to prevent issues with system libraries
        )
    endif()
endif()
