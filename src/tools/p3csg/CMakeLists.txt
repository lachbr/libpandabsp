project(p3csg)

file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

add_executable(p3csg ${SRCS} ${HEADERS})

bsp_setup_target_exe(p3csg)

target_compile_definitions(p3csg PRIVATE BUILDING_P3CSG NOMINMAX STDC_HEADERS HLCSG)

target_include_directories(p3csg PRIVATE
	./ ${INCPANDA} ./../common ./../../libpandabsp ./../keyvalue-parser)
target_link_directories(p3csg PRIVATE ${LIBPANDA})

target_link_libraries(p3csg PRIVATE
					  libpanda.lib
					  libpandaexpress.lib
					  libp3dtool.lib
					  libp3dtoolconfig.lib
                      bsp_common
                      vifparser
                      libpandabsp)
