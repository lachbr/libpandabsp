project(p3vis)

file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

add_executable(p3vis ${SRCS} ${HEADERS})

target_compile_definitions(p3vis PRIVATE BUILDING_P3VIS NOMINMAX STDC_HEADERS HLVIS)

bsp_setup_target_exe(p3vis)

target_include_directories(p3vis PRIVATE ./ ${INCPANDA} ./../common)
target_link_directories(p3vis PRIVATE ${LIBPANDA})

target_link_libraries(p3vis PRIVATE
					  libpanda.lib
					  libpandaexpress.lib
					  libp3dtool.lib
					  libp3dtoolconfig.lib
                      bsp_common)
