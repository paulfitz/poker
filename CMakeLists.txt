cmake_minimum_required(VERSION 2.6)

project(poker)

find_package(YARP REQUIRED)
include_directories(${YARP_INCLUDE_DIRS})

set(OS ${CMAKE_SOURCE_DIR}/yarp/src/libraries/os_services)
set(IMG ${CMAKE_SOURCE_DIR}/yarp/src/libraries/images)
set(POKE ${CMAKE_SOURCE_DIR}/yarp/src/libraries/poking)

add_definitions(-D__LINUX__)

# suppress string conversion warning
add_definitions(-Wno-write-strings)

include_directories(${OS}/include)
include_directories(${IMG}/include ${IMG}/fakeipl)
include_directories(${POKE}/include)

add_library(ancient_yarp_os
  ${OS}/common/YARPAll.cpp
  ${OS}/common/YARPRefCount.cpp
  )
target_link_libraries(ancient_yarp_os ${YARP_LIBRARIES})

add_library(ancient_yarp_sig
  ${IMG}/src/YARPImage.cpp
  ${IMG}/src/YARPImageFile.cpp
  ${IMG}/src/IntegralImageTool.cpp
  ${IMG}/src/graph.cpp
  ${IMG}/src/maxflow.cpp
  )
target_link_libraries(ancient_yarp_sig ancient_yarp_os)

add_library(ancient_yarp_poke
  ${POKE}/src/YARPVisualContact.cpp
  ${POKE}/src/YARPImageLabel.cpp
  )
target_link_libraries(ancient_yarp_poke ancient_yarp_sig)

add_executable(test_segmentation src/test_segmentation.cpp)
target_link_libraries(test_segmentation ancient_yarp_sig)
