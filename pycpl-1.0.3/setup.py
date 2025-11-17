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
import multiprocessing
from pathlib import Path

import pybind11

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = "") -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


class CMakeBuildExt(build_ext):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.deps_built = False

    def run(self) -> None:
        try:
            _ = subprocess.check_output(["cmake", "--version"])
        except OSError as e:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: "
                + ", ".join(e.name for e in self.extensions)
            ) from e

        # Build vendored dependencies first
        if not self.deps_built:
            self.build_dependencies()
            self.deps_built = True

        for ext in self.extensions:
            self.build_extension(ext)

    def build_dependencies(self) -> None:
        """Build vendored C libraries: cfitsio, fftw, wcslib, and cpl"""
        print("=" * 60)
        print("Building vendored C library dependencies")
        print("=" * 60)

        # Get the source directory (where setup.py is)
        source_dir = Path(__file__).parent.resolve()
        vendor_dir = source_dir / "vendor"

        # Create build directory for dependencies
        deps_build_dir = Path(self.build_temp).resolve() / "deps"
        deps_build_dir.mkdir(parents=True, exist_ok=True)

        # Installation prefix for dependencies
        deps_install_dir = deps_build_dir / "install"
        deps_install_dir.mkdir(parents=True, exist_ok=True)

        # Number of parallel jobs
        njobs = os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL") or str(multiprocessing.cpu_count())

        # Build each dependency in order
        self._build_cfitsio(vendor_dir, deps_build_dir, deps_install_dir, njobs)
        self._build_fftw(vendor_dir, deps_build_dir, deps_install_dir, njobs)
        self._build_wcslib(vendor_dir, deps_build_dir, deps_install_dir, njobs)
        self._build_cpl(vendor_dir, deps_build_dir, deps_install_dir, njobs)

        # Set CPLDIR environment variable so FindCPL.cmake can find it
        os.environ["CPLDIR"] = str(deps_install_dir)
        print(f"\nCPLDIR set to: {deps_install_dir}")
        print("=" * 60)

    def _build_cfitsio(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build cfitsio library"""
        print("\n>>> Building cfitsio...")
        src_dir = vendor_dir / "cfitsio-4.6.2"
        build_subdir = build_dir / "cfitsio-build"
        build_subdir.mkdir(parents=True, exist_ok=True)

        # Use CMake for cfitsio
        subprocess.run([
            "cmake",
            str(src_dir),
            f"-DCMAKE_INSTALL_PREFIX={install_dir}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_SHARED_LIBS=ON",
            "-DUSE_PTHREADS=ON",
        ], cwd=build_subdir, check=True)

        subprocess.run(["cmake", "--build", ".", "-j", njobs], cwd=build_subdir, check=True)
        subprocess.run(["cmake", "--install", "."], cwd=build_subdir, check=True)
        print(">>> cfitsio built successfully")

    def _build_fftw(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build fftw library"""
        print("\n>>> Building fftw...")
        src_dir = vendor_dir / "fftw-3.3.10"
        build_subdir = build_dir / "fftw-build"
        build_subdir.mkdir(parents=True, exist_ok=True)

        # Use CMake for fftw
        subprocess.run([
            "cmake",
            str(src_dir),
            f"-DCMAKE_INSTALL_PREFIX={install_dir}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_SHARED_LIBS=ON",
            "-DENABLE_THREADS=ON",
        ], cwd=build_subdir, check=True)

        subprocess.run(["cmake", "--build", ".", "-j", njobs], cwd=build_subdir, check=True)
        subprocess.run(["cmake", "--install", "."], cwd=build_subdir, check=True)
        print(">>> fftw built successfully")

    def _build_wcslib(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build wcslib library"""
        print("\n>>> Building wcslib...")
        src_dir = vendor_dir / "wcslib-8.2.2"

        # wcslib doesn't support out-of-tree builds well, build in-source
        env = os.environ.copy()
        # Set proper LDFLAGS and CFLAGS instead of CFITSIOLIB/CFITSIOINC
        env["LDFLAGS"] = f"-L{install_dir / 'lib'}"
        env["CFLAGS"] = f"-I{install_dir / 'include'}"
        env["LD_LIBRARY_PATH"] = str(install_dir / "lib")

        subprocess.run([
            "./configure",
            f"--prefix={install_dir}",
            "--without-pgplot",
            "--disable-fortran",
        ], cwd=src_dir, env=env, check=True)

        subprocess.run(["make", f"-j{njobs}"], cwd=src_dir, check=True)
        # Install only libraries, skip documentation to avoid missing file errors
        subprocess.run(["make", "-C", "C", "install"], cwd=src_dir, check=True)
        # Install pkg-config file
        subprocess.run(["make", "install-lib"], cwd=src_dir, check=False)  # This may partially fail but installs what we need
        # Clean up build artifacts
        subprocess.run(["make", "distclean"], cwd=src_dir, check=False)
        print(">>> wcslib built successfully")

    def _build_cpl(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build CPL library"""
        print("\n>>> Building CPL...")
        src_dir = vendor_dir / "cpl-7.3.2"

        # CPL uses autoconf and needs to find the dependencies
        env = os.environ.copy()
        env["PKG_CONFIG_PATH"] = str(install_dir / "lib" / "pkgconfig")
        env["CFITSIO_CFLAGS"] = f"-I{install_dir / 'include'}"
        env["CFITSIO_LIBS"] = f"-L{install_dir / 'lib'} -lcfitsio"
        env["FFTW3_CFLAGS"] = f"-I{install_dir / 'include'}"
        env["FFTW3_LIBS"] = f"-L{install_dir / 'lib'} -lfftw3"
        env["WCSLIB_CFLAGS"] = f"-I{install_dir / 'include'} -I{install_dir / 'include' / 'wcslib'}"
        env["WCSLIB_LIBS"] = f"-L{install_dir / 'lib'} -lwcs"
        env["CPPFLAGS"] = f"-I{install_dir / 'include'}"
        env["LDFLAGS"] = f"-L{install_dir / 'lib'}"
        env["LD_LIBRARY_PATH"] = str(install_dir / "lib")

        # Regenerate autotools files if configure is missing
        if not (src_dir / "configure").exists():
            print(">>> Regenerating autotools files for CPL...")
            subprocess.run(["autoreconf", "-i"], cwd=src_dir, env=env, check=True)

        subprocess.run([
            "./configure",
            f"--prefix={install_dir}",
            "--disable-static",
            "--enable-shared",
        ], cwd=src_dir, env=env, check=True)

        subprocess.run(["make", f"-j{njobs}"], cwd=src_dir, check=True)
        subprocess.run(["make", "install"], cwd=src_dir, check=True)
        # Clean up build artifacts
        subprocess.run(["make", "distclean"], cwd=src_dir, check=False)
        print(">>> CPL built successfully")

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
