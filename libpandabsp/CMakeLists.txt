file (GLOB SRCS "src/*.cpp")
file (GLOB HEADERS "include/*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type.")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

set(PANDA3D_DIR ../../panda3d/built_x64 CACHE PATH "Path to panda3d root dir.")

include_directories(./include ${PANDA3D_DIR}/include ../../panda3d/thirdparty/win-libs-vc14-x64/png/include ../../panda3d/thirdparty/win-libs-vc14-x64/jpeg/include)
link_directories(${PANDA3D_DIR}/lib)

add_definitions(-DBUILDING_LIBPANDABSP -DWITHIN_PANDA)

add_library(libpandabsp SHARED ${SRCS} ${HEADERS})

target_link_libraries(libpandabsp libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
						    libp3framework.lib
                            libpandaegg.lib)