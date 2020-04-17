
REM Make sure this is set to the correct directory on your machine!
set PANDA_DIR=..\..\..\cio-panda3d\built_x64

set INTERROGATE=%PANDA_DIR%\bin\interrogate
set INTERROGATE_MODULE=%PANDA_DIR%\bin\interrogate_module
set PANDA_INCLUDE=%PANDA_DIR%/include
set MODULE=libpandabsp

%INTERROGATE% -fnames -string -refcount -assert -python-native -S%PANDA_INCLUDE%/parser-inc/ -S%PANDA_INCLUDE%/ -I./ -srcdir ./ -oc %MODULE%_igate.cpp -od %MODULE%.in -module %MODULE% -library %MODULE% -Dvolatile= -D_PYTHON_VERSION -DINTERROGATE -DCPPPARSER -DCIO -D__STDC__=1 -D__cplusplus=201103L -D__inline -D_X86_ -DWIN32_VC -DWIN32 -D_WIN32 -D_MSC_VER=1600 -DWIN64_VC -DWIN64 -D_WIN64 -D"__declspec(param)=" -D__cdecl -D_near -D_far -D__near -D__far -D__stdcall config_bsp.h bsploader.h entity.h bsp_render.h shader_generator.h shader_spec.h bsp_material.h TexturePacker.h shader_vertexlitgeneric.h shader_lightmappedgeneric.h shader_unlitgeneric.h shader_unlitnomat.h shader_csmrender.h raytrace.h shader_skybox.h ambient_boost_effect.h audio_3d_manager.h ciolib.h bounding_kdop.h shader_decalmodulate.h glow_node.h postprocess/postprocess.h postprocess/hdr.h postprocess/bloom.h lighting_origin_effect.h planar_reflections.h postprocess/fxaa.h bloom_attrib.h physics_character_controller.h py_bsploader.h

%INTERROGATE_MODULE% -python-native -import panda3d.core -import panda3d.bullet -module %MODULE% -library %MODULE% -oc %MODULE%_module.cpp %MODULE%.in

pause
