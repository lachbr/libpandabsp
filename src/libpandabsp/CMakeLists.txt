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
if (PYTHON_VERSION)
	target_include_directories(libpandabsp PRIVATE
		${INCPANDA_PY}
	)
	target_link_directories(libpandabsp PRIVATE
		${LIBPANDA_PY}
	)
endif()

target_link_directories(libpandabsp PRIVATE
	${LIBPANDA}
	${LIBEMBREE}
	${LIBBULLET}
)

target_compile_definitions(libpandabsp PRIVATE NOMINMAX BUILDING_LIBPANDABSP STDC_HEADERS CIO)
if (DEFINED LINK_ALL_STATIC)
    set(OUT_EXT .lib)
else ()
    if (PYTHON_VERSION)
		set(OUT_EXT .pyd)
    else()
        set(OUT_EXT .dll)
    endif()
endif ()

set(bsp_suff ${OUT_EXT})
set(bsp_out libpandabsp)

set_target_properties(libpandabsp PROPERTIES SUFFIX ${bsp_suff})
set_target_properties(libpandabsp PROPERTIES OUTPUT_NAME ${bsp_out})

string(REGEX REPLACE "/" "\\\\" BSP_WIN_PANDA3D_DIR ${PANDA})

if (NOT SET_NDEBUG)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
	string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
	string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

if (PYTHON_VERSION)
    add_custom_command(TARGET libpandabsp PRE_BUILD COMMAND interrogate.bat)
endif()

if (DEFINED LINK_ALL_STATIC)
	install(TARGETS libpandabsp LIBRARY DESTINATION lib)
else()
	install(TARGETS libpandabsp LIBRARY DESTINATION bin)
endif()

target_link_libraries(libpandabsp LINK_PRIVATE libpanda.lib
                            LINK_PRIVATE libpandaexpress.lib
					        LINK_PRIVATE libp3dtool.lib
					        LINK_PRIVATE libp3dtoolconfig.lib
					        LINK_PRIVATE libp3direct.lib
						    LINK_PRIVATE libp3framework.lib
                            LINK_PRIVATE libpandaegg.lib
                            LINK_PRIVATE embree3.lib
                            LINK_PRIVATE tbb.lib
                            LINK_PRIVATE tbbmalloc.lib
							LINK_PRIVATE libpandagl.lib
                            LINK_PRIVATE bsp_common
							LINK_PRIVATE vifparser
							LINK_PRIVATE libpandabullet.lib)
							
if (PYTHON_VERSION)
	target_link_libraries(libpandabsp LINK_PRIVATE libp3interrogatedb.lib LINK_PRIVATE python27.lib)
endif()
                            
add_dependencies(libpandabsp vifparser bsp_common)
