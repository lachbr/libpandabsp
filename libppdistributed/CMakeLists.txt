file (GLOB SRCS "src/*.cpp")
file (GLOB HEADERS "include/*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type.")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

set(PANDA3D_DIR ../../panda3d/built_x64 CACHE PATH "Path to panda3d root dir.")
set(PPCORE_INCLUDE_DIR ../libppcore/include CACHE PATH "Path to libppcore include dir.")
set(PPCORE_LIB_DIR ../libppcore/Release CACHE PATH "Path to libppcore lib dir.")

include_directories(./include ${PANDA3D_DIR}/include ${PANDA3D_DIR}/python/include
                    ${PPCORE_INCLUDE_DIR})
link_directories(${PANDA3D_DIR}/lib ${PANDA3D_DIR}/python/libs
                 ${PPCORE_LIB_DIR})

add_definitions(-DBUILDING_PPDISTRIBUTED -DWITHIN_PANDA)

add_library(libppdistributed SHARED ${SRCS} ${HEADERS})

target_link_libraries(libppdistributed libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
						    libp3framework.lib
							libppcore.lib)

add_dependencies(libppdistributed libppcore)