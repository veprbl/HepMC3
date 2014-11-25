# - Locate HepMC library
# in a directory defined via  HEPMC2_ROOT_DIR or HEPMC2_DIR environment variable
# Defines:
#
#  HEPMC2_FOUND
#  HEPMC2_INCLUDE_DIR
#  HEPMC2_INCLUDE_DIRS (not cached)
#  HEPMC2_LIBRARIES
#  HEPMC2_FIO_LIBRARIES

find_path(HEPMC2_INCLUDE_DIR HepMC/GenEvent.h
          HINTS $ENV{HEPMC2_ROOT_DIR}/include ${HEPMC2_ROOT_DIR}/include
          $ENV{HEPMC2_DIR}/include ${HEPMC2_DIR}/include)

find_library(HEPMC2_LIBRARIES NAMES HepMC
             HINTS $ENV{HEPMC2_ROOT_DIR}/lib ${HEPMC2_ROOT_DIR}/lib
             HINTS $ENV{HEPMC2_DIR}/lib ${HEPMC2_DIR}/lib)

get_filename_component(HEPMC2_LIBRARY_DIR ${HEPMC2_LIBRARIES} PATH)
set(HEPMC2_FIO_LIBRARIES "-L${HEPMC2_LIBRARY_DIR} -lHepMCfio")

set(HEPMC2_INCLUDE_DIRS ${HEPMC2_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set HEPMC2_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HepMC DEFAULT_MSG HEPMC2_INCLUDE_DIR HEPMC2_LIBRARIES)

mark_as_advanced(HEPMC2_FOUND HEPMC2_INCLUDE_DIR HEPMC2_LIBRARIES)