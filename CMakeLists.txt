CMAKE_MINIMUM_REQUIRED(VERSION 3.6)
PROJECT( encgrid )

set(CMAKE_CXX_STANDARD 11)

# Add the libraries in the current directory to the include path
#============================================================================
FILE(GLOB LOCAL_LIBRARY_DIRS ./lib_*)
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/lib_enc_util" 
	"${PROJECT_SOURCE_DIR}/lib_geometry"
	"${PROJECT_SOURCE_DIR}/MOOSGeodesy"
	"${PROJECT_SOURCE_DIR}/lib_mbutil")


find_package(GDAL)
find_package( Boost COMPONENTS system filesystem REQUIRED )

#============================================================================
# List the subdirectories to build...
#============================================================================
ADD_SUBDIRECTORY(libMOOSGeodesy)
#ADD_SUBDIRECTORY(lib_ENC_util)
ADD_SUBDIRECTORY(lib_geometry)
ADD_SUBDIRECTORY(lib_mbutil)
ADD_SUBDIRECTORY(encgrid)

