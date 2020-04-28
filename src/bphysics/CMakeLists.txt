set(INCPHYSX C:/Users/brian/OneDrive/Documents/Projects/libpandabsp/src/dependencies/PhysX/physx/include CACHE PATH "Path to PhysX include directory.")
set(INCPHYSX_SHARED C:/Users/brian/OneDrive/Documents/Projects/libpandabsp/src/dependencies/PhysX/pxshared/include CACHE PATH "Path to PhysX shared include directory.")
set(LIBPHYSX C:/Users/brian/OneDrive/Documents/Projects/libpandabsp/src/dependencies/PhysX/physx/bin/win.x86_64.vc142.md/release CACHE PATH "Path to PhysX lib directory.")

file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

add_library(bphysics ${LIB_TYPE} ${SRCS} ${HEADERS})

target_include_directories(bphysics PRIVATE
	./
	${INCPANDA}
	${INCPHYSX}
	${INCPHYSX_SHARED}
)

target_link_directories(bphysics PRIVATE
	${LIBPANDA}
	${LIBPHYSX}
)

target_compile_definitions(bphysics PRIVATE BUILDING_BPHYSICS)

bsp_setup_target_dll(bphysics)

target_link_libraries(bphysics 
					libpanda.lib
					libpandaexpress.lib
					libp3dtool.lib
					libp3dtoolconfig.lib
					PhysX_64.lib
					PhysXCommon_64.lib
					PhysXFoundation_64.lib
					PhysXExtensions_static_64.lib)
