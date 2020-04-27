set(KV_HEADERS
	keyvalues.h
	config_vifparser.h
)

set(KV_SOURCES
	keyvalues.cpp
)

add_library(vifparser ${LIB_TYPE} ${KV_SOURCES} ${KV_HEADERS})

target_compile_definitions(vifparser PRIVATE BUILDING_VIFPARSER)

target_include_directories(vifparser PRIVATE ./ ${INCPANDA})
target_link_directories(vifparser PRIVATE ${LIBPANDA})

bsp_setup_target_dll(vifparser)

target_link_libraries(vifparser
					 LINK_PRIVATE libpanda.lib
					 LINK_PRIVATE libpandaexpress.lib
					 LINK_PRIVATE libp3dtool.lib
					 LINK_PRIVATE libp3dtoolconfig.lib)
