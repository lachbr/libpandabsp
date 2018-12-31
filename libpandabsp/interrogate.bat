set INTERROGATE=..\..\..\cio\cio-panda3d\built_x64\bin\interrogate
set INTERROGATE_MODULE=..\..\..\cio\cio-panda3d\built_x64\bin\interrogate_module
set PANDA_INCLUDE=../../../cio/cio-panda3d/built_x64/include
set MODULE=bsp

%INTERROGATE% -fnames -string -refcount -assert -python-native -S%PANDA_INCLUDE% -S%PANDA_INCLUDE%/parser-inc -I./include -srcdir ./include -oc src/%MODULE%_igate.cpp -od src/%MODULE%.in -module %MODULE% -library %MODULE% -Dvolatile= -DINTERROGATE -DCPPPARSER -D__STDC__=1 -D__cplusplus=201103L -D__inline -D_X86_ -DWIN32_VC -DWIN32 -D_WIN32 -D_MSC_VER=1600 -D"__declspec(param)=" -D__cdecl -D_near -D_far -D__near -D__far -D__stdcall config_bsp.h bsploader.h entity.h bsp_render.h shader_generator.h shader_spec.h bsp_material.h TexturePacker.h shader_vertexlitgeneric.h shader_lightmappedgeneric.h shader_unlitgeneric.h

%INTERROGATE_MODULE% -python-native -import panda3d.core -module %MODULE% -library %MODULE% -oc src/%MODULE%_module.cpp src/%MODULE%.in

pause
