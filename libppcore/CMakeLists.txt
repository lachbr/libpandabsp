file (GLOB SRCS "src/*.cpp")
file (GLOB HEADERS "include/*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type.")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

set(PANDA3D_DIR ../../panda3d/built_x64 CACHE PATH "Path to panda3d root dir.")

include_directories(./include ${PANDA3D_DIR}/include ${PANDA3D_DIR}/python/include)
link_directories(${PANDA3D_DIR}/lib ${PANDA3D_DIR}/python/libs)

add_definitions(-DBUILDING_PANDAPLUS -DWITHIN_PANDA)

add_library(libppcore SHARED ${SRCS} ${HEADERS})

target_link_libraries(libppcore libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
						    libp3framework.lib
                                                    libpandaphysics.lib)

add_dependencies(libppcore vifparser)