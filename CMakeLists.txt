# CMakeLists.txt
cmake_minimum_required(VERSION 3.10.2)
project(nn6644)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 平台检测
if(ANDROID)
    # Android
elseif(WIN32)
    # Windows
elseif(APPLE)
    if(IOS)
        # iOS
    else()
        # macOS
    endif()
else()
    # Linux
endif()

set(LIBRETRO_DIR Core/libretro)
set(LIBRETRO_COMM_DIR Core/libretro-common)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -fpermissive")

message(STATUS "Searching for SDL2...")
find_package(SDL2 REQUIRED)
if(SDL2_FOUND)
    message(STATUS "SDL2 found: ${SDL2_LIBRARIES}")

else()
    message(FATAL_ERROR "SDL2 not found! Please install: sudo apt install libsdl2-dev")
endif()
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${LIBRETRO_COMM_DIR}/include
    ${LIBRETRO_COMM_DIR}
    ${LIBRETRO_DIR}
)
add_subdirectory(Core)
#add_subdirectory(Common)
#add_subdirectory(UI)
#add_subdirectory(GPU)
#add_subdirectory(SDL)
add_executable(nn6644 SDL/SDLMain.cpp SDL/glad.c)

# 基础链接（即使子库不存在也要能编译）
target_link_libraries(nn6644
    ${SDL2_LIBRARIES}
    nn6644-Core
    pthread
    dl
    rt
    asound
)
#target_link_libraries(nn6644 Common Core UI GPU SDL)


