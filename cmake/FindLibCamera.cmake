# Try to find libcamera library.
# It can be used as :
#  
# find_package(LibCamera REQUIRED)
# target_link_libraries(program LibCamera::common)
#

set(LibCamera_FOUND OFF)
set(LibCamera_COMMON_FOUND OFF)

find_library(LibCamera_COMMON_LIBRARY NAMES camera)
find_path(LibCamera_COMMON_INCLUDE_DIRS libcamera/camera_manager.h PATH_SUFFIXES libcamera)

if (LibCamera_COMMON_LIBRARY AND LibCamera_COMMON_INCLUDE_DIRS)
  set(LibCamera_FOUND ON)
  set(LibCamera_COMMON_FOUND ON)

  if (NOT TARGET "LibCamera::common")

    add_library("LibCamera::common" UNKNOWN IMPORTED)
    set_target_properties("LibCamera::common"
      PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LibCamera_COMMON_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${LibCamera_COMMON_LIBRARY}"
    )

    get_filename_component(LibCamera_LIB_DIR "${LibCamera_COMMON_LIBRARY}" DIRECTORY CACHE)

  endif()

endif()

set(_LibCamera_MISSING_COMPONENTS "")
foreach(COMPONENT ${LibCamera_FIND_COMPONENTS})
  string(TOUPPER ${COMPONENT} COMPONENT)

  if(NOT LibCamera_${COMPONENT}_FOUND)
    string(TOLOWER ${COMPONENT} COMPONENT)
    list(APPEND _LibCamera_MISSING_COMPONENTS ${COMPONENT})
  endif()
endforeach()

if (_LibCamera_MISSING_COMPONENTS)
  set(LibCamera_FOUND OFF)

  if (LibCamera_FIND_REQUIRED)
    message(SEND_ERROR "Unable to find the requested LibCamera libraries.\n")
  else()
    message(STATUS "Unable to find the requested but not required LibCamera libraries.\n")
  endif()

endif()


if(LibCamera_FOUND)
  set(LibCamera_LIBRARIES ${LibCamera_COMMON_LIBRARY})
  set(LibCamera_INCLUDE_DIRS ${LibCamera_COMMON_INCLUDE_DIRS})
  
  message(STATUS "Found LibCamera include: ${LibCamera_INCLUDE_DIRS}")
  message(STATUS "Found LibCamera: ${LibCamera_LIBRARIES}")
endif()
