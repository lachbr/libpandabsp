file (GLOB SRCS "*.cpp")
file (GLOB HDRS "*.h")

source_group("Header Files" FILES ${HDRS})
source_group("Source Files" FILES ${SRCS})

add_executable(dedicated ${SRCS} ${HDRS})

target_include_directories(dedicated PRIVATE ./ ./../shared ./../server ${INCPANDA} ./../../tools/common
				    ./../../tools/keyvalue-parser ${INCBULLET} ${INCGNS} ${INCFREETYPE} ./../../libpandabsp ${INCBULLET})
target_link_directories(dedicated PRIVATE ${LIBPANDA})

target_compile_definitions(dedicated PRIVATE NOMINMAX BUILDING_DEDICATED STDC_HEADERS CIO DEDICATED)

bsp_setup_target_exe(dedicated)

target_link_libraries(dedicated PRIVATE libpanda.lib
                            libpandaexpress.lib
					        libp3dtool.lib
					        libp3dtoolconfig.lib
					        libp3direct.lib
							libpandabullet.lib
							server_dll)

add_dependencies(dedicated server_dll)
