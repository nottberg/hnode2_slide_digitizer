# Try to find libexif library.
# It can be used as :
#  
# find_package(LibExif REQUIRED)
# target_link_libraries(program LibExif::common)
#

set(LibExif_FOUND OFF)
set(LibExif_COMMON_FOUND OFF)

find_library(LibExif_COMMON_LIBRARY NAMES exif)
find_path(LibExif_COMMON_INCLUDE_DIRS libexif/exif-data.h)

if (LibExif_COMMON_LIBRARY AND LibExif_COMMON_INCLUDE_DIRS)
  set(LibExif_FOUND ON)
  set(LibExif_COMMON_FOUND ON)

  if (NOT TARGET "LibExif::common")

    add_library("LibExif::common" UNKNOWN IMPORTED)
    set_target_properties("LibExif::common"
      PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LibExif_COMMON_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${LibExif_COMMON_LIBRARY}"
    )

    get_filename_component(LibExif_LIB_DIR "${LibExif_COMMON_LIBRARY}" DIRECTORY CACHE)

  endif()

endif()

set(_LibExif_MISSING_COMPONENTS "")
foreach(COMPONENT ${LibExif_FIND_COMPONENTS})
  string(TOUPPER ${COMPONENT} COMPONENT)

  if(NOT LibExif_${COMPONENT}_FOUND)
    string(TOLOWER ${COMPONENT} COMPONENT)
    list(APPEND _LibExif_MISSING_COMPONENTS ${COMPONENT})
  endif()
endforeach()

if (_LibExif_MISSING_COMPONENTS)
  set(LibExif_FOUND OFF)

  if (LibExif_FIND_REQUIRED)
    message(SEND_ERROR "Unable to find the requested LibExif libraries.\n")
  else()
    message(STATUS "Unable to find the requested but not required LibExif libraries.\n")
  endif()

endif()


if(LibExif_FOUND)
  set(LibExif_LIBRARIES ${LibExif_COMMON_LIBRARY})
  set(LibExif_INCLUDE_DIRS ${LibExif_COMMON_INCLUDE_DIRS})
  
  message(STATUS "Found LibExif include: ${LibExif_INCLUDE_DIRS}")
  message(STATUS "Found LibExif: ${LibExif_LIBRARIES}")
endif()
