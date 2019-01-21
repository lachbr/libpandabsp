file (GLOB SRCS "src/*.cpp")
file (GLOB HEADERS "include/*.h")

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SRCS})

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type.")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

if (DEFINED LINK_ALL_STATIC)
    set(P3BUILT built_x64_static)
else()
    set(P3BUILT built_x64)
endif ()
set(VIF_PANDA3D_DIR ..\\..\\..\\cio\\cio-panda3d\\${P3BUILT})

add_definitions(-DBUILDING_VIFPARSER -DWITHIN_PANDA)
if (DEFINED LINK_ALL_STATIC)
    set(LIB_TYPE STATIC)
    add_definitions(-DLINK_ALL_STATIC)
else ()
    set(LIB_TYPE SHARED)
endif ()

include_directories(./include ${VIF_PANDA3D_DIR}/include)
link_directories(${VIF_PANDA3D_DIR}/lib)

add_library(vifparser ${LIB_TYPE} ${SRCS} ${HEADERS})

if (DEFINED LINK_ALL_STATIC)
    add_custom_command(TARGET vifparser POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\libvif.lib ${VIF_PANDA3D_DIR}\\lib\\libvif.lib)
else()
    add_custom_command(TARGET vifparser POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\libvif.dll ${VIF_PANDA3D_DIR}\\bin\\libvif.dll)
    add_custom_command(TARGET vifparser POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\libvif.pdb ${VIF_PANDA3D_DIR}\\bin\\libvif.pdb)
endif()

target_link_libraries(vifparser
					  libpanda.lib
					  libpandaexpress.lib
					  libp3dtool.lib
					  libp3dtoolconfig.lib)

set_target_properties(vifparser PROPERTIES OUTPUT_NAME "libvif")