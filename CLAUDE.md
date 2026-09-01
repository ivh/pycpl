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
└── hdrl-1.6.0/         # ESO High-level Data Reduction Library
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

**Vendored deps are shared across Python versions.** They are built into
`build/deps-<platform>/` (keyed on `sysconfig.get_platform()`, not the interpreter) and
guarded by a `.deps-complete` stamp listing the `vendor/` tree names. cibuildwheel builds
cp312/cp313/cp314 in one job, so the C stack is built once instead of three times — on the
slow macos-15-intel runner that was ~14 min of the 54 min wall clock. Bump
`DEPS_STAMP_SCHEMA` in `setup.py` if the dependency build changes in a way that old trees
would not pick up. The workflow additionally caches `build/deps-*` between runs, keyed on
the `vendor/` listing plus a hash of `setup.py`.

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
  os: [ubuntu-latest, ubuntu-22.04-arm, macos-latest, macos-15-intel]
```

- `ubuntu-latest`: Linux x86_64 (manylinux_2_28)
- `ubuntu-22.04-arm`: Linux aarch64 (manylinux_2_28)
- `macos-latest`: Apple Silicon arm64 (native builds only)
- `macos-15-intel`: Intel x86_64

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

Before tagging:

1. `python patches/apply.py --check` — every patch must report `pending` or `applied`,
   never `stale`.
2. Build once locally and smoke-test the result, since CI does not run the test suite:
   ```bash
   uv run --no-project --python 3.13 --with 'pybind11<3.1' --with setuptools --with cmake \
     python setup.py build_ext --inplace
   env -u DYLD_LIBRARY_PATH PYTHONPATH=$PWD uv run --no-project --with numpy python -c "
import cpl, cpl.ui as ui
print(cpl.__file__, cpl.__version__)
p = ui.ParameterValue('p', 'd', 'c', 1); p.value = 2; assert p.value == 2
b = ui.ParameterValue('b', 'd', 'c', True); b.value = False; assert b.value is False
assert str(cpl.core.Property('K', 3).type) == 'Type.LONG'
assert str(cpl.core.Property('K', 1.5).type) == 'Type.DOUBLE'
print('round-trip ok')"
   ```
   Check `cpl.__file__` — a stale `.so` elsewhere on the path silently passes otherwise.
   The round-trip assertions are not decoration: printing the version alone passes on a
   build where every int and bool parameter is unusable and every numeric FITS keyword is
   written as complex, which is exactly how v1.0.4.post5 and post6 shipped
   (`eso-bugs/10_pybind11_31_variant_resolution.py`). Anything that changes the toolchain
   — lifting a pybind11 pin above all — needs a *runtime* check; the compile succeeds.
   Then `python patches/apply.py --revert`, and delete the in-place artifacts
   (`rm -f *.dylib cpl.*.so src/cpl.*.so`).
3. Bump `version` in `pyproject.toml` (`.postN`, N+1) and add a `CHANGELOG.md` entry.
4. Commit, **push master, and confirm the push succeeded** before tagging. `origin/master`
   moves on its own: the release workflow's `update_index` job commits there. Do not pipe
   the push through anything — `git push | tail` returns the pipe's status, so a rejected
   push looks like success and `&&` will not stop the tag from going out.

Then:

```bash
git tag v1.0.4.post6
git push origin v1.0.4.post6
```

The workflow builds wheels, creates the GitHub Release, regenerates the package index and
commits it to master. The new version is installable from GitHub Pages as soon as it
finishes (~45 min).

If a tag ever goes out ahead of master, merge `origin/master` rather than rebasing: the
tagged commit has to stay reachable, and retagging while the workflow runs against that
tag is worse than a merge commit.

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

An in-place build does work, and is the fastest way to test a change to `src/` or
`patches/` — the vendored C stack in `build/deps-<platform>/` is reused, so only the
extension recompiles (a few minutes):

```bash
uv run --no-project --python 3.13 --with 'pybind11<3.1' --with setuptools --with cmake \
  python setup.py build_ext --inplace
env -u DYLD_LIBRARY_PATH PYTHONPATH=$PWD uv run --no-project --with numpy python -c \
  "import cpl; print(cpl.__file__)"
```

Always pin `PYTHONPATH` and print `cpl.__file__`: an installed or stale copy will
otherwise be imported instead of the one just built. Clean up afterwards (see Development
Notes). Alternatively install a pre-built wheel from the GitHub Pages index, or build one
with `cibuildwheel`.

## Development Notes

- **Never** commit changes that would break the vendored library build
- Test both Linux and macOS wheels before tagging a release
- Check wheel contents: `python -m zipfile -l <wheel>.whl`
- Check RPATH on Linux: `patchelf --print-rpath <module>.so`
- Check install names on macOS: `otool -L <module>.so`
- `build_ext --inplace` drops the extension plus ~16 MB of vendored dylibs in the repo
  root (`_copy_vendored_libraries`). All gitignored, but clean them up: `MANIFEST.in` has
  `global-include *.so *.so.* *.dylib`, so a *local* `uv build`/`sdist` would package
  them. CI is unaffected — it builds from a clean checkout.
- Never import `cpl` from inside the repo without checking `cpl.__file__`. Old in-place
  builds linger and silently pass tests that should fail.

## Local Patches and Bug Reports

`src/` is pristine upstream; the local bug fixes live in `patches/`, applied to it by
`setup.py` before every build, so the published wheels differ from ESO's by exactly that
directory. `patches/README.md` carries the rule for what may be added: a patch must never
change the result of a call that already succeeds against ESO's wheels — only errors,
undefined behaviour and leaks are fair game — and each one needs a ticket filed upstream
first.

```bash
python patches/apply.py --check | --apply | --revert
```

`eso-bugs/` holds a standalone reproducer per reported defect. Several no longer fire
against our own wheels precisely because `patches/` fixes them: run them against ESO's
build.

## Upgrading PyCPL/PyHDRL Sources

`.github/workflows/check-upstream.yml` runs weekly and opens (or updates, or closes) an
issue titled "Upstream ESO releases available" when a vendored component falls behind, so
this normally starts from that issue rather than from watching the FTP site.

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
The local bug fixes live in `patches/`, applied to `src/` by `setup.py` at build time, so
they never enter the repo: run `python patches/apply.py --revert` before the swap and
`--check` after it. A patch reporting `stale` means ESO touched that code — read their
version, then rebase the patch or delete it. Never fuzz one; `patches/README.md` explains
why and which divergences are allowed at all.

**READMEs and change logs**: ours are `README.md` and `CHANGELOG.md`, ESO's are kept
verbatim as `README_orig.md` and `CHANGELOG_orig.md`. An upstream tarball will want to
write its own `README.md`/`CHANGELOG.md` — put those in the `_orig` files instead. This is
not a patch, because a rename has nothing to fail on at build time; instead
`patches/apply.py` checks that `README.md` still carries our first line and fails the
build if it does not.

**New CPL/HDRL C libraries**: PyCPL/PyHDRL state their minimum in their own README
(here: `README_orig.md`)
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
