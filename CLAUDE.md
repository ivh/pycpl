# PyCPL Build System Documentation

## Project Overview

PyCPL provides Python bindings for the ESO Common Pipeline Library (CPL) using pybind11. The project bundles all C library dependencies to provide a self-contained wheel that works without system dependencies.

## Architecture

### Vendored Dependencies (vendor/)

The project vendors all C library dependencies to ensure reproducible builds:

```
vendor/
├── cfitsio-4.6.2/      # FITS file I/O
├── fftw-3.3.10/        # Fast Fourier Transform (double + single precision)
├── wcslib-8.2.2/       # World Coordinate System transformations
└── cpl-7.3.2/          # ESO Common Pipeline Library
    ├── libcext/        # CPL extension library
    ├── cplcore/        # Core CPL functionality
    ├── cplui/          # User interface components
    ├── cpldfs/         # Data flow system
    └── cpldrs/         # Data reduction system
```

**Why vendored?** CPL and its dependencies are not available via system package managers on all platforms, and version compatibility is critical.

## Build Process (setup.py)

### Build Phases

The build uses a custom `CMakeBuildExt` class that extends setuptools:

1. **Phase 1: Build cfitsio and fftw in parallel**
   - Built with CMake
   - Installed to `build/temp.*/deps/install/`
   - `-DCMAKE_INSTALL_LIBDIR=lib` forces use of `lib/` not `lib64/` (important for manylinux)

2. **Phase 2: Build wcslib**
   - Depends on cfitsio
   - Uses autotools (configure/make)
   - Requires CFITSIO_CFLAGS and LDFLAGS to find vendored cfitsio

3. **Phase 3: Build CPL**
   - Depends on all previous libraries
   - Uses autotools
   - `--disable-java` prevents building Java components (would need libtool-ltdl)
   - `JAVA_HOME` unset to prevent Java auto-detection

4. **Phase 4: Build Python extension**
   - Uses CMake + pybind11
   - Links against vendored CPL libraries

5. **Phase 5: Copy vendored libraries**
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
build = ["cp311-*", "cp312-*", "cp313-*", "cp314-*"]
```

Minimum: Python 3.11 (uses C++17 features)

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
[tool.uv]
extra-index-url = ["https://ivh.github.io/pycpl/simple/"]
```

The `--extra-index-url` allows pip/uv to find pycpl from our index while still using PyPI for dependencies like numpy and astropy.

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
pycpl-0.1.0-cp311-cp311-linux_x86_64.whl:
  cpl.cpython-311-x86_64-linux-gnu.so    # Extension module
  libcext.so.0.2.4                       # Vendored libraries
  libcfitsio.so.10
  libcplcore.so.26.3.2
  libcpldfs.so.26.3.2
  libcpldrs.so.26.3.2
  libcplui.so.26.3.2
  libfftw3.so.3.6.9
  libfftw3_threads.so.3.6.9
  libfftw3f.so.3.6.9
  libfftw3f_threads.so.3.6.9
  libwcs.so.8.2.2
  [symlinks to versioned .so files]
```

All libraries at wheel root, extension has RPATH to find them.

## Development Notes

- **Never** commit changes that would break the vendored library build
- Test both Linux and macOS wheels before tagging a release
- Check wheel contents: `python -m zipfile -l <wheel>.whl`
- Check RPATH on Linux: `patchelf --print-rpath <module>.so`
- Check install names on macOS: `otool -L <module>.so`

## References

- CPL Documentation: http://www.eso.org/sci/software/cpl
- cibuildwheel docs: https://cibuildwheel.readthedocs.io/
- PEP 503 (Simple Repository API): https://peps.python.org/pep-0503/
- GitHub Pages: https://pages.github.com/
