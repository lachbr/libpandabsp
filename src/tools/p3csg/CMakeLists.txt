project(p3csg)

file (GLOB SRCS "*.cpp")
file (GLOB HEADERS "*.h")

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

set(PANDA3D_DIR ..\\..\\cio-panda3d\\${P3BUILT} CACHE PATH "Panda3D directory")
set(BSP_COMMON_INCLUDE ..\\common CACHE PATH "Common directory")
set(BSP_COMMON_LIB ..\\common\\Release\\bsp_common CACHE FILEPATH "Common lib file")
set(LIBPANDABSP_INCLUDE ..\\..\\libpandabsp\\include CACHE PATH "libpandabsp include directory")
set(LIBPANDABSP_LIB ..\\..\\libpandabsp\\Release\\bsp CACHE FILEPATH "libpandabsp lib file")
set(VIF_INCLUDE ..\\..\\keyvalue-parser\\include CACHE PATH "keyvalue-parser include directory")
set(VIF_LIB ..\\..\\keyvalue-parser\\Release\\libvif CACHE FILEPATH "keyvalue-parser lib file")

add_definitions(-DBUILDING_P3CSG -DWITHIN_PANDA -DNOMINMAX -DSTDC_HEADERS -DHLCSG)
if (DEFINED LINK_ALL_STATIC)
    add_definitions(-DLINK_ALL_STATIC)
endif ()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    add_definitions(-DVERSION_64BIT)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
    add_definitions(-DVERSION_32BIT)
endif()

include_directories(./ ${PANDA3D_DIR}/include ${BSP_COMMON_INCLUDE} ${LIBPANDABSP_INCLUDE} ${VIF_INCLUDE})
link_directories(${PANDA3D_DIR}/lib)

add_executable(p3csg ${SRCS} ${HEADERS})

STRING(REGEX REPLACE "/" "\\\\" WIN_PANDA3D_DIR ${PANDA3D_DIR})

add_custom_command(TARGET p3csg POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\p3csg.exe ${WIN_PANDA3D_DIR}\\bin\\p3csg.exe)
add_custom_command(TARGET p3csg POST_BUILD COMMAND copy /Y ${CMAKE_BUILD_TYPE}\\p3csg.pdb ${WIN_PANDA3D_DIR}\\bin\\p3csg.pdb)

target_link_libraries(p3csg
					  libpanda.lib
					  libpandaexpress.lib
					  libp3dtool.lib
					  libp3dtoolconfig.lib
                      ${BSP_COMMON_LIB}
                      ${LIBPANDABSP_LIB}
                      ${VIF_LIB})
