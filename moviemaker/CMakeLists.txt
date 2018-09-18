cmake_minimum_required(VERSION 3.1)

project(moviemaker-cpp)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)

# Search path for CMake include files.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Boost REQUIRED COMPONENTS system thread )
find_package(FFmpeg REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(RSVG REQUIRED)
find_package(x264 REQUIRED)
find_package(OpenCV REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Manual build type selection (for debugging purposes)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

# Enable C++ 11 support
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${FFMPEG_INCLUDE_DIRS})
include_directories(${GTK3_INCLUDE_DIRS})
include_directories(${RSVG_INCLUDE_DIR})
include_directories(${SWSCALE_INCLUDE_DIRS})
include_directories(${X264_INCLUDE_DIR})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories( include ${Boost_INCLUDE_DIRS})


set(SRC "src/test.cpp" "src/reader.cpp" "src/writer.cpp")

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} ${GTK3_LIBRARIES} ${X264_LIBRARIES} ${FFMPEG_LIBRARIES} ${SWSCALE_LIBRARIES} ${RSVG_LIBRARY} ${OpenCV_LIBS} ${Boost_LIBRARIES})