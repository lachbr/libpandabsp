cmake_minimum_required (VERSION 2.8.12.2)

project(libpandabsp)

option(PYTHON_VERSION "Builds the libraries for use in Python. Set this is you're compiling for CIO." ON)
option(SET_NDEBUG "Defines NDEBUG in all projects. Set this for Panda3D optimize 4 builds." OFF)

if (PYTHON_VERSION)
	set(IS_GAME_BUILD 0)
else()
	set(IS_GAME_BUILD (IS_DIRECTORY "./game"))
endif()

# Setup paths to dependencies

set(PANDA C:/Users/brian/OneDrive/Documents/Projects/cio-panda3d/built_x64 CACHE PATH "Path to root Panda3D SDK directory.")
string(REGEX REPLACE "/" "\\\\" PANDA_BACK ${PANDA})
set(LIBPANDA ${PANDA}/lib)
set(LIBPANDA_BACK ${PANDA_BACK}\\lib)
set(INCPANDA ${PANDA}/include)
set(BINPANDA ${PANDA}/bin)
set(BINPANDA_BACK ${PANDA_BACK}\\bin)
set(LIBPANDA_PY ${PANDA}/python/libs)
set(INCPANDA_PY ${PANDA}/python/include)

set(EMBREE C:/Users/brian/OneDrive/Documents/Projects/embree-3.5.0.x64.vc14.windows CACHE PATH "Path to root Embree SDK directory.")
set(INCEMBREE ${EMBREE}/include)
set(LIBEMBREE ${EMBREE}/lib)

set(BULLET C:/Users/brian/OneDrive/Documents/Projects/cio-panda3d/thirdparty/win-libs-vc14-x64/bullet CACHE PATH "Path to thirdparty Bullet directory.")
set(INCBULLET ${BULLET}/include)
set(LIBBULLET ${BULLET}/lib)

set(INCGNS C:/Users/brian/OneDrive/Documents/Projects/libpandabsp/src/dependencies/GameNetworkingSockets/include CACHE PATH "Path to GameNetworkingSockets include directory.")
set(LIBGNS C:/Users/brian/OneDrive/Documents/Projects/libpandabsp/src/dependencies/GameNetworkingSockets/src/RelWithDebInfo/GameNetworkingSockets CACHE FILEPATH "Path to GameNetworkingSockets library.")

set(CMAKE_INSTALL_PREFIX ${PANDA})

if (IS_GAME_BUILD)
	set(INCFREETYPE C:/Users/brian/OneDrive/Documents/Projects/cio-panda3d/thirdparty/win-libs-vc14-x64/freetype/include/freetype2 CACHE PATH "Path to thirdparty FreeType include directory.")
endif()

add_compile_definitions(WITHIN_PANDA)

if (PYTHON_VERSION)
    add_compile_definitions(_PYTHON_VERSION)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    set(PLATFORM_BITS 64)
	add_compile_definitions(VERSION_64BIT)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
	set(PLATFORM_BITS 32)
    add_compile_definitions(VERSION_32BIT)
endif()

if (DEFINED LINK_ALL_STATIC)
    set(LIB_TYPE STATIC)
    add_compile_definitions(LINK_ALL_STATIC)
else ()
    set(LIB_TYPE SHARED)
endif ()

macro(bsp_setup_target_ext)
	if (NOT SET_NDEBUG)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
		string(REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
		string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	endif()
endmacro()

macro(bsp_setup_target_dll targetname)
	bsp_setup_target_ext()
	if (DEFINED LINK_ALL_STATIC)
		install(TARGETS ${targetname} LIBRARY DESTINATION lib)
	else()
		install(TARGETS ${targetname} LIBRARY DESTINATION bin)
	endif()
endmacro()

macro(bsp_setup_target_exe targetname)
	bsp_setup_target_ext()
	install(TARGETS ${targetname} RUNTIME DESTINATION bin)
endmacro()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4275")

##################################################################################################

add_subdirectory(tools/keyvalue-parser)
add_subdirectory(tools/common)
add_subdirectory(libpandabsp)

if (PYTHON_VERSION)
	add_subdirectory(networksystem)
endif()

if (IS_GAME_BUILD)
	add_subdirectory(game/server)
	add_subdirectory(game/client)
	add_subdirectory(game/dedicated)
endif()

#add_subdirectory(tools/p3csg)
#add_subdirectory(tools/p3bsp)
#add_subdirectory(tools/p3vis)
#add_subdirectory(tools/p3rad)