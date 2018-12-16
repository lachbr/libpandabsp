file (GLOB SRCS "src/*.cpp")
file (GLOB HEADERS "include/*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type.")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

set(VIF_PANDA3D_DIR ../../../cio/cio-panda3d/built_x64 CACHE PATH "Path to panda3d root dir.")

add_definitions(-DBUILDING_VIFPARSER -DWITHIN_PANDA)

include_directories(./include ${VIF_PANDA3D_DIR}/include)
link_directories(${BSPT_PANDA3D_DIR}/lib)

add_library(vifparser SHARED ${SRCS} ${HEADERS})

target_link_libraries(vifparser
					  libpanda.lib
					  libpandaexpress.lib
					  libp3dtool.lib
					  libp3dtoolconfig.lib)

set_target_properties(vifparser PROPERTIES OUTPUT_NAME "libvif")