set(TEST_TAUOLAPP_ROOT_DIR  "" ${TAUOLAPP_ROOT_DIR})
IF(TEST_TAUOLAPP_ROOT_DIR STREQUAL "")
IF(DEFINED ENV{TAUOLAPP_ROOT_DIR})
set(TAUOLAPP_ROOT_DIR  $ENV{TAUOLAPP_ROOT_DIR})
else()
set(TAUOLAPP_ROOT_DIR  "/usr")
endif()
endif()
find_path(TAUOLAPP_INCLUDE_DIR Tauola/Tauola.h
          HINTS $ENV{TAUOLAPP_ROOT_DIR}/include ${TAUOLAPP_ROOT_DIR}/include)

find_library(TAUOLAPP_CxxInterface_LIBRARY NAMES TauolaCxxInterface
             HINTS $ENV{TAUOLAPP_ROOT_DIR}/lib ${TAUOLAPP_ROOT_DIR}/lib
             HINTS $ENV{TAUOLAPP_ROOT_DIR}/lib64 ${TAUOLAPP_ROOT_DIR}/lib64
             )

find_library(TAUOLAPP_Fortran_LIBRARY NAMES TauolaFortran
             HINTS $ENV{TAUOLAPP_ROOT_DIR}/lib ${TAUOLAPP_ROOT_DIR}/lib
             HINTS $ENV{TAUOLAPP_ROOT_DIR}/lib64 ${TAUOLAPP_ROOT_DIR}/lib64
             )



set(TAUOLAPP_INCLUDE_DIRS ${TAUOLAPP_INCLUDE_DIR})
if (TAUOLAPP_LIBRARY)
set(TAUOLAPP_LIBRARIES ${TAUOLAPP_LIBRARY} )
else()
if (TAUOLAPP_CxxInterface_LIBRARY)
set(TAUOLAPP_LIBRARIES ${TAUOLAPP_CxxInterface_LIBRARY} ${TAUOLAPP_Fortran_LIBRARY})
endif()
endif()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TAUOLAPP DEFAULT_MSG TAUOLAPP_INCLUDE_DIR TAUOLAPP_LIBRARIES)

mark_as_advanced(TAUOLAPP_FOUND TAUOLAPP_INCLUDE_DIR TAUOLAPP_LIBRARIES)
