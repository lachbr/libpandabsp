file (GLOB SRCS "src/*.cpp")
file (GLOB HEADERS "include/*.h")
file (GLOB KD_SRCS "kdtree/*.cpp")
file (GLOB KD_HDRS "kdtree/*.h")

file (GLOB CM_SRCS "../../panda_hl_bsp_tools/common/*.cpp")
file (GLOB CM_INC "../../panda_hl_bsp_tools/common/*.h")
file (GLOB SSE_SRCS "../../panda_hl_bsp_tools/common/mathlib/*.cpp")
file (GLOB SSE_INC "../../panda_hl_bsp_tools/common/mathlib/*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})
source_group("Source Files\\kdtree" FILES ${KD_SRCS})
source_group("Header Files\\kdtree" FILES ${KD_HDRS})
source_group("Header Files\\common" FILES ${CM_INC})
source_group("Source Files\\common" FILES ${CM_SRCS})
source_group("Source Files\\common\\mathlib" FILES ${SSE_SRCS})
source_group("Header Files\\common\\mathlib" FILES ${SSE_INC})

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type.")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

if (DEFINED LINK_ALL_STATIC)
    set(P3BUILT built_x64_static)
else()
    set(P3BUILT built_x64)
endif ()

set(P3BSP_PANDA3D_DIR ../../../cio/cio-panda3d/${P3BUILT})
set(EMBREE_DIR ../embree-3.5.0.x64.vc14.windows)

include_directories(./include ${P3BSP_PANDA3D_DIR}/include ${P3BSP_PANDA3D_DIR}/python/include ../../../cio/cio-panda3d/thirdparty/win-libs-vc14-x64/png/include ../../../cio/cio-panda3d/thirdparty/win-libs-vc14-x64/jpeg/include ../../panda_hl_bsp_tools/common ../../panda_hl_bsp_tools/template ../vifparser/include)
link_directories(${P3BSP_PANDA3D_DIR}/lib ${P3BSP_PANDA3D_DIR}/python/libs)
if (NOT DEFINED LINK_ALL_STATIC)
    link_directories(../vifparser/Release)
endif()
link_directories(${EMBREE_DIR}/lib)
include_directories(${EMBREE_DIR}/include)

add_definitions(-DNOMINMAX -DBUILDING_LIBPANDABSP -DWITHIN_PANDA -DCOMPILER_MSVC64 -DVERSION_64BIT -DDOUBLEVEC_T -DSYSTEM_WIN32 -DSTDC_HEADERS -DCIO)
if (DEFINED LINK_ALL_STATIC)
    set(LIB_TYPE STATIC)
    set(OUT_EXT .lib)
    add_definitions(-DLINK_ALL_STATIC)
else ()
    set(LIB_TYPE SHARED)
    set(OUT_EXT .pyd)
endif ()

add_library(libpandabsp ${LIB_TYPE} ${SRCS} ${HEADERS} ${CM_INC} ${CM_SRCS} ${SSE_INC} ${SSE_SRCS} ${KD_SRCS} ${KD_HDRS})

set(bsp_suff ${OUT_EXT})
set(bsp_out bsp)

set_target_properties(libpandabsp PROPERTIES SUFFIX ${bsp_suff})
set_target_properties(libpandabsp PROPERTIES OUTPUT_NAME ${bsp_out})

string(REGEX REPLACE "/" "\\\\" P3BSP_PANDA3D_DIR ${P3BSP_PANDA3D_DIR})

add_custom_command(TARGET libpandabsp PRE_BUILD COMMAND interrogate.bat)

if (DEFINED LINK_ALL_STATIC)
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}${bsp_suff} ${P3BSP_PANDA3D_DIR}\\lib\\${bsp_out}${bsp_suff})
else()
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}${bsp_suff} ${P3BSP_PANDA3D_DIR}\\panda3d\\${bsp_out}${bsp_suff})
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}${bsp_suff} ${P3BSP_PANDA3D_DIR}\\bin\\${bsp_out}${bsp_suff})
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}.pdb ${P3BSP_PANDA3D_DIR}\\panda3d\\${bsp_out}.pdb)
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}.pdb ${P3BSP_PANDA3D_DIR}\\bin\\${bsp_out}.pdb)
endif()

target_link_libraries(libpandabsp libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
						    libp3framework.lib
                            libpandaegg.lib
							libp3interrogatedb.lib
							libvif.lib
							python27.lib
                            embree3.lib
                            tbb.lib
                            tbbmalloc.lib)
                            
add_dependencies(libpandabsp vifparser)