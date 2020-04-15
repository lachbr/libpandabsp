set(KV_HEADERS
	keyvalues.h
	config_vifparser.h
)

set(KV_SOURCES
	keyvalues.cpp
)

source_group("Header Files" FILES ${KV_HEADERS})
source_group("Source Files" FILES ${KV_SOURCES})

add_library(vifparser ${LIB_TYPE} ${KV_SOURCES} ${KV_HEADERS})

target_compile_definitions(vifparser PRIVATE BUILDING_VIFPARSER)

target_include_directories(vifparser PRIVATE ./ ${INCPANDA})
target_link_directories(vifparser PRIVATE ${LIBPANDA})

bsp_setup_target(vifparser)

target_link_libraries(vifparser
					  libpanda.lib
					  libpandaexpress.lib
					  libp3dtool.lib
					  libp3dtoolconfig.lib)
