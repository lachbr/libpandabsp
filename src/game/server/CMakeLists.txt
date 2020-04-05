file (GLOB SRCS "*.cpp")
file (GLOB HDRS "*.h")
file (GLOB SHARED_SRCS "../shared/*.cpp")
file (GLOB SHARED_HDRS "../shared/*.h")
file (GLOB CIO_SRCS "cio/*.cpp")
file (GLOB CIO_HDRS "cio/*.h")
file (GLOB CIO_SHARED_HDRS "../shared/cio/*.h")
file (GLOB CIO_SHARED_SRCS "../shared/cio/*.cpp")

source_group("Header Files" FILES ${HDRS})
source_group("Source Files" FILES ${SRCS})
source_group("Header Files\\shared" FILES ${SHARED_HDRS})
source_group("Source Files\\shared" FILES ${SHARED_SRCS})
source_group("Header Files\\cio" FILES ${CIO_HDRS})
source_group("Source Files\\cio" FILES ${CIO_SRCS})
source_group("Header Files\\cio shared" FILES ${CIO_SHARED_HDRS})
source_group("Source Files\\cio shared" FILES ${CIO_SHARED_SRCS})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4275")

add_library(server_dll ${LIB_TYPE}
	${SRCS} ${HDRS}
	${SHARED_SRCS} ${SHARED_HDRS}
	${CIO_SRCS} ${CIO_HDRS}
	${CIO_SHARED_SRCS} ${CIO_SHARED_HDRS}
)

target_include_directories(server_dll PRIVATE ./ ./../shared ${INCPANDA} ./../../tools/common
				    ./../../tools/keyvalue-parser ${INCBULLET} ${INCGNS} ./../../libpandabsp ${INCEMBREE})
target_link_directories(server_dll PRIVATE ${LIBPANDA} ${LIBEMBREE})

target_compile_definitions(server_dll PRIVATE NOMINMAX BUILDING_SERVER_DLL STDC_HEADERS CIO SERVER_DLL)

bsp_setup_target(server_dll)

target_link_libraries(server_dll PRIVATE libpanda.lib
                            PRIVATE libpandaexpress.lib
					        PRIVATE libp3dtool.lib
					        PRIVATE libp3dtoolconfig.lib
					        PRIVATE libp3direct.lib
                            PRIVATE bsp_common
                            PRIVATE vifparser
							PRIVATE libpandabullet.lib
                            PRIVATE libpandabsp
                            ${LIBGNS})

add_dependencies(server_dll libpandabsp bsp_common vifparser)