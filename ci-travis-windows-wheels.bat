setlocal EnableDelayedExpansion

set PATH=C:\tools\msys64\mingw64\bin

g++.exe --version
mingw32-make.exe --version
cmake.exe --version

set CXX=g++.exe
set CMAKE_GENERATOR=MinGW Makefiles
set CMAKE_PREFIX_PATH=C:\libs\install;C:\libs;C:\libs\lib\cmake;C:\libs\dune
rem https://stackoverflow.com/questions/10660524/error-building-boost-1-49-0-with-gcc-4-7-0/30881190#30881190
set SME_EXTRA_CORE_DEFS=_hypot=hypot;MS_WIN64
rem add static flags
set SME_EXTRA_EXE_LIBS=-static;-static-libgcc;-static-libstdc++
rem add windows libs that are currently missing from qt cmake
set SME_EXTRA_CORE_LIBS=C:\\libs\\install\\lib\\libqtfreetype.a;C:\\libs\\install\\lib\\libqtlibpng.a;C:\\libs\\install\\lib\\libqtharfbuzz.a;C:\\libs\\install\\lib\\libqtpcre2.a;-ld2d1;-ld3d11;-ldwmapi;-ldwrite;-ldxgi;-ldxguid;-limm32;-lmpr;-lnetapi32;-lpsapi;-lshlwapi;-luserenv;-luxtheme;-lversion;-lws2_32;-lwtsapi32;-lwinmm

for %%v in (27 35 36 37 38) DO (
    set PYBIN=C:\Python%%v\python.exe
    set PYTHON_LIBRARY=C:\Python%%v\python%%v.dll

    if "%%v"=="27" (
	    set PYBIN=C:\tools\python\python.exe
    	set PYTHON_LIBRARY=C:\tools\python\libs\libpython%%v.a
	)

    echo !PYBIN!
    echo !PYTHON_LIBRARY!

    !PYBIN! --version
    !PYBIN! -m pip install --upgrade pip
    !PYBIN! -m pip install --upgrade setuptools
    !PYBIN! -m pip install --upgrade wheel

    !PYBIN! -m pip -v wheel . -w dist
    !PYBIN! setup.py sdist -d dist
)
