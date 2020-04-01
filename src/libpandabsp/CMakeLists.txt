file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")
file (GLOB KD_SRCS "kdtree/*.cpp")
file (GLOB KD_HDRS "kdtree/*.h")
file (GLOB DX11_SRCS "dxgsg11/*.cpp")
file (GLOB DX11_HDRS "dxgsg11/*.h")
file (GLOB PP_HDRS "postprocess/*.h")
file (GLOB PP_SRCS "postprocess/*.cpp")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})
source_group("Source Files\\kdtree" FILES ${KD_SRCS})
source_group("Header Files\\kdtree" FILES ${KD_HDRS})
source_group("Header Files\\dxgsg11" FILES ${DX11_HDRS})
source_group("Source Files\\dxgsg11" FILES ${DX11_SRCS})
source_group("Source Files\\postprocess" FILES ${PP_SRCS})
source_group("Header Files\\postprocess" FILES ${PP_HDRS})

add_library(libpandabsp ${LIB_TYPE} ${SRCS} ${HEADERS} ${KD_SRCS} ${KD_HDRS} ${PP_SRCS} ${PP_HDRS})# interrogate.bat)

target_include_directories(libpandabsp PRIVATE
	./
	${INCPANDA}
	./../tools/common
	./../tools/keyvalue-parser
	${INCBULLET}
	${INCEMBREE}
)
target_link_directories(libpandabsp PRIVATE
	${LIBPANDA}
	${LIBEMBREE}
	${LIBBULLET}
)

target_compile_definitions(libpandabsp PRIVATE NOMINMAX BUILDING_LIBPANDABSP STDC_HEADERS CIO)
if (DEFINED LINK_ALL_STATIC)
    set(OUT_EXT .lib)
else ()
    set(OUT_EXT .dll)
endif ()

set(bsp_suff ${OUT_EXT})
set(bsp_out libpandabsp)

set_target_properties(libpandabsp PROPERTIES SUFFIX ${bsp_suff})
set_target_properties(libpandabsp PROPERTIES OUTPUT_NAME ${bsp_out})

string(REGEX REPLACE "/" "\\\\" BSP_WIN_PANDA3D_DIR ${PANDA})

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
#string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
#string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

#add_custom_command(TARGET libpandabsp PRE_BUILD COMMAND interrogate.bat)

if (DEFINED LINK_ALL_STATIC)
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}${bsp_suff} ${LIBPANDA_BACK}\\${bsp_out}${bsp_suff})
else()
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}${bsp_suff} ${PANDA_BACK}\\panda3d\\${bsp_out}${bsp_suff})
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}${bsp_suff} ${PANDA_BACK}\\bin\\${bsp_out}${bsp_suff})
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}.pdb ${PANDA_BACK}\\panda3d\\${bsp_out}.pdb)
    add_custom_command(TARGET libpandabsp POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\${bsp_out}.pdb ${PANDA_BACK}\\bin\\${bsp_out}.pdb)
endif()

target_link_libraries(libpandabsp libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
						    libp3framework.lib
                            libpandaegg.lib
                            embree3.lib
                            tbb.lib
                            tbbmalloc.lib
							libpandagl.lib
                            bsp_common
							vifparser
							libpandabullet.lib)
                            
add_dependencies(libpandabsp vifparser bsp_common)
