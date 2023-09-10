# Try to find libjpeg library.
# It can be used as :
#  
# find_package(LibJpeg REQUIRED)
# target_link_libraries(program LibJpeg::common)
#

set(LibJpeg_FOUND OFF)
set(LibJpeg_COMMON_FOUND OFF)

find_library(LibJpeg_COMMON_LIBRARY NAMES jpeg)
find_path(LibJpeg_COMMON_INCLUDE_DIRS jpeglib.h)

if (LibJpeg_COMMON_LIBRARY AND LibJpeg_COMMON_INCLUDE_DIRS)
  set(LibJpeg_FOUND ON)
  set(LibJpeg_COMMON_FOUND ON)

  if (NOT TARGET "LibJpeg::common")

    add_library("LibJpeg::common" UNKNOWN IMPORTED)
    set_target_properties("LibJpeg::common"
      PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LibJpeg_COMMON_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${LibJpeg_COMMON_LIBRARY}"
    )

    get_filename_component(LibJpeg_LIB_DIR "${LibJpeg_COMMON_LIBRARY}" DIRECTORY CACHE)

  endif()

endif()

set(_LibJpeg_MISSING_COMPONENTS "")
foreach(COMPONENT ${LibJpeg_FIND_COMPONENTS})
  string(TOUPPER ${COMPONENT} COMPONENT)

  if(NOT LibJpeg_${COMPONENT}_FOUND)
    string(TOLOWER ${COMPONENT} COMPONENT)
    list(APPEND _LibJpeg_MISSING_COMPONENTS ${COMPONENT})
  endif()
endforeach()

if (_LibJpeg_MISSING_COMPONENTS)
  set(LibJpeg_FOUND OFF)

  if (LibJpeg_FIND_REQUIRED)
    message(SEND_ERROR "Unable to find the requested LibJpeg libraries.\n")
  else()
    message(STATUS "Unable to find the requested but not required LibJpeg libraries.\n")
  endif()

endif()


if(LibJpeg_FOUND)
  set(LibJpeg_LIBRARIES ${LibJpeg_COMMON_LIBRARY})
  set(LibJpeg_INCLUDE_DIRS ${LibJpeg_COMMON_INCLUDE_DIRS})
  
  message(STATUS "Found LibJpeg include: ${LibJpeg_INCLUDE_DIRS}")
  message(STATUS "Found LibJpeg: ${LibJpeg_LIBRARIES}")
endif()
