# This file is part of PyCPL the ESO CPL Python language bindings
# Copyright (C) 2020-2024 European Southern Observatory
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import sys
import subprocess
from pathlib import Path

import pybind11

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = "") -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


class CMakeBuildExt(build_ext):
    def run(self) -> None:
        try:
            _ = subprocess.check_output(["cmake", "--version"])
        except OSError as e:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: "
                + ", ".join(e.name for e in self.extensions)
            ) from e
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext: CMakeExtension) -> None:
        # CAUTION: Using extdir requires trailing slash for auto-detection &
        # inclusion of auxiliary "native" libs
        #
        # Must be in this form due to bug in .resolve() only fixed in
        # Python 3.10+
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()

        debug = (
            int(os.environ.get("PYCPL_BUILD_DEBUG", 0))
            if self.debug is None
            else self.debug
        )
        sanitize = os.environ.get("PYCPL_BUILD_SANITIZE", "")
        # Preferably the namespace protected variable should be used,
        # however the environment variable VERBOSE is checked and used
        # by cmake and its generated scripts. So we are conservative here
        # in order to have a consistent behavior.
        verbose = int(os.environ.get("PYCPL_BUILD_VERBOSE", 0)) or int(
            os.environ.get("VERBOSE", 0)
        )

        cmake_args = []
        build_args = []

        if verbose:
            cmake_args += ["-DCMAKE_VERBOSE_MAKEFILE=TRUE"]

        if sanitize in ["address", "leak"]:
            debug = 1
            cmake_args += [f"-DSANITIZE:STRING={sanitize}"]

        cfg = "Debug" if debug else "Release"
        cmake_args += [
            f"-DCMAKE_BUILD_TYPE={cfg}",  # not used on MSVC, but no harm
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPython3_EXECUTABLE={sys.executable}",
        ]

        if "CMAKE_ARGS" in os.environ:
            cmake_args += [item for item in os.environ["CMAKE_ARGS"].split(" ") if item]

        cmake_args += [f"-DPYCPL_VERSION={self.distribution.get_version()}"]

        cmake_args += ["-Dpybind11_DIR:PATH=" + pybind11.get_cmake_dir()]

        cpldir = os.environ.get("CPLDIR", None)
        if cpldir is not None:
            cmake_args += [f"-DCPL_ROOT:PATH={Path(cpldir).resolve()}"]
        recipedir = os.environ.get("PYCPL_RECIPE_DIR", None)
        if recipedir is not None:
            cmake_args += [f"-DPYCPL_RECIPE_DIR:PATH={Path(recipedir).resolve()}"]

        # Set CMAKE_BUILD_PARALLEL_LEVEL to control the parallel build level
        # across all generators.
        if "CMAKE_BUILD_PARALLEL_LEVEL" not in os.environ:
            # self.parallel is a Python 3 only way to set parallel jobs by hand
            # using -j in the build_ext call, not supported by pip or PyPA-build.
            if hasattr(self, "parallel") and self.parallel:
                # CMake 3.12+ only.
                build_args += [f"-j{self.parallel}"]

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)

        subprocess.run(
            ["cmake", ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=build_temp, check=True
        )


setup(
    ext_modules=[CMakeExtension("cpl")],
    cmdclass={"build_ext": CMakeBuildExt},
)
