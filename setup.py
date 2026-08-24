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
import sysconfig
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import pybind11

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


# Bump when the dependency build itself changes in a way that invalidates old trees
DEPS_STAMP_SCHEMA = "deps-v1"


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

    def _deps_dir(self) -> Path:
        """Directory for the vendored C libraries.

        Deliberately keyed on the platform rather than the interpreter: these
        libraries have no Python linkage, so all of cp312/cp313/cp314 share one
        build instead of repeating it three times per CI run.
        """
        return Path(self.build_temp).resolve().parent / f"deps-{sysconfig.get_platform()}"

    def _deps_stamp_value(self) -> str:
        """Identity of the current vendored sources; a change forces a rebuild."""
        vendor_dir = Path(__file__).parent.resolve() / "vendor"
        trees = sorted(p.name for p in vendor_dir.iterdir() if p.is_dir())
        return "\n".join([DEPS_STAMP_SCHEMA, *trees])

    def build_dependencies(self) -> None:
        """Build vendored C libraries: cfitsio, fftw, wcslib, cpl, gsl, erfa, and hdrl"""
        print("=" * 60)
        print("Building vendored C library dependencies")
        print("=" * 60)

        # Get the source directory (where setup.py is)
        source_dir = Path(__file__).parent.resolve()
        vendor_dir = source_dir / "vendor"

        # Create build directory for dependencies (shared across Python versions)
        deps_build_dir = self._deps_dir()
        deps_build_dir.mkdir(parents=True, exist_ok=True)

        # Installation prefix for dependencies
        deps_install_dir = deps_build_dir / "install"
        deps_install_dir.mkdir(parents=True, exist_ok=True)

        stamp = deps_build_dir / ".deps-complete"
        want = self._deps_stamp_value()
        if stamp.is_file() and stamp.read_text() == want:
            print(f">>> Reusing vendored C libraries from {deps_install_dir}")
            os.environ["CPLDIR"] = str(deps_install_dir)
            os.environ["HDRLDIR"] = str(deps_install_dir)
            print("=" * 60)
            return

        # Number of parallel jobs
        njobs = os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL") or str(multiprocessing.cpu_count())

        # Build dependencies with parallelization where possible
        # Phase 1: Build cfitsio, fftw, gsl, erfa in parallel (independent)
        print("\n>>> Phase 1: Building cfitsio, fftw, gsl, erfa in parallel...")
        with ThreadPoolExecutor(max_workers=4) as executor:
            future_cfitsio = executor.submit(
                self._build_cfitsio, vendor_dir, deps_build_dir, deps_install_dir, njobs
            )
            future_fftw = executor.submit(
                self._build_fftw, vendor_dir, deps_build_dir, deps_install_dir, njobs
            )
            future_gsl = executor.submit(
                self._build_gsl, vendor_dir, deps_build_dir, deps_install_dir, njobs
            )
            future_erfa = executor.submit(
                self._build_erfa, vendor_dir, deps_build_dir, deps_install_dir, njobs
            )

            # Wait for all to complete and handle any errors
            for future in as_completed([future_cfitsio, future_fftw, future_gsl, future_erfa]):
                future.result()  # Will raise exception if build failed

        print(">>> Phase 1 complete: cfitsio, fftw, gsl, erfa built successfully")

        # Phase 2: Build wcslib (depends on cfitsio)
        print("\n>>> Phase 2: Building wcslib...")
        self._build_wcslib(vendor_dir, deps_build_dir, deps_install_dir, njobs)

        # Phase 3: Build cpl (depends on cfitsio, fftw, wcslib)
        print("\n>>> Phase 3: Building cpl...")
        self._build_cpl(vendor_dir, deps_build_dir, deps_install_dir, njobs)

        # Phase 4: Build hdrl (depends on cpl, gsl, erfa)
        print("\n>>> Phase 4: Building hdrl...")
        self._build_hdrl(vendor_dir, deps_build_dir, deps_install_dir, njobs)

        stamp.write_text(want)

        # Set CPLDIR environment variable so FindCPL.cmake can find it
        os.environ["CPLDIR"] = str(deps_install_dir)
        # Set HDRLDIR for FindHDRL.cmake
        os.environ["HDRLDIR"] = str(deps_install_dir)
        print(f"\nCPLDIR/HDRLDIR set to: {deps_install_dir}")
        print("=" * 60)

    def _build_cfitsio(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build cfitsio library"""
        print("\n>>> Building cfitsio...")
        src_dir = vendor_dir / "cfitsio-4.6.2"
        build_subdir = build_dir / "cfitsio-build"
        build_subdir.mkdir(parents=True, exist_ok=True)

        # Use CMake for cfitsio
        cmake_args = [
            "cmake",
            str(src_dir),
            f"-DCMAKE_INSTALL_PREFIX={install_dir}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_SHARED_LIBS=ON",
            "-DUSE_PTHREADS=ON",
            "-DCMAKE_INSTALL_LIBDIR=lib",
        ]
        # Use absolute install names during build so configure tests work (macOS)
        if sys.platform == "darwin":
            cmake_args.append(f"-DCMAKE_INSTALL_NAME_DIR={install_dir / 'lib'}")
        # Force RPATH instead of RUNPATH on Linux for transitive library loading
        if sys.platform == "linux":
            cmake_args.append("-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--disable-new-dtags")
        subprocess.run(cmake_args, cwd=build_subdir, check=True)

        subprocess.run(["cmake", "--build", ".", "-j", njobs], cwd=build_subdir, check=True)
        subprocess.run(["cmake", "--install", "."], cwd=build_subdir, check=True)

        # Note: install names will be fixed after all dependencies are built
        print(">>> cfitsio built successfully")

    def _build_fftw(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build fftw library (both double and single precision)"""
        print("\n>>> Building fftw...")
        src_dir = vendor_dir / "fftw-3.3.10"

        # Build double precision (default)
        print(">>> Building fftw (double precision)...")
        build_double = build_dir / "fftw-build-double"
        build_double.mkdir(parents=True, exist_ok=True)

        cmake_args_double = [
            "cmake",
            str(src_dir),
            f"-DCMAKE_INSTALL_PREFIX={install_dir}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_SHARED_LIBS=ON",
            "-DENABLE_THREADS=ON",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            # Use absolute install names during build so configure tests work
            f"-DCMAKE_INSTALL_NAME_DIR={install_dir / 'lib'}",
        ]
        # Force RPATH instead of RUNPATH on Linux for transitive library loading
        if sys.platform == "linux":
            cmake_args_double.append("-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--disable-new-dtags")
        subprocess.run(cmake_args_double, cwd=build_double, check=True)
        subprocess.run(["cmake", "--build", ".", "-j", njobs], cwd=build_double, check=True)
        subprocess.run(["cmake", "--install", "."], cwd=build_double, check=True)

        # Build single precision
        print(">>> Building fftw (single precision)...")
        build_single = build_dir / "fftw-build-single"
        build_single.mkdir(parents=True, exist_ok=True)

        cmake_args_single = [
            "cmake",
            str(src_dir),
            f"-DCMAKE_INSTALL_PREFIX={install_dir}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_SHARED_LIBS=ON",
            "-DENABLE_THREADS=ON",
            "-DENABLE_FLOAT=ON",  # Enable single precision
            "-DCMAKE_INSTALL_LIBDIR=lib",
            # Use absolute install names during build so configure tests work
            f"-DCMAKE_INSTALL_NAME_DIR={install_dir / 'lib'}",
        ]
        # Force RPATH instead of RUNPATH on Linux for transitive library loading
        if sys.platform == "linux":
            cmake_args_single.append("-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--disable-new-dtags")
        subprocess.run(cmake_args_single, cwd=build_single, check=True)
        subprocess.run(["cmake", "--build", ".", "-j", njobs], cwd=build_single, check=True)
        subprocess.run(["cmake", "--install", "."], cwd=build_single, check=True)

        # Note: install names will be fixed after all dependencies are built

        # Generate pkg-config files (CMake build doesn't create them, but CPL needs them)
        pkgconfig_dir = install_dir / "lib" / "pkgconfig"
        pkgconfig_dir.mkdir(parents=True, exist_ok=True)

        # On macOS, include rpath so configure test programs can find the library
        if sys.platform == "darwin":
            rpath_flag = f"-Wl,-rpath,{install_dir / 'lib'}"
        else:
            rpath_flag = f"-Wl,-rpath,{install_dir / 'lib'}"

        fftw3_pc = f"""prefix={install_dir}
exec_prefix=${{prefix}}
libdir=${{exec_prefix}}/lib
includedir=${{prefix}}/include

Name: fftw3
Description: FFTW3 double precision library
Version: 3.3.10
Libs: -L${{libdir}} {rpath_flag} -lfftw3
Cflags: -I${{includedir}}
"""
        (pkgconfig_dir / "fftw3.pc").write_text(fftw3_pc)

        fftw3f_pc = f"""prefix={install_dir}
exec_prefix=${{prefix}}
libdir=${{exec_prefix}}/lib
includedir=${{prefix}}/include

Name: fftw3f
Description: FFTW3 single precision library
Version: 3.3.10
Libs: -L${{libdir}} {rpath_flag} -lfftw3f
Cflags: -I${{includedir}}
"""
        (pkgconfig_dir / "fftw3f.pc").write_text(fftw3f_pc)

        print(">>> fftw built successfully (both precisions)")

    def _build_wcslib(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build wcslib library"""
        print("\n>>> Building wcslib...")
        src_dir = vendor_dir / "wcslib-8.2.2"

        # wcslib doesn't support out-of-tree builds well, build in-source
        env = os.environ.copy()
        # Set proper LDFLAGS and CFLAGS instead of CFITSIOLIB/CFITSIOINC
        env["CFLAGS"] = f"-I{install_dir / 'include'}"
        lib_path = str(install_dir / "lib")
        # Use $ORIGIN for RPATH and force old-style RPATH (not RUNPATH) for transitive loading
        # Also include rpath to install_dir/lib so configure test programs can find libraries
        if sys.platform == "linux":
            rpath_flags = f"-Wl,--disable-new-dtags,-rpath,$ORIGIN,-rpath,{lib_path}"
        else:
            rpath_flags = f"-Wl,-rpath,@loader_path,-rpath,{lib_path}"
        ldflags = f"-L{lib_path} {rpath_flags}"
        env["LDFLAGS"] = (
            f"{ldflags} {env['LDFLAGS']}"
            if env.get("LDFLAGS")
            else ldflags
        )
        env["LD_LIBRARY_PATH"] = (
            f"{lib_path}:{env['LD_LIBRARY_PATH']}"
            if env.get("LD_LIBRARY_PATH")
            else lib_path
        )
        if sys.platform == "darwin":
            env["DYLD_LIBRARY_PATH"] = (
                f"{lib_path}:{env['DYLD_LIBRARY_PATH']}"
                if env.get("DYLD_LIBRARY_PATH")
                else lib_path
            )

        subprocess.run([
            "./configure",
            f"--prefix={install_dir}",
            "--without-pgplot",
            "--disable-fortran",
        ], cwd=src_dir, env=env, check=True)

        subprocess.run(["make", f"-j{njobs}"], cwd=src_dir, check=True)
        # Install library and headers, skip documentation
        subprocess.run(["make", "-C", "C", "install"], cwd=src_dir, check=True)
        # Install wcsconfig.h and other header files
        subprocess.run(["make", "install-nobase_includeHEADERS"], cwd=src_dir, check=False)
        # Install pkg-config file
        pkgconfig_dir = install_dir / "lib" / "pkgconfig"
        pkgconfig_dir.mkdir(parents=True, exist_ok=True)
        if (src_dir / "wcsconfig.h").exists():
            import shutil
            # Copy wcsconfig.h to the wcslib include directory
            wcslib_include = install_dir / "include" / "wcslib"
            if wcslib_include.exists():
                shutil.copy(src_dir / "wcsconfig.h", wcslib_include / "wcsconfig.h")
        if (src_dir / "wcslib.pc").exists():
            import shutil
            shutil.copy(src_dir / "wcslib.pc", pkgconfig_dir / "wcslib.pc")

        # Note: install names will be fixed after all dependencies are built

        # Clean up build artifacts
        subprocess.run(["make", "distclean"], cwd=src_dir, check=False)
        print(">>> wcslib built successfully")

    def _build_cpl(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build CPL library"""
        print("\n>>> Building CPL...")
        src_dir = vendor_dir / "cpl-7.4"

        # CPL uses autoconf and needs to find the dependencies
        env = os.environ.copy()
        # Prevent Java from being found to avoid building cpljava
        env.pop("JAVA_HOME", None)
        env["PKG_CONFIG_PATH"] = str(install_dir / "lib" / "pkgconfig")
        env["CFITSIO_CFLAGS"] = f"-I{install_dir / 'include'}"
        env["CFITSIO_LIBS"] = f"-L{install_dir / 'lib'} -lcfitsio"
        env["FFTW3_CFLAGS"] = f"-I{install_dir / 'include'}"
        env["FFTW3_LIBS"] = f"-L{install_dir / 'lib'} -lfftw3"
        env["WCSLIB_CFLAGS"] = f"-I{install_dir / 'include' / 'wcslib'}"
        env["WCSLIB_LIBS"] = f"-L{install_dir / 'lib'} -lwcs"
        env["CPPFLAGS"] = f"-I{install_dir / 'include'} -I{install_dir / 'include' / 'wcslib'}"
        lib_path = str(install_dir / "lib")
        # Use $ORIGIN for RPATH and force old-style RPATH (not RUNPATH) for transitive loading
        # Also include rpath to install_dir/lib so configure test programs can find libraries
        if sys.platform == "linux":
            rpath_flags = f"-Wl,--disable-new-dtags,-rpath,$ORIGIN,-rpath,{lib_path}"
        else:
            rpath_flags = f"-Wl,-rpath,@loader_path,-rpath,{lib_path}"
        ldflags = f"-L{lib_path} {rpath_flags}"
        env["LDFLAGS"] = (
            f"{ldflags} {env['LDFLAGS']}"
            if env.get("LDFLAGS")
            else ldflags
        )
        env["LD_LIBRARY_PATH"] = (
            f"{lib_path}:{env['LD_LIBRARY_PATH']}"
            if env.get("LD_LIBRARY_PATH")
            else lib_path
        )
        if sys.platform == "darwin":
            env["DYLD_LIBRARY_PATH"] = (
                f"{lib_path}:{env['DYLD_LIBRARY_PATH']}"
                if env.get("DYLD_LIBRARY_PATH")
                else lib_path
            )

        # Regenerate autotools files if configure is missing
        if not (src_dir / "configure").exists():
            print(">>> Regenerating autotools files for CPL...")
            subprocess.run(["autoreconf", "-i"], cwd=src_dir, env=env, check=True)

        subprocess.run([
            "./configure",
            f"--prefix={install_dir}",
            "--disable-static",
            "--enable-shared",
            "--disable-java",
            # CPL 7.4 defaults --with-system-cext to yes; we always want the bundled one
            "--with-system-cext=no",
        ], cwd=src_dir, env=env, check=True)

        subprocess.run(["make", f"-j{njobs}"], cwd=src_dir, check=True)
        subprocess.run(["make", "install"], cwd=src_dir, check=True)

        # Fix install names on macOS for ALL vendored libraries (deferred until now
        # so configure test programs can find libraries during build)
        self._fix_darwin_install_names(
            install_dir / "lib",
            [
                # cfitsio
                "libcfitsio.10.dylib",
                # fftw
                "libfftw3.3.dylib",
                "libfftw3_threads.3.dylib",
                "libfftw3f.3.dylib",
                "libfftw3f_threads.3.dylib",
                # wcslib
                "libwcs.8.dylib",
                # CPL
                "libcext.0.dylib",
                "libcplcore.26.dylib",
                "libcplui.26.dylib",
                "libcpldfs.26.dylib",
                "libcpldrs.26.dylib",
            ],
        )

        # Clean up build artifacts
        subprocess.run(["make", "distclean"], cwd=src_dir, check=False)
        print(">>> CPL built successfully")

    def _build_gsl(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build GSL library"""
        print("\n>>> Building GSL...")
        src_dir = vendor_dir / "gsl-2.8"

        env = os.environ.copy()
        lib_path = str(install_dir / "lib")
        if sys.platform == "linux":
            rpath_flags = f"-Wl,--disable-new-dtags,-rpath,$ORIGIN,-rpath,{lib_path}"
        else:
            rpath_flags = f"-Wl,-rpath,@loader_path,-rpath,{lib_path}"
        ldflags = f"-L{lib_path} {rpath_flags}"
        env["LDFLAGS"] = f"{ldflags} {env['LDFLAGS']}" if env.get("LDFLAGS") else ldflags
        env["LD_LIBRARY_PATH"] = f"{lib_path}:{env['LD_LIBRARY_PATH']}" if env.get("LD_LIBRARY_PATH") else lib_path
        if sys.platform == "darwin":
            env["DYLD_LIBRARY_PATH"] = f"{lib_path}:{env['DYLD_LIBRARY_PATH']}" if env.get("DYLD_LIBRARY_PATH") else lib_path

        # GSL needs autoreconf (doc/ dir removed from sources)
        if not (src_dir / "configure").exists():
            print(">>> Regenerating autotools files for GSL...")
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
        print(">>> GSL built successfully")

    def _build_erfa(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build ERFA library"""
        print("\n>>> Building ERFA...")
        src_dir = vendor_dir / "erfa-2.0.1"

        env = os.environ.copy()
        lib_path = str(install_dir / "lib")
        if sys.platform == "linux":
            rpath_flags = f"-Wl,--disable-new-dtags,-rpath,$ORIGIN,-rpath,{lib_path}"
        else:
            rpath_flags = f"-Wl,-rpath,@loader_path,-rpath,{lib_path}"
        ldflags = f"-L{lib_path} {rpath_flags}"
        env["LDFLAGS"] = f"{ldflags} {env['LDFLAGS']}" if env.get("LDFLAGS") else ldflags
        env["LD_LIBRARY_PATH"] = f"{lib_path}:{env['LD_LIBRARY_PATH']}" if env.get("LD_LIBRARY_PATH") else lib_path
        if sys.platform == "darwin":
            env["DYLD_LIBRARY_PATH"] = f"{lib_path}:{env['DYLD_LIBRARY_PATH']}" if env.get("DYLD_LIBRARY_PATH") else lib_path

        # ERFA needs autoreconf
        if not (src_dir / "configure").exists():
            print(">>> Regenerating autotools files for ERFA...")
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
        print(">>> ERFA built successfully")

    def _build_hdrl(self, vendor_dir: Path, build_dir: Path, install_dir: Path, njobs: str) -> None:
        """Build HDRL library"""
        print("\n>>> Building HDRL...")
        src_dir = vendor_dir / "hdrl-1.6.0a5"

        env = os.environ.copy()
        env["PKG_CONFIG_PATH"] = str(install_dir / "lib" / "pkgconfig")
        env["CPL_CFLAGS"] = f"-I{install_dir / 'include'}"
        env["CPL_LIBS"] = f"-L{install_dir / 'lib'} -lcplcore -lcplui -lcpldfs -lcpldrs -lcext"
        env["GSL_CFLAGS"] = f"-I{install_dir / 'include'}"
        env["GSL_LIBS"] = f"-L{install_dir / 'lib'} -lgsl -lgslcblas"
        env["ERFA_CFLAGS"] = f"-I{install_dir / 'include'}"
        env["ERFA_LIBS"] = f"-L{install_dir / 'lib'} -lerfa"
        env["CPPFLAGS"] = f"-I{install_dir / 'include'}"
        lib_path = str(install_dir / "lib")
        if sys.platform == "linux":
            rpath_flags = f"-Wl,--disable-new-dtags,-rpath,$ORIGIN,-rpath,{lib_path}"
        else:
            rpath_flags = f"-Wl,-rpath,@loader_path,-rpath,{lib_path}"
        ldflags = f"-L{lib_path} {rpath_flags}"
        env["LDFLAGS"] = f"{ldflags} {env['LDFLAGS']}" if env.get("LDFLAGS") else ldflags
        env["LD_LIBRARY_PATH"] = f"{lib_path}:{env['LD_LIBRARY_PATH']}" if env.get("LD_LIBRARY_PATH") else lib_path
        if sys.platform == "darwin":
            env["DYLD_LIBRARY_PATH"] = f"{lib_path}:{env['DYLD_LIBRARY_PATH']}" if env.get("DYLD_LIBRARY_PATH") else lib_path

        # Regenerate autotools files if configure is missing
        if not (src_dir / "configure").exists():
            print(">>> Regenerating autotools files for HDRL...")
            subprocess.run(["autoreconf", "-i"], cwd=src_dir, env=env, check=True)

        subprocess.run([
            "./configure",
            f"--prefix={install_dir}",
            "--disable-static",
            "--enable-shared",
            "--enable-standalone",
            f"--with-cpl={install_dir}",
            f"--with-gsl={install_dir}",
            f"--with-erfa={install_dir}",
        ], cwd=src_dir, env=env, check=True)

        subprocess.run(["make", f"-j{njobs}"], cwd=src_dir, check=True)
        subprocess.run(["make", "install"], cwd=src_dir, check=True)

        # Fix install names on macOS for all HDRL-related libraries
        self._fix_darwin_install_names(
            install_dir / "lib",
            [
                # GSL
                "libgsl.28.dylib",
                "libgslcblas.0.dylib",
                # ERFA
                "liberfa.1.dylib",
                # HDRL
                "libhdrl.1.dylib",
            ],
        )

        # Clean up build artifacts
        subprocess.run(["make", "distclean"], cwd=src_dir, check=False)
        print(">>> HDRL built successfully")

    def _fix_darwin_install_names(self, lib_dir: Path, libraries: list[str]) -> None:
        """Fix macOS dylib install names and dependencies to use @rpath so they can be relocated."""
        if sys.platform != "darwin":
            return

        lib_dir = Path(lib_dir)

        # First pass: fix install names
        for name in libraries:
            dylib = lib_dir / name
            if not dylib.exists():
                continue
            subprocess.run(
                ["install_name_tool", "-id", f"@rpath/{name}", str(dylib)],
                check=True,
            )

        # Second pass: fix dependencies between libraries
        for name in libraries:
            dylib = lib_dir / name
            if not dylib.exists():
                continue

            # Get list of dependencies
            result = subprocess.run(
                ["otool", "-L", str(dylib)],
                capture_output=True,
                text=True,
                check=True,
            )

            # Parse otool output and fix any absolute paths
            for line in result.stdout.splitlines()[1:]:  # Skip first line (the dylib itself)
                line = line.strip()
                if not line:
                    continue
                # Extract the path (before the version info in parentheses)
                dep_path = line.split('(')[0].strip()

                # If it's an absolute path in the build directory, fix it
                if str(lib_dir) in dep_path or "/Users/runner/" in dep_path:
                    # Extract just the library filename
                    dep_name = Path(dep_path).name
                    # Change to use @rpath
                    subprocess.run(
                        ["install_name_tool", "-change", dep_path, f"@rpath/{dep_name}", str(dylib)],
                        check=True,
                    )

    def _copy_vendored_libraries(self, extdir: Path) -> None:
        """Copy vendored shared libraries alongside the extension module."""
        import shutil
        import glob

        deps_install_dir = self._deps_dir() / "install"
        lib_dir = deps_install_dir / "lib"

        if not lib_dir.exists():
            print(f"Warning: Library directory {lib_dir} does not exist")
            return

        extdir = Path(extdir).resolve()
        extdir.mkdir(parents=True, exist_ok=True)

        if sys.platform == "darwin":
            lib_pattern = "*.dylib"
        else:
            lib_pattern = "*.so*"

        print(f"\nCopying vendored libraries from {lib_dir} to {extdir}")
        for lib_file in glob.glob(str(lib_dir / lib_pattern)):
            lib_path = Path(lib_file)
            if lib_path.is_file() and not lib_path.is_symlink():
                dest = extdir / lib_path.name
                print(f"  Copying {lib_path.name}")
                shutil.copy2(lib_path, dest)
            elif lib_path.is_symlink():
                dest = extdir / lib_path.name
                link_target = os.readlink(lib_path)
                if dest.exists() or dest.is_symlink():
                    dest.unlink()
                os.symlink(link_target, dest)
                print(f"  Creating symlink {lib_path.name} -> {link_target}")

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
        hdrldir = os.environ.get("HDRLDIR", None)
        if hdrldir is not None:
            cmake_args += [f"-DHDRL_ROOT:PATH={Path(hdrldir).resolve()}"]
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

        # Copy vendored libraries alongside the extension
        self._copy_vendored_libraries(extdir)


setup(
    ext_modules=[CMakeExtension("cpl")],
    cmdclass={"build_ext": CMakeBuildExt},
    packages=["hdrl"],
    package_data={"": ["*.so", "*.so.*", "*.dylib"]},
    include_package_data=True,
)
