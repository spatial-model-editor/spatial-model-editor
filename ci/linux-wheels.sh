# based on https://github.com/pypa/python-manylinux-demo
# directory with setup.py should be mounted as /io
# compiled manylinux wheels generated in /io/dist

#!/bin/bash
set -e -x

TMPDIR=tmpwheelbuildir

mkdir -p /io/$TMPDIR
cd /io

cmake --version
gcc --version
ccache -s

export SME_EXTRA_EXE_LIBS="-static-libgcc;-static-libstdc++"

# Compile wheels
for PYBIN in $(ls -d /opt/py*/*/bin); do
    echo $PYBIN
	"${PYBIN}/python" --version
	"${PYBIN}/pip" --version
    # compile wheel
    time "${PYBIN}/pip" wheel /io/ -w /io/$TMPDIR/wheels/
    "${PYBIN}/python" /io/setup.py sdist -d /io/dist/
    # install wheel locally & run tests
    rm -rf /io/tmp_python_install
    "${PYBIN}/pip" install --target=/io/tmp_python_install --no-index --find-links=/io/$TMPDIR/wheels/ sme
    PYTHONPATH=/io/tmp_python_install "${PYBIN}/python" -m unittest discover -v
done

# Check and rename wheels
for whl in /io/$TMPDIR/wheels/*.whl; do
    auditwheel repair "$whl" -w /io/dist/
done

ccache -s