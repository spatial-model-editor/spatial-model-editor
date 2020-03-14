# based on https://github.com/pypa/python-manylinux-demo
# directory with setup.py should be mounted as /io
# compiled manylinux wheels generated in /io/dist

#!/bin/bash
set -e -x

TMPDIR=tmpwheelbuildir

mkdir -p /io/$TMPDIR
cd /io/$TMPDIR

cmake --version
$CXX --version

export SME_EXTRA_EXE_LIBS="-static-libgcc;-static-libstdc++"

# Compile wheels
for PYBIN in /opt/python/cp*/bin; do
	"${PYBIN}/python" --version
	"${PYBIN}/pip" --version
    time "${PYBIN}/pip" -v wheel /io/ -w /io/$TMPDIR/wheels/
    "${PYBIN}/python" /io/setup.py sdist -d /io/dist/
done

# Check and rename wheels
for whl in /io/$TMPDIR/wheels/*.whl; do
    auditwheel repair "$whl" -w /io/dist/
done
