setlocal EnableDelayedExpansion
for %%v in (27 35 36 37 38) DO (
    set PYBIN=C:\Python%%v-x64\python.exe
    echo !PYBIN!

    set PYTHON_LIBRARY=C:\Python%%v-x64\python%%v.dll
    if "%%v"=="27" (
    	set PYTHON_LIBRARY=C:\Python%%v-x64\libs\libpython%%v.a
	)
    echo !PYTHON_LIBRARY!

    dir C:\Python%%v-x64\

    !PYBIN! --version
    !PYBIN! -m pip install --upgrade pip
    !PYBIN! -m pip install wheel

    !PYBIN! -m pip -v wheel . -w dist
    !PYBIN! setup.py sdist -d dist
)
