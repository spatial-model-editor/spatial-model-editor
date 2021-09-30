# based on https://github.com/pybind/cmake_example
import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: "
                + ", ".join(e.name for e in self.extensions)
            )

        if platform.system() == "Windows":
            cmake_version = LooseVersion(
                re.search(r"version\s*([\d.]+)", out.decode()).group(1)
            )
            if cmake_version < "3.1.0":
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = [
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + extdir,
            "-DPYTHON_EXECUTABLE=" + sys.executable,
            "-DBUILD_BENCHMARKS=off",
            "-DBUILD_CLI=off",
            "-DBUILD_GUI=off",
            "-DBUILD_TESTING=off",
        ]
        for e in [
            "CMAKE_PREFIX_PATH",
            "SME_EXTRA_CORE_DEFS",
            "SME_EXTRA_EXE_LIBS",
            "SME_EXTERNAL_SMECORE",
            "SME_QT_DISABLE_UNICODE",
            "SME_WITH_OPENMP",
            "PYTHON_LIBRARY",
            "CMAKE_CXX_COMPILER_LAUNCHER",
            "SME_DUNE_COPASI_USE_FALLBACK_FILESYSTEM",
        ]:
            try:
                cmake_args += ["-D" + e + "=" + os.environ.get(e)]
            except:
                pass

        cfg = "Debug" if self.debug else "Release"
        build_args = ["--config", cfg]

        cmake_args += ["-DCMAKE_BUILD_TYPE=" + cfg]
        build_args += ["--", "-j2"]

        env = os.environ.copy()
        env["CXXFLAGS"] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get("CXXFLAGS", ""), self.distribution.get_version()
        )
        # export source code directory as ccache base dir
        os.environ["CCACHE_BASEDIR"] = ext.sourcedir
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env
        )
        subprocess.check_call(
            ["cmake", "--build", ".", "--target", "sme"] + build_args,
            cwd=self.build_temp,
        )


from os import path

sme_directory = path.join(path.abspath(path.dirname(__file__)), "sme")
with open(path.join(sme_directory, "README.md")) as f:
    long_description = f.read()

setup(
    name="sme",
    version="1.2.1",
    author="Liam Keegan",
    author_email="liam@keegan.ch",
    description="Spatial Model Editor python bindings",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://spatial-model-editor.github.io/",
    project_urls={
        "Documentation": "https://spatial-model-editor.readthedocs.io/",
        "Github": "https://github.com/spatial-model-editor/spatial-model-editor",
        "Issues": "https://github.com/spatial-model-editor/spatial-model-editor/issues",
        "Try it online": "https://colab.research.google.com/github/spatial-model-editor/spatial-model-editor/blob/master/docs/sme/notebooks/getting_started.ipynb",
    },
    classifiers=[
        "Topic :: Scientific/Engineering :: Bio-Informatics",
        "License :: OSI Approved :: MIT License",
        "Intended Audience :: Science/Research",
        "Intended Audience :: Education",
        "Natural Language :: English",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: Implementation :: CPython",
        "Programming Language :: Python :: Implementation :: PyPy",
    ],
    ext_modules=[CMakeExtension("sme")],
    cmdclass=dict(build_ext=CMakeBuild),
    test_suite="test",
    install_requires=[
        "numpy",
    ],
    zip_safe=False,
)
