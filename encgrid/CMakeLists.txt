

find_package( Boost COMPONENTS system filesystem REQUIRED )
find_package(GDAL)
#include_directories(${Boost_INCLUDE_DIRS} ${GDAL_INCLUDE_DIR} "libMOOSGeodesy/include/MOOS/libMOOSGeodesy/MOOSGeodesy.h" ".")
include_directories(${Boost_INCLUDE_DIRS} ${GDAL_INCLUDE_DIR} ".")

SET(SRC
    main.cpp
    gridinterp.cpp
    gridinterp.h
    geodesy.cpp
    geodesy.h
)



ADD_EXECUTABLE(encgrid ${SRC})

TARGET_LINK_LIBRARIES( encgrid
   ${GDAL_LIBRARY}
   #ENC_util
#   geometry
   ${Boost_SYSTEM_LIBRARY}
   ${Boost_FILESYSTEM_LIBRARY}
)
