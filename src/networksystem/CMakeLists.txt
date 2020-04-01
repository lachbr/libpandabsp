file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

add_library(networksystem ${LIB_TYPE} ${SRCS} ${HEADERS})# interrogate.bat)

target_include_directories(networksystem PRIVATE
	./
	${INCPANDA}
	${INCGNS}
)
target_link_directories(networksystem PRIVATE
	${LIBPANDA}
)

target_compile_definitions(networksystem PRIVATE BUILDING_NETWORKSYSTEM)
if (DEFINED LINK_ALL_STATIC)
    set(OUT_EXT .lib)
else ()
    set(OUT_EXT .pyd)
endif ()

set_target_properties(networksystem PROPERTIES SUFFIX ${OUT_EXT})

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
#string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
#string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

#add_custom_command(TARGET networksystem PRE_BUILD COMMAND interrogate.bat)

if (DEFINED LINK_ALL_STATIC)
    add_custom_command(TARGET networksystem POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\networksystem${OUT_EXT} ${LIBPANDA_BACK}\\networksystem${OUT_EXT})
else()
    add_custom_command(TARGET networksystem POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\networksystem${OUT_EXT} ${PANDA_BACK}\\panda3d\\networksystem${OUT_EXT})
    add_custom_command(TARGET networksystem POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\networksystem${OUT_EXT} ${PANDA_BACK}\\bin\\networksystem${OUT_EXT})
    add_custom_command(TARGET networksystem POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\networksystem.pdb ${PANDA_BACK}\\panda3d\\networksystem.pdb)
    add_custom_command(TARGET networksystem POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\networksystem.pdb ${PANDA_BACK}\\bin\\networksystem.pdb)
endif()

target_link_libraries(networksystem 
							libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
                            libpandaegg.lib
							${LIBGNS})
