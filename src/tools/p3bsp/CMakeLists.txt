project(p3bsp)

file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

add_executable(p3bsp ${SRCS} ${HEADERS})

target_compile_definitions(p3bsp PRIVATE BUILDING_P3BSP NOMINMAX STDC_HEADERS HLBSP)

target_include_directories(p3bsp PRIVATE ./ ${INCPANDA} ./../common)
target_link_directories(p3bsp PRIVATE ${LIBPANDA})

bsp_setup_target_exe(p3bsp)

target_link_libraries(p3bsp
					  libpanda.lib
					  libpandaexpress.lib
					  libp3dtool.lib
					  libp3dtoolconfig.lib
                      bsp_common)
