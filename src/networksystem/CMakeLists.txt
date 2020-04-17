file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

add_library(networksystem ${LIB_TYPE} ${SRCS} ${HEADERS})# interrogate.bat)

target_include_directories(networksystem PRIVATE
	./
	${INCPANDA}
	${INCGNS}
	${INCPANDA_PY}
)
target_link_directories(networksystem PRIVATE
	${LIBPANDA}
	${LIBPANDA_PY}
)

target_compile_definitions(networksystem PRIVATE BUILDING_NETWORKSYSTEM)
if (DEFINED LINK_ALL_STATIC)
    set(OUT_EXT .lib)
else ()
    set(OUT_EXT .pyd)
endif ()

set_target_properties(networksystem PROPERTIES SUFFIX ${OUT_EXT})

if (NOT SET_NDEBUG)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
    string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

add_custom_command(TARGET networksystem PRE_BUILD COMMAND interrogate.bat)

if (DEFINED LINK_ALL_STATIC)
	install(TARGETS networksystem LIBRARY DESTINATION lib)
else()
	install(TARGETS networksystem LIBRARY DESTINATION bin)
endif()

target_link_libraries(networksystem 
							libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
                            libpandaegg.lib
							libp3interrogatedb.lib
							python27.lib
							${LIBGNS})
