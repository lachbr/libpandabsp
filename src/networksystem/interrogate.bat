
REM Make sure this is set to the correct directory on your machine!
set PANDA_DIR=..\..\..\cio-panda3d\built_x64

set INTERROGATE=%PANDA_DIR%\bin\interrogate
set INTERROGATE_MODULE=%PANDA_DIR%\bin\interrogate_module
set PANDA_INCLUDE=%PANDA_DIR%/include
set MODULE=networksystem

%INTERROGATE% -fnames -string -refcount -assert -python-native -S%PANDA_INCLUDE% -S%PANDA_INCLUDE%/parser-inc -I./ -srcdir ./ -oc %MODULE%_igate.cpp -od %MODULE%.in -module %MODULE% -library %MODULE% -Dvolatile= -DINTERROGATE -DCPPPARSER -DCIO -D__STDC__=1 -D__cplusplus=201103L -D__inline -D_X86_ -DWIN32_VC -DWIN32 -D_WIN32 -D_MSC_VER=1600 -D"__declspec(param)=" -D__cdecl -D_near -D_far -D__near -D__far -D__stdcall networksystem.h

%INTERROGATE_MODULE% -python-native -import panda3d.core -module panda3d.%MODULE% -library %MODULE% -oc %MODULE%_module.cpp %MODULE%.in

pause
