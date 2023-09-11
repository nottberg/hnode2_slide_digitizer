# Try to find libcamera library.
# It can be used as :
#  
# find_package(LibCamera REQUIRED)
# target_link_libraries(program ${LibCamera_LIBRARIES})
#

set(LibCamera_FOUND OFF)

find_library(LibCamera_BASE_LIBRARY NAMES camera-base)
find_library(LibCamera_COMMON_LIBRARY NAMES camera)
find_path(LibCamera_COMMON_INCLUDE_DIRS libcamera/camera_manager.h PATH_SUFFIXES libcamera)

if (LibCamera_COMMON_LIBRARY AND LibCamera_BASE_LIBRARY AND LibCamera_COMMON_INCLUDE_DIRS)
  set(LibCamera_FOUND ON)
endif()

if(LibCamera_FOUND)
  set(LibCamera_LIBRARIES ${LibCamera_COMMON_LIBRARY})
  set(LibCamera_INCLUDE_DIRS ${LibCamera_COMMON_INCLUDE_DIRS})
  
  message(STATUS "Found LibCamera include: ${LibCamera_INCLUDE_DIRS}")
  message(STATUS "Found LibCamera: ${LibCamera_LIBRARIES}")
endif()
