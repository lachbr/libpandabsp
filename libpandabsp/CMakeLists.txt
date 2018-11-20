file (GLOB SRCS "src/*.cpp")
file (GLOB HEADERS "include/*.h")

file (GLOB CM_SRCS "../../panda_hl_bsp_tools/common/*.cpp")
file (GLOB CM_INC "../../panda_hl_bsp_tools/common/*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})
source_group("Header Files\\common" FILES ${CM_INC})
source_group("Source Files\\common" FILES ${CM_SRCS})

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type.")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

set(P3BSP_PANDA3D_DIR ../../../cio/cio-panda3d/built_x64 CACHE PATH "Path to panda3d root dir.")

include_directories(./include ${P3BSP_PANDA3D_DIR}/include ${P3BSP_PANDA3D_DIR}/python/include ../../panda3d/thirdparty/win-libs-vc14-x64/png/include ../../panda3d/thirdparty/win-libs-vc14-x64/jpeg/include ../../panda_hl_bsp_tools/common ../../panda_hl_bsp_tools/template ../vifparser/include)
link_directories(${P3BSP_PANDA3D_DIR}/lib ${P3BSP_PANDA3D_DIR}/python/libs ../vifparser/Release)

add_definitions(-DNOMINMAX -DBUILDING_LIBPANDABSP -DWITHIN_PANDA -DCOMPILER_MSVC64 -DVERSION_64BIT -DDOUBLEVEC_T -DSYSTEM_WIN32 -DSTDC_HEADERS -DCIO)

add_library(libpandabsp SHARED ${SRCS} ${HEADERS} ${CM_INC} ${CM_SRCS})

set_target_properties(libpandabsp PROPERTIES SUFFIX ".pyd")
set_target_properties(libpandabsp PROPERTIES OUTPUT_NAME "bsp")

target_link_libraries(libpandabsp libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
						    libp3framework.lib
                            libpandaegg.lib
							libp3interrogatedb.lib
							libvif.lib
							python27.lib)