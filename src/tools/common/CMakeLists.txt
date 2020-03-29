set(COMMON_HEADERS
	anorms.h
	blockmem.h
	boundingbox.h
	bspfile.h
	bsptools.h
	clhelper.h
	cmdlib.h
	cmdlinecfg.h
	common_config.h
	filelib.h
	halton.h
	mathtypes.h
	messages.h
	resourcelock.h
	scriplib.h
	threads.h
	TimeCounter.h
	win32fix.h
	winding.h
)

set(COMMON_SOURCES
	anorms.cpp
	blockmem.cpp
	bspfile.cpp
	bsptools.cpp
	clhelper.cpp
	cmdlib.cpp
	cmdlinecfg.cpp
	filelib.cpp
	files.cpp
	halton.cpp
	log.cpp
	mathlib.cpp
	messages.cpp
	resourcelock.cpp
	scriplib.cpp
	threads.cpp
	winding.cpp
)

set(MATHLIB_HEADERS
	mathlib/sse.h
	mathlib/ssemath.h
)

set(MATHLIB_SOURCES
	mathlib/powsse.cpp
	mathlib/sse.cpp
	mathlib/sseconst.cpp
)

set(CL_HEADERS
	CL/cl.h
	CL/cl_d3d10.h
	CL/cl_d3d11.h
	CL/cl_dx9_media_sharing.h
	CL/cl_dx9_media_sharing_intel.h
	CL/cl_egl.h
	CL/cl_ext.h
	CL/cl_ext_intel.h
	CL/cl_gl.h
	CL/cl_gl_ext.h
	CL/cl_platform.h
	CL/cl_va_api_media_sharing_intel.h
	CL/cl_version.h
	CL/opencl.h
)

source_group("Header Files" FILES ${COMMON_HEADERS})
source_group("Source Files" FILES ${COMMON_SOURCES})
source_group("Header Files\\mathlib" FILES ${MATHLIB_HEADERS})
source_group("Source Files\\mathlib" FILES ${MATHLIB_SOURCES})
source_group("Header Files\\CL" FILES ${CL_HEADERS})

add_library(bsp_common ${LIB_TYPE}
	${COMMON_SOURCES}
	${COMMON_HEADERS}
	${MATHLIB_SOURCES}
	${MATHLIB_HEADERS}
	${CL_HEADERS}
)

bsp_setup_target(bsp_common)

target_compile_definitions(bsp_common PRIVATE BUILDING_BSPCOMMON NOMINMAX STDC_HEADERS)

target_include_directories(bsp_common PRIVATE
	./
	${INCPANDA}
)

target_link_directories(bsp_common PRIVATE
	${LIBPANDA}
)

target_link_libraries(bsp_common
	libpanda.lib
	libpandaexpress.lib
	libp3dtool.lib
	libp3dtoolconfig.lib
)


