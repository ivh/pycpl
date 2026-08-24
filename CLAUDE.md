# PyCPL Build System Documentation

## Project Overview

PyCPL provides Python bindings for the ESO Common Pipeline Library (CPL) and HDRL (High-level Data Reduction Library) using pybind11. The project bundles all C library dependencies to provide a self-contained wheel that works without system dependencies.

The package provides two top-level imports:
- `import cpl` - CPL bindings
- `import hdrl` - HDRL bindings (also available as `from cpl import hdrl`)

## Architecture

### Vendored Dependencies (vendor/)

The project vendors all C library dependencies to ensure reproducible builds:

```
vendor/
├── cfitsio-4.6.2/      # FITS file I/O
├── fftw-3.3.10/        # Fast Fourier Transform (double + single precision)
├── wcslib-8.2.2/       # World Coordinate System transformations
├── gsl-2.8/            # GNU Scientific Library (for HDRL)
├── erfa-2.0.1/         # Essential Routines for Fundamental Astronomy (for HDRL)
├── cpl-7.4/            # ESO Common Pipeline Library
│   ├── libcext/        # CPL extension library
│   ├── cplcore/        # Core CPL functionality
│   ├── cplui/          # User interface components
│   ├── cpldfs/         # Data flow system
│   └── cpldrs/         # Data reduction system
└── hdrl-1.6.0a5/       # ESO High-level Data Reduction Library
```

**Why vendored?** CPL and its dependencies are not available via system package managers on all platforms, and version compatibility is critical.

## Build Process (setup.py)

### Build Phases

The build uses a custom `CMakeBuildExt` class that extends setuptools:

1. **Phase 1: Build cfitsio, fftw, gsl, erfa in parallel**
   - cfitsio and fftw built with CMake
   - gsl and erfa built with autotools
   - Installed to `build/temp.*/deps/install/`
   - `-DCMAKE_INSTALL_LIBDIR=lib` forces use of `lib/` not `lib64/` (important for manylinux)

2. **Phase 2: Build wcslib**
   - Depends on cfitsio
   - Uses autotools (configure/make)
   - Requires CFITSIO_CFLAGS and LDFLAGS to find vendored cfitsio

3. **Phase 3: Build CPL**
   - Depends on cfitsio, fftw, wcslib
   - Uses autotools
   - `--disable-java` prevents building Java components (would need libtool-ltdl)
   - `JAVA_HOME` unset to prevent Java auto-detection

4. **Phase 4: Build HDRL**
   - Depends on CPL, GSL, ERFA
   - Uses autotools
   - `--enable-standalone` for standalone library build

5. **Phase 5: Build Python extension**
   - Uses CMake + pybind11
   - Links against vendored CPL and HDRL libraries

6. **Phase 6: Copy vendored libraries**
   - All `.so`/`.dylib` files copied alongside extension module
   - Enables self-contained wheels

### Key Build Settings

**CMakeLists.txt:**
```cmake
INSTALL_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>"
BUILD_WITH_INSTALL_RPATH TRUE
```
- Linux: `$ORIGIN` = look in same directory as .so
- macOS: `@loader_path` = macOS equivalent

**Python version:**
```cmake
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
```
- `Development.Module` not `Development` - only needs headers, not libpython.so
- Critical for manylinux containers where full Python libraries aren't available

### macOS-Specific: Dylib Install Names

macOS embeds library paths into binaries. We fix these after building:

```python
def _fix_darwin_install_names(self, lib_dir, libraries):
    # 1. Fix library's own install name: -id @rpath/libname
    # 2. Fix dependencies: -change /absolute/path @rpath/libname
```

**Why?** Without this, dylibs reference absolute build paths like `/Users/runner/work/...` which don't exist on user machines.

## GitHub Actions Workflow (.github/workflows/python-publish.yml)

### Trigger Conditions

- **Manual**: `workflow_dispatch` - builds wheels only
- **Release**: Push tag `v*` - builds wheels, creates GitHub release, and updates package index

### Build Matrix

```yaml
matrix:
  os: [ubuntu-latest, macos-15-intel, macos-latest]
```

- `ubuntu-latest`: Linux x86_64 (manylinux_2_28)
- `macos-15-intel`: Intel x86_64
- `macos-latest`: Apple Silicon arm64 (native builds only)

### Python Versions

```toml
build = ["cp312-*", "cp313-*", "cp314-*"]
```

Wheels built for Python 3.12+. Package declares `requires-python = ">=3.9"` so older Python can build from source if needed.

### Platform-Specific Settings

**Linux:**
```toml
[tool.cibuildwheel.linux]
before-build = "yum install -y autoconf automake libtool"
repair-wheel-command = ""  # Skip auditwheel - we bundle libraries ourselves
manylinux-x86_64-image = "manylinux_2_28"
```

**macOS:**
```toml
[tool.cibuildwheel.macos]
archs = ["native"]  # Native builds only, no cross-compilation
before-build = "brew install autoconf automake libtool"
repair-wheel-command = ""  # Skip delocate - we handle dylibs ourselves
environment = { MACOSX_DEPLOYMENT_TARGET = "11.0" }
```

- Deployment target 11.0 for C++17 `<filesystem>` support
- Native builds only (no cross-compilation) for faster builds

### Why Skip Repair Tools?

- **auditwheel** (Linux) and **delocate** (macOS) normally bundle external libraries
- They failed because they couldn't find our vendored libraries during the build
- We handle bundling ourselves via `_copy_vendored_libraries()` and RPATH settings
- Our approach works because:
  1. Libraries are copied to wheel root alongside extension
  2. Extension has RPATH=$ORIGIN/@loader_path
  3. Library install names use @rpath (macOS)

## Distribution via GitHub Pages

Since PyPI rejected the package name as too similar to ESO's `pycpl`, we distribute wheels via a custom package index hosted on GitHub Pages.

### Package Index Structure

The `simple/` directory contains a PEP 503 compliant package index:

```
simple/
├── .nojekyll              # Disable Jekyll processing
├── index.html             # Root index
└── pycpl/
    └── index.html         # Package index with links to all wheel releases
```

### Automatic Index Updates

When a new release is tagged, the workflow automatically:

1. Builds wheels for all platforms
2. Creates a GitHub Release with wheel attachments
3. Runs `update-index.sh` to regenerate the index from all releases
4. Commits and pushes the updated index to master

The `update_index` job queries all GitHub releases and generates HTML with direct links to wheel files.

### Manual Index Update

To manually update the index:
```bash
./update-index.sh
git add simple/
git commit -m "update package index"
git push
```

### Installation

Users install with:
```bash
pip install pycpl --extra-index-url https://ivh.github.io/pycpl/simple/
```

Or configure in `pyproject.toml`:
```toml
[[tool.uv.index]]
name = "pycpl"
url = "https://ivh.github.io/pycpl/simple/"
```

This allows pip/uv to find pycpl from our index while still using PyPI for dependencies like numpy and astropy.

### Creating a New Release

1. Push a tag:
   ```bash
   git tag v1.0.3.post2
   git push origin v1.0.3.post2
   ```

2. Workflow automatically:
   - Builds wheels
   - Creates GitHub Release
   - Updates package index
   - Commits updated index to master

3. New version is immediately installable from GitHub Pages

## Common Issues & Solutions

### ImportError: cannot open shared object file (Linux)

**Symptom:** `libcpldrs.so.26: cannot open shared object file`

**Cause:** Extension module doesn't have RPATH set

**Solution:** INSTALL_RPATH in CMakeLists.txt

### Library not loaded: /Users/runner/... (macOS)

**Symptom:** Absolute build paths in error

**Cause:** Dylib install names not fixed

**Solution:** `_fix_darwin_install_names()` uses `install_name_tool -id` and `-change`

### Symbol not found on macOS, resolved against a system CPL

**Symptom:** `ImportError: dlopen(...): Symbol not found: _cpl_wcs_duplicate`, with
`Expected in: <somewhere>/lib/libcpldrs.26.dylib` pointing outside site-packages.

**Cause:** `DYLD_LIBRARY_PATH` (set by ESO pipeline/`esorex` setups) is searched by
library *leaf name* before the extension's `@rpath`, so a system CPL shadows the bundled
libraries. Harmless while the versions match; fatal once they diverge (CPL 7.4 added
`cpl_wcs_duplicate`). Linux is immune because `--disable-new-dtags` yields `DT_RPATH`,
which is searched before `LD_LIBRARY_PATH`.

**Solution:** run with `env -u DYLD_LIBRARY_PATH`. A permanent fix would require giving
the bundled dylibs unique leaf names so they cannot be shadowed.

### CMake can't find Python3_LIBRARIES (manylinux)

**Symptom:** `Could NOT find Python3 (missing: Python3_LIBRARIES Development)`

**Cause:** Looking for full Python development package

**Solution:** Use `Development.Module` instead of `Development`

### CPL tries to build Java components

**Symptom:** `No rule to make target -lltdl needed by libcplgasgano.la`

**Cause:** Java found in build environment

**Solution:** `--disable-java` + unset `JAVA_HOME`

## File Manifest in Wheels

```
pycpl-1.0.3-cp312-cp312-linux_x86_64.whl:
  cpl.cpython-312-x86_64-linux-gnu.so    # Extension module
  hdrl/                                   # Pure Python hdrl package
    __init__.py                           # Re-exports from cpl.hdrl
  libcext.so.*                            # Vendored libraries
  libcfitsio.so.*
  libcplcore.so.*
  libcpldfs.so.*
  libcpldrs.so.*
  libcplui.so.*
  libfftw3.so.*
  libfftw3_threads.so.*
  libfftw3f.so.*
  libfftw3f_threads.so.*
  libwcs.so.*
  libgsl.so.*                             # HDRL dependencies
  libgslcblas.so.*
  liberfa.so.*
  libhdrl.so.*
  [symlinks to versioned .so files]
```

All libraries at wheel root, extension has RPATH to find them.

## Local Development

Editable installs (`uv sync` or `pip install -e`) don't work because the multi-stage native build (cfitsio -> fftw -> wcslib -> CPL -> pybind11) fails when run in temporary directories. Use:

```bash
uv sync --no-install-project  # Sync dependencies only, skip building pycpl
```

To test pycpl locally, install a pre-built wheel from the GitHub Pages index or build one with `cibuildwheel`.

## Development Notes

- **Never** commit changes that would break the vendored library build
- Test both Linux and macOS wheels before tagging a release
- Check wheel contents: `python -m zipfile -l <wheel>.whl`
- Check RPATH on Linux: `patchelf --print-rpath <module>.so`
- Check install names on macOS: `otool -L <module>.so`

## Upgrading PyCPL/PyHDRL Sources

When ESO releases new versions:

**New PyCPL**: Replace `src/cplcore/`, `src/cpldfs/`, `src/cpldrs/`, `src/cplui/`

**New PyHDRL**: Replace `src/hdrlcore/`, `src/hdrlfunc/`, `src/hdrldebug/`
  - Re-apply function renames to avoid linker conflicts with CPL:

| Original | Renamed |
|----------|---------|
| `bind_errors` (hdrlcore) | `bind_hdrl_errors` |
| `bind_image` (hdrlcore) | `bind_hdrl_image` |
| `bind_imagelist` (hdrlcore) | `bind_hdrl_imagelist` |
| `bind_types` (hdrldebug) | `bind_hdrl_types` |

**pycpl.cpp**: Small file with module init code, manual merge if needed.

**Upstream sources**: https://ftp.eso.org/pub/dfs/pipelines/libraries/{pycpl,pyhdrl,cpl,hdrl}/

Apart from the renames above and `pycpl.cpp`, `src/` is pristine upstream — verify with a
diff ignoring the copyright-year line, which upstream bumps on every file each release.

**New CPL/HDRL C libraries**: PyCPL/PyHDRL state their minimum in their `README.md`
(e.g. PyCPL 1.0.4 needs cpl >= 7.4). Update the two paths in `setup.py`
(`_build_cpl`, `_build_hdrl`). Vendored trees are pruned to keep the repo small:
drop `html/` + `ChangeLog` (CPL) and `tests/` + `doxygen/` (HDRL).

**CPL >= 7.4 defaults `--with-system-cext` to `yes`** and then hard-fails with
"libcext (headers) was not found" when no system libcext exists (7.3.2 defaulted to `no`).
`setup.py` passes `--with-system-cext=no` to force the bundled one. Beware: a stale
`cext.pc` left in `build/` from an earlier release satisfies the pkg-config probe, so a
local incremental build can succeed while CI fails — always `rm -rf build` before trusting
a vendor upgrade.

**Vendored trees must be committed whole.** Root `.gitignore` rules (`**/doc/`,
`ChangeLog`) silently excluded upstream files that autotools needs, so the build worked
locally but failed on a fresh CI checkout (HDRL's automake is GNU-strict and needs
`ChangeLog`; its configure generates `doc/Doxyfile`). `.gitignore` now carries
`!vendor/**/ChangeLog` and `!vendor/**/doc/`. After any vendor swap, verify nothing is
hidden:

```bash
git status --ignored --short vendor/ | grep '^!!'          # expect only build scratch
diff <(find vendor/<tree> -type f | sort) <(git ls-files vendor/<tree>)
```

**HDRL is patched to drop libcurl** — re-apply on every HDRL upgrade:
1. `hdrl_download.c` replaced by a stub returning `CPL_ERROR_UNSUPPORTED_MODE`
   (keep the old copy; the two declarations in `hdrl_download.h` rarely change).
2. `configure.ac`: comment out `ESO_CHECK_LIBCURL`, drop `tests/Makefile` and
   `catalogue/tests/Makefile` from `AC_CONFIG_FILES`.
3. `Makefile.am`: `SUBDIRS = catalogue .`, and remove `$(LIBCURL_CFLAGS)`/`$(LIBCURL_LIBS)`.
4. Run `autoreconf -i` in the vendored tree and commit the generated files — `setup.py`
   only regenerates them when `configure` is absent.

## References

- CPL Documentation: http://www.eso.org/sci/software/cpl
- HDRL Documentation: http://www.eso.org/sci/software/hdrl
- cibuildwheel docs: https://cibuildwheel.readthedocs.io/
- PEP 503 (Simple Repository API): https://peps.python.org/pep-0503/
- GitHub Pages: https://pages.github.com/
