file (GLOB SRCS "*.cpp")
file (GLOB HDRS "*.h")
file (GLOB UI_SRCS "ui/*.cpp")
file (GLOB UI_HDRS "ui/*.h")
file (GLOB SHARED_SRCS "../shared/*.cpp")
file (GLOB SHARED_HDRS "../shared/*.h")

source_group("Header Files" FILES ${HDRS})
source_group("Source Files" FILES ${SRCS})
source_group("Header Files\\shared" FILES ${SHARED_HDRS})
source_group("Source Files\\shared" FILES ${SHARED_SRCS})
source_group("Header Files\\ui" FILES ${UI_HDRS})
source_group("Source Files\\ui" FILES ${UI_SRCS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4275")

add_executable(client ${SRCS} ${HDRS} ${UI_SRCS} ${UI_HDRS} ${SHARED_SRCS} ${SHARED_HDRS})

target_include_directories(client PRIVATE ./ ./../shared ${INCPANDA} ./../../tools/common
				    ./../../tools/keyvalue-parser ${INCBULLET} ${INCGNS} ${INCFREETYPE} ./../../libpandabsp)
target_link_directories(client PRIVATE ${LIBPANDA})

target_compile_definitions(client PRIVATE NOMINMAX BUILDING_CLIENT_DLL STDC_HEADERS CIO CLIENT_DLL)

bsp_setup_target_exe(client)

target_link_libraries(client PRIVATE libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
                            bsp_common
                            vifparser
							server_dll
							libpandabullet.lib
                            libpandabsp
                            ${LIBGNS})

add_dependencies(client libpandabsp bsp_common vifparser server_dll)
