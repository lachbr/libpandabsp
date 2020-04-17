project(p3rad)

file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

add_executable(p3rad ${SRCS} ${HEADERS})

target_compile_definitions(p3rad PRIVATE BUILDING_P3RAD NOMINMAX STDC_HEADERS HLRAD)

target_include_directories(p3rad PRIVATE ./ ${INCPANDA} ./../common ./../../libpandabsp ./../keyvalue-parser)
target_link_directories(p3rad PRIVATE ${LIBPANDA})

bsp_setup_target_exe(p3rad)

target_link_libraries(p3rad PRIVATE
					  libpanda.lib
					  libpandaexpress.lib
					  libp3dtool.lib
					  libp3dtoolconfig.lib
                      bsp_common
                      libpandabsp
                      vifparser)
