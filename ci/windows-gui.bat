setlocal EnableDelayedExpansion

set PATH=C:\tools\msys64\mingw64\bin
dir C:\tools\msys64\mingw64\bin

C:\Python38\python.exe --version
g++.exe --version
mingw32-make.exe --version
cmake.exe --version

C:\Python38\python.exe -m pip install nose

set CXX=g++.exe
set CMAKE_GENERATOR=MinGW Makefiles
set CMAKE_PREFIX_PATH=C:\libs\install;C:\libs;C:\libs\lib\cmake;C:\libs\dune
rem https://stackoverflow.com/questions/10660524/error-building-boost-1-49-0-with-gcc-4-7-0/30881190#30881190
set SME_EXTRA_CORE_DEFS=_hypot=hypot;MS_WIN64
set SME_EXTRA_EXE_LIBS=-static;-static-libgcc;-static-libstdc++
rem add windows libs that are currently missing from qt cmake
rem set SME_EXTRA_CORE_LIBS=C:\\libs\\install\\lib\\libqtfreetype.a;C:\\libs\\install\\lib\\libqtlibpng.a;C:\\libs\\install\\lib\\libqtharfbuzz.a;C:\\libs\\install\\lib\\libqtpcre2.a;-ld2d1;-ld3d11;-ldwmapi;-ldwrite;-ldxgi;-ldxguid;-limm32;-lmpr;-lnetapi32;-lpsapi;-lshlwapi;-luserenv;-luxtheme;-lversion;-lws2_32;-lwtsapi32;-lwinmm

mkdir build
cd build
cmake.exe .. -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=C:\Python38\python.exe -DPYTHON_LIBRARY=C:\Python38\Python38.dll -DSME_EXTRA_EXE_LIBS=%SME_EXTRA_EXE_LIBS% -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% -DSME_EXTRA_CORE_LIBS=%SME_EXTRA_CORE_LIBS%

mingw32-make.exe -j2 VERBOSE=1

test\tests.exe -as "~[gui]" > tests.txt 2>&1

benchmark\benchmark.exe 1

cd ..
move build\sme\sme.cp38-win_amd64.pyd .

objdump.exe -x sme.cp38-win_amd64.pyd > sme_obj.txt

C:\Python38\python.exe -m unittest discover -v
