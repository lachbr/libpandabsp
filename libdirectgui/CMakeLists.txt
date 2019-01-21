file (GLOB SRCS "src/*.cpp")
file (GLOB HEADERS "include/*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type.")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

set(LIBDIRECTGUI_PANDA3D_DIR ../../panda3d/built_x64)

include_directories(./include ${LIBDIRECTGUI_PANDA3D_DIR}/include ../libppcore/include)
link_directories(${LIBDIRECTGUI_PANDA3D_DIR}/lib ../libppcore/Release)

add_definitions(-DBUILDING_LIBDIRECTGUI -DWITHIN_PANDA)

add_library(libdirectgui SHARED ${SRCS} ${HEADERS})

target_link_libraries(libdirectgui libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
						    libp3framework.lib
							libppcore.lib)

add_dependencies(libdirectgui libppcore)