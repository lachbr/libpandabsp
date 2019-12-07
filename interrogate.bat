
REM Make sure this is set to the correct directory on your machine!
set PANDA_DIR=..\cio-panda3d\built_x64

set INTERROGATE=%PANDA_DIR%\bin\interrogate
set INTERROGATE_MODULE=%PANDA_DIR%\bin\interrogate_module
set PANDA_INCLUDE=%PANDA_DIR%/include
set MODULE=bsp

move %PANDA_DIR%\include\Eigen %PANDA_DIR%\Eigen

%INTERROGATE% -fnames -string -refcount -assert -python-native -S%PANDA_INCLUDE% -S%PANDA_INCLUDE%/parser-inc -I./include -srcdir ./include -oc src/%MODULE%_igate.cpp -od src/%MODULE%.in -module %MODULE% -library %MODULE% -Dvolatile= -DINTERROGATE -DCPPPARSER -DCIO -D__STDC__=1 -D__cplusplus=201103L -D__inline -D_X86_ -DWIN32_VC -DWIN32 -D_WIN32 -D_MSC_VER=1600 -D"__declspec(param)=" -D__cdecl -D_near -D_far -D__near -D__far -D__stdcall config_bsp.h bsploader.h entity.h bsp_render.h shader_generator.h shader_spec.h bsp_material.h TexturePacker.h shader_vertexlitgeneric.h shader_lightmappedgeneric.h shader_unlitgeneric.h shader_unlitnomat.h shader_csmrender.h raytrace.h shader_skybox.h ambient_boost_effect.h audio_3d_manager.h ciolib.h bounding_kdop.h shader_decalmodulate.h glow_node.h postprocess/postprocess.h postprocess/hdr.h postprocess/bloom.h lighting_origin_effect.h planar_reflections.h postprocess/fxaa.h bloom_attrib.h

%INTERROGATE_MODULE% -python-native -import panda3d.core -module panda3d.%MODULE% -library %MODULE% -oc src/%MODULE%_module.cpp src/%MODULE%.in

move %PANDA_DIR%\Eigen %PANDA_DIR%\include\Eigen

pause
